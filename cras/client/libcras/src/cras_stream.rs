// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
use std::io;
use std::mem;
use std::{error, fmt};

use audio_streams::{BufferDrop, PlaybackBuffer, PlaybackBufferStream};
use cras_sys::gen::{
    cras_disconnect_stream_message, cras_server_message, snd_pcm_format_t, CRAS_AUDIO_MESSAGE_ID,
    CRAS_SERVER_MESSAGE_ID, CRAS_STREAM_DIRECTION,
};
use sys_util::{error, log};

use crate::audio_socket::{AudioMessage, AudioSocket};
use crate::cras_server_socket::CrasServerSocket;
use crate::cras_shm::*;

#[derive(Debug)]
pub enum ErrorType {
    IoError(io::Error),
    MessageTypeError,
    NoShmError,
}

#[derive(Debug)]
pub struct Error {
    error_type: ErrorType,
}

impl Error {
    fn new(error_type: ErrorType) -> Error {
        Error { error_type }
    }
}

impl error::Error for Error {}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self.error_type {
            ErrorType::IoError(ref err) => err.fmt(f),
            ErrorType::MessageTypeError => write!(f, "Message type error"),
            ErrorType::NoShmError => write!(f, "Shared memory area is not created"),
        }
    }
}

impl From<io::Error> for Error {
    fn from(io_err: io::Error) -> Error {
        Error::new(ErrorType::IoError(io_err))
    }
}

/// A structure controls the state of `CrasAudioHeader` and
/// interacts with server's audio thread through `AudioSocket`.
struct CrasStreamData<'a> {
    audio_sock: AudioSocket,
    header: Option<CrasAudioHeader<'a>>,
}

impl<'a> CrasStreamData<'a> {
    // Creates `CrasStreamData` with only `AudioSocket`.
    fn new(audio_sock: AudioSocket) -> Self {
        Self {
            audio_sock,
            header: None,
        }
    }
}

impl<'a> BufferDrop for CrasStreamData<'a> {
    fn trigger(&mut self, nframes: usize) {
        let log_err = |e| error!("BufferDrop error: {}", e);
        self.header.as_mut().map(|header| {
            if let Err(e) = header.commit_written_frames(nframes as u32) {
                log_err(e);
            }
        });
        if let Err(e) = self.audio_sock.data_ready(nframes as u32) {
            log_err(e);
        }
    }
}

#[allow(dead_code)]
pub struct CrasStream<'a> {
    stream_id: u32,
    server_socket: CrasServerSocket,
    block_size: u32,
    direction: CRAS_STREAM_DIRECTION,
    rate: usize,
    num_channels: usize,
    format: snd_pcm_format_t,
    /// A structure for stream to interact with server audio thread.
    controls: CrasStreamData<'a>,
    audio_buffer: Option<CrasAudioBuffer>,
}

impl<'a> CrasStream<'a> {
    /// Creates a CrasStream by given arguments.
    ///
    /// # Returns
    /// `CrasStream` - CRAS client stream.
    pub fn new(
        stream_id: u32,
        server_socket: CrasServerSocket,
        block_size: u32,
        direction: CRAS_STREAM_DIRECTION,
        rate: usize,
        num_channels: usize,
        format: snd_pcm_format_t,
        audio_sock: AudioSocket,
    ) -> Self {
        Self {
            stream_id,
            server_socket,
            block_size,
            direction,
            rate,
            num_channels,
            format,
            controls: CrasStreamData::new(audio_sock),
            audio_buffer: None,
        }
    }

    /// Receives shared memory fd and initialize stream audio shared memory area
    pub fn init_shm(&mut self, shm_fd: CrasShmFd) -> Result<(), Error> {
        let (buffer, header) = create_header_and_buffers(shm_fd)?;
        self.controls.header = Some(header);
        self.audio_buffer = Some(buffer);
        Ok(())
    }

    fn wait_request_data(&mut self) -> Result<(), Error> {
        match self.controls.audio_sock.read_audio_message()? {
            AudioMessage::Success { id, frames: _ } => match id {
                CRAS_AUDIO_MESSAGE_ID::AUDIO_MESSAGE_REQUEST_DATA => Ok(()),
                _ => Err(Error::new(ErrorType::MessageTypeError)),
            },
            _ => Err(Error::new(ErrorType::MessageTypeError)),
        }
    }
}

impl<'a> Drop for CrasStream<'a> {
    /// A blocking drop function, sends the disconnect message to `CrasClient` and waits for
    /// the return message.
    /// Logs an error message to stderr if the method fails.
    fn drop(&mut self) {
        // Send stream disconnect message
        let msg_header = cras_server_message {
            length: mem::size_of::<cras_disconnect_stream_message>() as u32,
            id: CRAS_SERVER_MESSAGE_ID::CRAS_SERVER_DISCONNECT_STREAM,
        };
        let server_cmsg = cras_disconnect_stream_message {
            header: msg_header,
            stream_id: self.stream_id,
        };
        if let Err(e) = self
            .server_socket
            .send_server_message_with_fds(&server_cmsg, &[])
        {
            error!("CrasStream::Drop error: {}", e);
        }
    }
}

impl<'a> PlaybackBufferStream for CrasStream<'a> {
    fn next_playback_buffer(&mut self) -> Result<PlaybackBuffer, Box<error::Error>> {
        // Wait for request audio message
        self.wait_request_data().map_err(|e| Box::new(e))?;
        let (frame_size, (offset, len)) = match self.controls.header.as_ref() {
            None => return Err(Error::new(ErrorType::NoShmError).into()),
            Some(header) => (header.get_frame_size(), header.get_offset_and_len()),
        };
        let buf = match self.audio_buffer.as_mut() {
            None => return Err(Error::new(ErrorType::NoShmError).into()),
            Some(audio_buffer) => &mut audio_buffer.get_buffer()[offset..offset + len],
        };
        PlaybackBuffer::new(frame_size, buf, &mut self.controls).map_err(|e| e.into())
    }
}
