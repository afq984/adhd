// Copyright 2014 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

extern "C" {
#include "cras/src/server/audio_thread.c"
#include "cras/src/server/cras_audio_area.h"
#include "cras/src/server/input_data.h"
#include "cras/src/tests/metrics_stub.h"
}

#include <gtest/gtest.h>
#include <map>

#define MAX_CALLS 10
#define BUFFER_SIZE 8192
#define FIRST_CB_LEVEL 480

static int cras_audio_thread_event_busyloop_called;
static int cras_audio_thread_event_severe_underrun_called;
static unsigned int cras_rstream_dev_offset_called;
static unsigned int cras_rstream_dev_offset_ret[MAX_CALLS];
static const struct cras_rstream*
    cras_rstream_dev_offset_rstream_val[MAX_CALLS];
static unsigned int cras_rstream_dev_offset_dev_id_val[MAX_CALLS];
static unsigned int cras_rstream_dev_offset_update_called;
static const struct cras_rstream*
    cras_rstream_dev_offset_update_rstream_val[MAX_CALLS];
static unsigned int cras_rstream_dev_offset_update_frames_val[MAX_CALLS];
static unsigned int cras_rstream_dev_offset_update_dev_id_val[MAX_CALLS];
static int cras_rstream_is_pending_reply_ret;
static int cras_iodev_all_streams_written_ret;
static struct cras_audio_area* cras_iodev_get_output_buffer_area;
static int cras_iodev_put_output_buffer_called;
static unsigned int cras_iodev_put_output_buffer_nframes;
static unsigned int cras_iodev_fill_odev_zeros_frames;
static int dev_stream_playback_frames_ret;
static int dev_stream_mix_called;
static unsigned int dev_stream_update_next_wake_time_called;
static unsigned int dev_stream_request_playback_samples_called;
static unsigned int cras_iodev_prepare_output_before_write_samples_called;
static enum CRAS_IODEV_STATE
    cras_iodev_prepare_output_before_write_samples_state;
static unsigned int cras_iodev_get_output_buffer_called;
static unsigned int cras_iodev_frames_to_play_in_sleep_called;
static int cras_iodev_prepare_output_before_write_samples_ret;
static int cras_iodev_reset_request_called;
static struct cras_iodev* cras_iodev_reset_request_iodev;
static int cras_iodev_get_valid_frames_ret;
static int cras_iodev_output_underrun_called;
static int cras_iodev_start_stream_called;
static int cras_device_monitor_reset_device_called;
static struct cras_iodev* cras_device_monitor_reset_device_iodev;
static struct cras_iodev* cras_iodev_start_ramp_odev;
static enum CRAS_IODEV_RAMP_REQUEST cras_iodev_start_ramp_request;
static struct timespec clock_gettime_retspec;
static struct timespec init_cb_ts_;
static struct timespec sleep_interval_ts_;
static std::map<const struct dev_stream*, struct timespec>
    dev_stream_wake_time_val;
static int cras_device_monitor_set_device_mute_state_called;
static int cras_iodev_is_zero_volume_ret;

void ResetGlobalStubData() {
  cras_rstream_dev_offset_called = 0;
  cras_rstream_dev_offset_update_called = 0;
  cras_rstream_is_pending_reply_ret = 0;
  for (int i = 0; i < MAX_CALLS; i++) {
    cras_rstream_dev_offset_ret[i] = 0;
    cras_rstream_dev_offset_rstream_val[i] = NULL;
    cras_rstream_dev_offset_dev_id_val[i] = 0;
    cras_rstream_dev_offset_update_rstream_val[i] = NULL;
    cras_rstream_dev_offset_update_frames_val[i] = 0;
    cras_rstream_dev_offset_update_dev_id_val[i] = 0;
  }
  cras_iodev_all_streams_written_ret = 0;
  if (cras_iodev_get_output_buffer_area) {
    free(cras_iodev_get_output_buffer_area->channels[0].buf);
    free(cras_iodev_get_output_buffer_area);
    cras_iodev_get_output_buffer_area = NULL;
  }
  cras_iodev_put_output_buffer_called = 0;
  cras_iodev_put_output_buffer_nframes = 0;
  cras_iodev_fill_odev_zeros_frames = 0;
  cras_iodev_frames_to_play_in_sleep_called = 0;
  dev_stream_playback_frames_ret = 0;
  dev_stream_mix_called = 0;
  dev_stream_request_playback_samples_called = 0;
  dev_stream_update_next_wake_time_called = 0;
  cras_iodev_prepare_output_before_write_samples_called = 0;
  cras_iodev_prepare_output_before_write_samples_state = CRAS_IODEV_STATE_OPEN;
  cras_iodev_get_output_buffer_called = 0;
  cras_iodev_prepare_output_before_write_samples_ret = 0;
  cras_iodev_reset_request_called = 0;
  cras_iodev_reset_request_iodev = NULL;
  cras_iodev_get_valid_frames_ret = 0;
  cras_iodev_output_underrun_called = 0;
  cras_iodev_start_stream_called = 0;
  cras_device_monitor_reset_device_called = 0;
  cras_device_monitor_reset_device_iodev = NULL;
  cras_iodev_start_ramp_odev = NULL;
  cras_iodev_start_ramp_request = CRAS_IODEV_RAMP_REQUEST_UP_START_PLAYBACK;
  cras_device_monitor_set_device_mute_state_called = 0;
  cras_iodev_is_zero_volume_ret = 0;
  clock_gettime_retspec.tv_sec = 0;
  clock_gettime_retspec.tv_nsec = 0;
  dev_stream_wake_time_val.clear();
}

void SetupRstream(struct cras_rstream* rstream,
                  enum CRAS_STREAM_DIRECTION direction) {
  uint32_t frame_bytes = 4;
  uint32_t used_size = 4096 * frame_bytes;

  memset(rstream, 0, sizeof(*rstream));
  rstream->direction = direction;
  rstream->cb_threshold = 480;
  rstream->format.frame_rate = 48000;

  rstream->shm = static_cast<cras_audio_shm*>(calloc(1, sizeof(*rstream->shm)));
  rstream->shm->header = static_cast<cras_audio_shm_header*>(
      calloc(1, sizeof(*rstream->shm->header)));

  rstream->shm->samples = static_cast<uint8_t*>(
      calloc(1, cras_shm_calculate_samples_size(used_size)));

  cras_shm_set_frame_bytes(rstream->shm, frame_bytes);
  cras_shm_set_used_size(rstream->shm, used_size);
}

void TearDownRstream(struct cras_rstream* rstream) {
  free(rstream->shm->samples);
  free(rstream->shm->header);
  free(rstream->shm);
}

// Test streams and devices manipulation.
class StreamDeviceSuite : public testing::Test {
 protected:
  virtual void SetUp() {
    thread_ = audio_thread_create();
    ResetStubData();
  }

  virtual void TearDown() {
    audio_thread_destroy(thread_);
    ResetGlobalStubData();
  }

  virtual void SetupDevice(cras_iodev* iodev,
                           enum CRAS_STREAM_DIRECTION direction) {
    memset(iodev, 0, sizeof(*iodev));
    iodev->info.idx = ++device_id_;
    iodev->direction = direction;
    iodev->configure_dev = configure_dev;
    iodev->close_dev = close_dev;
    iodev->frames_queued = frames_queued;
    iodev->delay_frames = delay_frames;
    iodev->get_buffer = get_buffer;
    iodev->put_buffer = put_buffer;
    iodev->flush_buffer = flush_buffer;
    iodev->format = &format_;
    iodev->buffer_size = BUFFER_SIZE;
    iodev->min_cb_level = FIRST_CB_LEVEL;
    iodev->state = CRAS_IODEV_STATE_NORMAL_RUN;
    format_.frame_rate = 48000;
  }

  void ResetStubData() {
    device_id_ = 0;
    open_dev_called_ = 0;
    close_dev_called_ = 0;
    frames_queued_ = 0;
    delay_frames_ = 0;
    audio_buffer_size_ = 0;
    cras_iodev_start_ramp_odev = NULL;
    cras_iodev_is_zero_volume_ret = 0;
  }

  void SetupPinnedStream(struct cras_rstream* rstream,
                         enum CRAS_STREAM_DIRECTION direction,
                         cras_iodev* pin_to_dev) {
    SetupRstream(rstream, direction);
    rstream->is_pinned = 1;
    rstream->pinned_dev_idx = pin_to_dev->info.idx;
  }

  static int configure_dev(cras_iodev* iodev) {
    open_dev_called_++;
    return 0;
  }

  static int close_dev(cras_iodev* iodev) {
    close_dev_called_++;
    return 0;
  }

  static int frames_queued(const cras_iodev* iodev, struct timespec* tstamp) {
    clock_gettime(CLOCK_MONOTONIC_RAW, tstamp);
    return frames_queued_;
  }

  static int delay_frames(const cras_iodev* iodev) { return delay_frames_; }

  static int get_buffer(cras_iodev* iodev,
                        struct cras_audio_area** area,
                        unsigned int* num) {
    size_t sz = sizeof(*area_) + sizeof(struct cras_channel_area) * 2;

    if (audio_buffer_size_ < *num) {
      *num = audio_buffer_size_;
    }

    area_ = (cras_audio_area*)calloc(1, sz);
    area_->frames = *num;
    area_->num_channels = 2;
    area_->channels[0].buf = audio_buffer_;
    channel_area_set_channel(&area_->channels[0], CRAS_CH_FL);
    area_->channels[0].step_bytes = 4;
    area_->channels[1].buf = audio_buffer_ + 2;
    channel_area_set_channel(&area_->channels[1], CRAS_CH_FR);
    area_->channels[1].step_bytes = 4;

    *area = area_;
    return 0;
  }

  static int put_buffer(cras_iodev* iodev, unsigned int num) {
    free(area_);
    return 0;
  }

  static int flush_buffer(cras_iodev* iodev) { return 0; }

  int device_id_;
  struct audio_thread* thread_;

  static int open_dev_called_;
  static int close_dev_called_;
  static int frames_queued_;
  static int delay_frames_;
  static struct cras_audio_format format_;
  static struct cras_audio_area* area_;
  static uint8_t audio_buffer_[BUFFER_SIZE];
  static unsigned int audio_buffer_size_;
};

int StreamDeviceSuite::open_dev_called_;
int StreamDeviceSuite::close_dev_called_;
int StreamDeviceSuite::frames_queued_;
int StreamDeviceSuite::delay_frames_;
struct cras_audio_format StreamDeviceSuite::format_;
struct cras_audio_area* StreamDeviceSuite::area_;
uint8_t StreamDeviceSuite::audio_buffer_[8192];
unsigned int StreamDeviceSuite::audio_buffer_size_;

TEST_F(StreamDeviceSuite, AddRemoveOpenOutputDevice) {
  struct cras_iodev iodev;
  struct open_dev* adev;

  SetupDevice(&iodev, CRAS_STREAM_OUTPUT);

  // Check the newly added device is open.
  thread_add_open_dev(thread_, &iodev);
  adev = thread_->open_devs[CRAS_STREAM_OUTPUT];
  EXPECT_EQ(adev->dev, &iodev);

  thread_rm_open_dev(thread_, CRAS_STREAM_OUTPUT, iodev.info.idx);
  adev = thread_->open_devs[CRAS_STREAM_OUTPUT];
  EXPECT_EQ(NULL, adev);
}

TEST_F(StreamDeviceSuite, StartRamp) {
  struct cras_iodev iodev;
  struct open_dev* adev;
  int rc;
  enum CRAS_IODEV_RAMP_REQUEST req;

  SetupDevice(&iodev, CRAS_STREAM_OUTPUT);

  // Check the newly added device is open.
  thread_add_open_dev(thread_, &iodev);
  adev = thread_->open_devs[CRAS_STREAM_OUTPUT];
  EXPECT_EQ(adev->dev, &iodev);

  // Ramp up for unmute.
  iodev.ramp = reinterpret_cast<cras_ramp*>(0x123);
  req = CRAS_IODEV_RAMP_REQUEST_UP_UNMUTE;
  rc = thread_dev_start_ramp(thread_, iodev.info.idx, req);

  EXPECT_EQ(0, rc);
  EXPECT_EQ(&iodev, cras_iodev_start_ramp_odev);
  EXPECT_EQ(req, cras_iodev_start_ramp_request);

  // Ramp down for mute.
  ResetStubData();
  req = CRAS_IODEV_RAMP_REQUEST_DOWN_MUTE;

  rc = thread_dev_start_ramp(thread_, iodev.info.idx, req);

  EXPECT_EQ(0, rc);
  EXPECT_EQ(&iodev, cras_iodev_start_ramp_odev);
  EXPECT_EQ(req, cras_iodev_start_ramp_request);

  // If device's volume percentage is zero, than ramp won't start.
  ResetStubData();
  cras_iodev_is_zero_volume_ret = 1;
  rc = thread_dev_start_ramp(thread_, iodev.info.idx, req);

  EXPECT_EQ(0, rc);
  EXPECT_EQ(NULL, cras_iodev_start_ramp_odev);
  EXPECT_EQ(1, cras_device_monitor_set_device_mute_state_called);

  // Assume iodev changed to no_stream run state, it should not use ramp.
  ResetStubData();
  iodev.state = CRAS_IODEV_STATE_NO_STREAM_RUN;
  rc = thread_dev_start_ramp(thread_, iodev.info.idx, req);

  EXPECT_EQ(0, rc);
  EXPECT_EQ(NULL, cras_iodev_start_ramp_odev);
  EXPECT_EQ(2, cras_device_monitor_set_device_mute_state_called);

  thread_rm_open_dev(thread_, CRAS_STREAM_OUTPUT, iodev.info.idx);
}

TEST_F(StreamDeviceSuite, AddRemoveOpenInputDevice) {
  struct cras_iodev iodev;
  struct open_dev* adev;

  SetupDevice(&iodev, CRAS_STREAM_INPUT);

  // Check the newly added device is open.
  thread_add_open_dev(thread_, &iodev);
  adev = thread_->open_devs[CRAS_STREAM_INPUT];
  EXPECT_EQ(adev->dev, &iodev);

  thread_rm_open_dev(thread_, CRAS_STREAM_INPUT, iodev.info.idx);
  adev = thread_->open_devs[CRAS_STREAM_INPUT];
  EXPECT_EQ(NULL, adev);
}

TEST_F(StreamDeviceSuite, AddRemoveMultipleOpenDevices) {
  struct cras_iodev odev;
  struct cras_iodev odev2;
  struct cras_iodev odev3;
  struct cras_iodev idev;
  struct cras_iodev idev2;
  struct cras_iodev idev3;
  struct open_dev* adev;

  SetupDevice(&odev, CRAS_STREAM_OUTPUT);
  SetupDevice(&odev2, CRAS_STREAM_OUTPUT);
  SetupDevice(&odev3, CRAS_STREAM_OUTPUT);
  SetupDevice(&idev, CRAS_STREAM_INPUT);
  SetupDevice(&idev2, CRAS_STREAM_INPUT);
  SetupDevice(&idev3, CRAS_STREAM_INPUT);

  // Add 2 open devices and check both are open.
  thread_add_open_dev(thread_, &odev);
  thread_add_open_dev(thread_, &odev2);
  adev = thread_->open_devs[CRAS_STREAM_OUTPUT];
  EXPECT_EQ(adev->dev, &odev);
  EXPECT_EQ(adev->next->dev, &odev2);

  // Remove first open device and check the second one is still open.
  thread_rm_open_dev(thread_, CRAS_STREAM_OUTPUT, odev.info.idx);
  adev = thread_->open_devs[CRAS_STREAM_OUTPUT];
  EXPECT_EQ(adev->dev, &odev2);

  // Add another open device and check both are open.
  thread_add_open_dev(thread_, &odev3);
  adev = thread_->open_devs[CRAS_STREAM_OUTPUT];
  EXPECT_EQ(adev->dev, &odev2);
  EXPECT_EQ(adev->next->dev, &odev3);

  // Add 2 open devices and check both are open.
  thread_add_open_dev(thread_, &idev);
  thread_add_open_dev(thread_, &idev2);
  adev = thread_->open_devs[CRAS_STREAM_INPUT];
  EXPECT_EQ(adev->dev, &idev);
  EXPECT_EQ(adev->next->dev, &idev2);

  // Remove first open device and check the second one is still open.
  thread_rm_open_dev(thread_, CRAS_STREAM_INPUT, idev.info.idx);
  adev = thread_->open_devs[CRAS_STREAM_INPUT];
  EXPECT_EQ(adev->dev, &idev2);

  // Add and remove another open device and check still open.
  thread_add_open_dev(thread_, &idev3);
  thread_rm_open_dev(thread_, CRAS_STREAM_INPUT, idev3.info.idx);
  adev = thread_->open_devs[CRAS_STREAM_INPUT];
  EXPECT_EQ(adev->dev, &idev2);
  thread_rm_open_dev(thread_, CRAS_STREAM_INPUT, idev2.info.idx);
  thread_rm_open_dev(thread_, CRAS_STREAM_OUTPUT, odev2.info.idx);
  thread_rm_open_dev(thread_, CRAS_STREAM_OUTPUT, odev3.info.idx);
}

TEST_F(StreamDeviceSuite, MultipleInputStreamsCopyFirstStreamOffset) {
  struct cras_iodev iodev;
  struct cras_iodev iodev2;
  struct cras_iodev* iodevs[] = {&iodev, &iodev2};
  struct cras_rstream rstream;
  struct cras_rstream rstream2;
  struct cras_rstream rstream3;

  SetupDevice(&iodev, CRAS_STREAM_INPUT);
  SetupDevice(&iodev2, CRAS_STREAM_INPUT);
  SetupRstream(&rstream, CRAS_STREAM_INPUT);
  SetupRstream(&rstream2, CRAS_STREAM_INPUT);
  SetupRstream(&rstream3, CRAS_STREAM_INPUT);

  thread_add_open_dev(thread_, &iodev);
  thread_add_open_dev(thread_, &iodev2);

  thread_add_stream(thread_, &rstream, iodevs, 2);
  EXPECT_NE((void*)NULL, iodev.streams);
  EXPECT_NE((void*)NULL, iodev2.streams);

  EXPECT_EQ(0, cras_rstream_dev_offset_called);
  EXPECT_EQ(0, cras_rstream_dev_offset_update_called);

  // Fake offset for rstream
  cras_rstream_dev_offset_ret[0] = 30;
  cras_rstream_dev_offset_ret[1] = 0;

  thread_add_stream(thread_, &rstream2, iodevs, 2);
  EXPECT_EQ(2, cras_rstream_dev_offset_called);
  EXPECT_EQ(&rstream, cras_rstream_dev_offset_rstream_val[0]);
  EXPECT_EQ(iodev.info.idx, cras_rstream_dev_offset_dev_id_val[0]);
  EXPECT_EQ(&rstream, cras_rstream_dev_offset_rstream_val[1]);
  EXPECT_EQ(iodev2.info.idx, cras_rstream_dev_offset_dev_id_val[1]);

  EXPECT_EQ(2, cras_rstream_dev_offset_update_called);
  EXPECT_EQ(&rstream2, cras_rstream_dev_offset_update_rstream_val[0]);
  EXPECT_EQ(30, cras_rstream_dev_offset_update_frames_val[0]);
  EXPECT_EQ(&rstream2, cras_rstream_dev_offset_update_rstream_val[1]);
  EXPECT_EQ(0, cras_rstream_dev_offset_update_frames_val[1]);

  thread_rm_open_dev(thread_, CRAS_STREAM_INPUT, iodev.info.idx);
  thread_rm_open_dev(thread_, CRAS_STREAM_INPUT, iodev2.info.idx);
  TearDownRstream(&rstream);
  TearDownRstream(&rstream2);
  TearDownRstream(&rstream3);
}

TEST_F(StreamDeviceSuite, InputStreamsSetInputDeviceWakeTime) {
  struct cras_iodev iodev;
  struct cras_iodev* iodevs[] = {&iodev};
  struct cras_rstream rstream1, rstream2;
  struct timespec ts_wake_1 = {.tv_sec = 1, .tv_nsec = 500};
  struct timespec ts_wake_2 = {.tv_sec = 1, .tv_nsec = 1000};
  struct open_dev* adev;

  SetupDevice(&iodev, CRAS_STREAM_INPUT);
  SetupRstream(&rstream1, CRAS_STREAM_INPUT);
  SetupRstream(&rstream2, CRAS_STREAM_INPUT);

  thread_add_open_dev(thread_, &iodev);
  thread_add_stream(thread_, &rstream1, iodevs, 1);
  thread_add_stream(thread_, &rstream2, iodevs, 1);
  EXPECT_NE((void*)NULL, iodev.streams);

  // Assume device is running.
  iodev.state = CRAS_IODEV_STATE_NORMAL_RUN;

  // Set stub data for dev_stream_wake_time.
  dev_stream_wake_time_val[iodev.streams] = ts_wake_1;
  dev_stream_wake_time_val[iodev.streams->next] = ts_wake_2;

  // Send captured samples to client.
  // This will also update wake time for this device based on
  // dev_stream_wake_time of each stream of this device.
  dev_io_send_captured_samples(thread_->open_devs[CRAS_STREAM_INPUT]);

  // wake_ts is maintained in open_dev.
  adev = thread_->open_devs[CRAS_STREAM_INPUT];

  // The wake up time for this device is the minimum of
  // ts_wake_1 and ts_wake_2.
  EXPECT_EQ(ts_wake_1.tv_sec, adev->wake_ts.tv_sec);
  EXPECT_EQ(ts_wake_1.tv_nsec, adev->wake_ts.tv_nsec);

  thread_rm_open_dev(thread_, CRAS_STREAM_INPUT, iodev.info.idx);
  TearDownRstream(&rstream1);
  TearDownRstream(&rstream2);
}

TEST_F(StreamDeviceSuite, AddOutputStream) {
  struct cras_iodev iodev, *piodev = &iodev;
  struct cras_rstream rstream;
  struct cras_audio_shm_header* shm_header;
  struct dev_stream* dev_stream;
  struct open_dev* adev;

  ResetGlobalStubData();
  SetupDevice(&iodev, CRAS_STREAM_OUTPUT);
  SetupRstream(&rstream, CRAS_STREAM_OUTPUT);
  shm_header = rstream.shm->header;

  thread_add_open_dev(thread_, &iodev);
  thread_add_stream(thread_, &rstream, &piodev, 1);
  dev_stream = iodev.streams;
  EXPECT_EQ(dev_stream->stream, &rstream);
  /*
   * When a output stream is added, the start_stream function will be called
   * just before its first fetch.
   */
  EXPECT_EQ(cras_iodev_start_stream_called, 0);

  adev = thread_->open_devs[CRAS_STREAM_OUTPUT];

  shm_header->write_buf_idx = 0;
  shm_header->write_offset[0] = 0;

  // Assume device is started.
  iodev.state = CRAS_IODEV_STATE_NORMAL_RUN;

  // Fetch stream.
  cras_rstream_is_pending_reply_ret = 0;
  dev_io_playback_fetch(adev);
  EXPECT_EQ(dev_stream_request_playback_samples_called, 1);
  EXPECT_EQ(cras_iodev_start_stream_called, 1);

  thread_rm_open_dev(thread_, CRAS_STREAM_OUTPUT, iodev.info.idx);
  TearDownRstream(&rstream);
}

TEST_F(StreamDeviceSuite, OutputStreamFetchTime) {
  struct cras_iodev iodev, *piodev = &iodev;
  struct cras_rstream rstream1, rstream2;
  struct dev_stream* dev_stream;
  struct timespec expect_ts;

  ResetGlobalStubData();
  SetupDevice(&iodev, CRAS_STREAM_OUTPUT);
  SetupRstream(&rstream1, CRAS_STREAM_OUTPUT);
  SetupRstream(&rstream2, CRAS_STREAM_OUTPUT);

  thread_add_open_dev(thread_, &iodev);

  // Add a new stream. init_cb_ts should be the time right now.
  clock_gettime_retspec.tv_sec = 1;
  clock_gettime_retspec.tv_nsec = 500;
  cras_iodev_get_valid_frames_ret = 0;
  expect_ts = clock_gettime_retspec;
  thread_add_stream(thread_, &rstream1, &piodev, 1);
  dev_stream = iodev.streams;
  EXPECT_EQ(dev_stream->stream, &rstream1);
  EXPECT_EQ(init_cb_ts_.tv_sec, expect_ts.tv_sec);
  EXPECT_EQ(init_cb_ts_.tv_nsec, expect_ts.tv_nsec);

  thread_rm_open_dev(thread_, CRAS_STREAM_OUTPUT, iodev.info.idx);

  thread_add_open_dev(thread_, &iodev);

  /*
   * Add a new stream when there are remaining frames in device buffer.
   * init_cb_ts should be the time that hw_level drops to min_cb_level.
   * In this case, we should wait 480 / 48000 = 0.01s.
   */
  clock_gettime_retspec.tv_sec = 1;
  clock_gettime_retspec.tv_nsec = 500;
  expect_ts = clock_gettime_retspec;
  cras_iodev_get_valid_frames_ret = 960;
  rstream1.cb_threshold = 480;
  expect_ts.tv_nsec += 10 * 1000000;
  thread_add_stream(thread_, &rstream1, &piodev, 1);
  dev_stream = iodev.streams;
  EXPECT_EQ(dev_stream->stream, &rstream1);
  EXPECT_EQ(init_cb_ts_.tv_sec, expect_ts.tv_sec);
  EXPECT_EQ(init_cb_ts_.tv_nsec, expect_ts.tv_nsec);

  /*
   * Add a new stream when there are other streams exist. init_cb_ts should
   * be the earliest next callback time from other streams.
   */
  rstream1.next_cb_ts = expect_ts;
  thread_add_stream(thread_, &rstream2, &piodev, 1);
  dev_stream = iodev.streams->prev;
  EXPECT_EQ(dev_stream->stream, &rstream2);
  EXPECT_EQ(init_cb_ts_.tv_sec, expect_ts.tv_sec);
  EXPECT_EQ(init_cb_ts_.tv_nsec, expect_ts.tv_nsec);

  thread_rm_open_dev(thread_, CRAS_STREAM_OUTPUT, iodev.info.idx);
  TearDownRstream(&rstream1);
  TearDownRstream(&rstream2);
}

TEST_F(StreamDeviceSuite, AddRemoveMultipleStreamsOnMultipleDevices) {
  struct cras_iodev iodev, *piodev = &iodev;
  struct cras_iodev iodev2, *piodev2 = &iodev2;
  struct cras_rstream rstream;
  struct cras_rstream rstream2;
  struct cras_rstream rstream3;
  struct dev_stream* dev_stream;

  SetupDevice(&iodev, CRAS_STREAM_OUTPUT);
  SetupDevice(&iodev2, CRAS_STREAM_OUTPUT);
  SetupRstream(&rstream, CRAS_STREAM_OUTPUT);
  SetupRstream(&rstream2, CRAS_STREAM_OUTPUT);
  SetupRstream(&rstream3, CRAS_STREAM_OUTPUT);

  // Add first device as open and check 2 streams can be added.
  thread_add_open_dev(thread_, &iodev);
  thread_add_stream(thread_, &rstream, &piodev, 1);
  dev_stream = iodev.streams;
  EXPECT_EQ(dev_stream->stream, &rstream);
  thread_add_stream(thread_, &rstream2, &piodev, 1);
  EXPECT_EQ(dev_stream->next->stream, &rstream2);

  // Add second device as open and check no streams are copied over.
  thread_add_open_dev(thread_, &iodev2);
  dev_stream = iodev2.streams;
  EXPECT_EQ(NULL, dev_stream);
  // Also check the 2 streams on first device remain intact.
  dev_stream = iodev.streams;
  EXPECT_EQ(dev_stream->stream, &rstream);
  EXPECT_EQ(dev_stream->next->stream, &rstream2);

  // Add a stream to the second dev and check it isn't also added to the first.
  thread_add_stream(thread_, &rstream3, &piodev2, 1);
  dev_stream = iodev.streams;
  EXPECT_EQ(dev_stream->stream, &rstream);
  EXPECT_EQ(dev_stream->next->stream, &rstream2);
  EXPECT_EQ(NULL, dev_stream->next->next);
  dev_stream = iodev2.streams;
  EXPECT_EQ(&rstream3, dev_stream->stream);
  EXPECT_EQ(NULL, dev_stream->next);

  // Remove first device from open and streams on second device remain
  // intact.
  thread_rm_open_dev(thread_, CRAS_STREAM_OUTPUT, iodev.info.idx);
  dev_stream = iodev2.streams;
  EXPECT_EQ(&rstream3, dev_stream->stream);
  EXPECT_EQ(NULL, dev_stream->next);

  // Remove 2 streams, check the streams are removed from both open devices.
  dev_io_remove_stream(&thread_->open_devs[rstream.direction], &rstream,
                       &iodev);
  dev_io_remove_stream(&thread_->open_devs[rstream3.direction], &rstream3,
                       &iodev2);
  dev_stream = iodev2.streams;
  EXPECT_EQ(NULL, dev_stream);

  // Remove open devices and check stream is on fallback device.
  thread_rm_open_dev(thread_, CRAS_STREAM_OUTPUT, iodev2.info.idx);

  // Add open device, again check it is empty of streams.
  thread_add_open_dev(thread_, &iodev);
  dev_stream = iodev.streams;
  EXPECT_EQ(NULL, dev_stream);

  thread_rm_open_dev(thread_, CRAS_STREAM_OUTPUT, iodev.info.idx);
  TearDownRstream(&rstream);
  TearDownRstream(&rstream2);
  TearDownRstream(&rstream3);
}

TEST_F(StreamDeviceSuite, FetchStreams) {
  struct cras_iodev iodev, *piodev = &iodev;
  struct open_dev* adev;
  struct cras_rstream rstream;
  struct cras_audio_shm_header* shm_header;

  SetupRstream(&rstream, CRAS_STREAM_OUTPUT);
  shm_header = rstream.shm->header;
  ResetGlobalStubData();

  SetupDevice(&iodev, CRAS_STREAM_OUTPUT);

  shm_header->write_buf_idx = 0;

  // Add the device and add the stream.
  thread_add_open_dev(thread_, &iodev);
  thread_add_stream(thread_, &rstream, &piodev, 1);

  adev = thread_->open_devs[CRAS_STREAM_OUTPUT];

  // Assume device is started.
  iodev.state = CRAS_IODEV_STATE_NORMAL_RUN;

  /*
   * If the stream is pending a reply and shm buffer for writing is empty,
   * just skip it.
   */
  cras_rstream_is_pending_reply_ret = 1;
  shm_header->write_offset[0] = 0;
  dev_io_playback_fetch(adev);

  EXPECT_EQ(dev_stream_request_playback_samples_called, 0);
  EXPECT_EQ(dev_stream_update_next_wake_time_called, 0);

  /*
   * If the stream is not pending a reply and shm buffer for writing is full,
   * update next wake up time and skip fetching.
   */
  cras_rstream_is_pending_reply_ret = 0;
  shm_header->write_offset[0] = cras_shm_used_size(rstream.shm);
  dev_io_playback_fetch(adev);
  EXPECT_EQ(dev_stream_request_playback_samples_called, 0);
  EXPECT_EQ(dev_stream_update_next_wake_time_called, 1);

  // If the stream can be fetched, fetch it.
  cras_rstream_is_pending_reply_ret = 0;
  shm_header->write_offset[0] = 0;
  dev_io_playback_fetch(adev);
  EXPECT_EQ(dev_stream_request_playback_samples_called, 1);

  thread_rm_open_dev(thread_, CRAS_STREAM_OUTPUT, iodev.info.idx);
  TearDownRstream(&rstream);
}

TEST_F(StreamDeviceSuite, WriteOutputSamplesPrepareOutputFailed) {
  struct cras_iodev iodev;
  struct open_dev* adev;

  ResetGlobalStubData();

  SetupDevice(&iodev, CRAS_STREAM_OUTPUT);

  // Add the device.
  thread_add_open_dev(thread_, &iodev);
  adev = thread_->open_devs[CRAS_STREAM_OUTPUT];

  // Assume device is started.
  iodev.state = CRAS_IODEV_STATE_NO_STREAM_RUN;
  // Assume device remains in no stream state;
  cras_iodev_prepare_output_before_write_samples_state =
      CRAS_IODEV_STATE_NO_STREAM_RUN;

  // Assume there is an error in prepare_output.
  cras_iodev_prepare_output_before_write_samples_ret = -EINVAL;

  // cras_iodev should handle no stream playback.
  EXPECT_EQ(-EINVAL,
            write_output_samples(&thread_->open_devs[CRAS_STREAM_OUTPUT], adev,
                                 nullptr));

  // cras_iodev_get_output_buffer in audio_thread write_output_samples is not
  // called.
  EXPECT_EQ(0, cras_iodev_get_output_buffer_called);

  thread_rm_open_dev(thread_, CRAS_STREAM_OUTPUT, iodev.info.idx);
}

TEST_F(StreamDeviceSuite, WriteOutputSamplesNoStream) {
  struct cras_iodev iodev;
  struct open_dev* adev;

  ResetGlobalStubData();

  SetupDevice(&iodev, CRAS_STREAM_OUTPUT);

  // Add the device.
  thread_add_open_dev(thread_, &iodev);
  adev = thread_->open_devs[CRAS_STREAM_OUTPUT];

  // Assume device is started.
  iodev.state = CRAS_IODEV_STATE_NO_STREAM_RUN;
  // Assume device remains in no stream state;
  cras_iodev_prepare_output_before_write_samples_state =
      CRAS_IODEV_STATE_NO_STREAM_RUN;

  // cras_iodev should handle no stream playback.
  write_output_samples(&thread_->open_devs[CRAS_STREAM_OUTPUT], adev, nullptr);
  EXPECT_EQ(1, cras_iodev_prepare_output_before_write_samples_called);
  // cras_iodev_get_output_buffer in audio_thread write_output_samples is not
  // called.
  EXPECT_EQ(0, cras_iodev_get_output_buffer_called);

  thread_rm_open_dev(thread_, CRAS_STREAM_OUTPUT, iodev.info.idx);
}

TEST_F(StreamDeviceSuite, WriteOutputSamplesLeaveNoStream) {
  struct cras_iodev iodev;
  struct open_dev* adev;

  ResetGlobalStubData();

  SetupDevice(&iodev, CRAS_STREAM_OUTPUT);

  // Setup the output buffer for device.
  cras_iodev_get_output_buffer_area = cras_audio_area_create(2);

  // Add the device.
  thread_add_open_dev(thread_, &iodev);
  adev = thread_->open_devs[CRAS_STREAM_OUTPUT];

  // Assume device in no stream state.
  iodev.state = CRAS_IODEV_STATE_NO_STREAM_RUN;

  // Assume device remains in no stream state;
  cras_iodev_prepare_output_before_write_samples_state =
      CRAS_IODEV_STATE_NO_STREAM_RUN;

  // cras_iodev should NOT leave no stream state;
  write_output_samples(&thread_->open_devs[CRAS_STREAM_OUTPUT], adev, nullptr);
  EXPECT_EQ(1, cras_iodev_prepare_output_before_write_samples_called);
  // cras_iodev_get_output_buffer in audio_thread write_output_samples is not
  // called.
  EXPECT_EQ(0, cras_iodev_get_output_buffer_called);

  // Assume device leaves no stream state;
  cras_iodev_prepare_output_before_write_samples_state =
      CRAS_IODEV_STATE_NORMAL_RUN;

  // cras_iodev should write samples from streams.
  write_output_samples(&thread_->open_devs[CRAS_STREAM_OUTPUT], adev, nullptr);
  EXPECT_EQ(2, cras_iodev_prepare_output_before_write_samples_called);
  EXPECT_EQ(1, cras_iodev_get_output_buffer_called);

  thread_rm_open_dev(thread_, CRAS_STREAM_OUTPUT, iodev.info.idx);
}

TEST_F(StreamDeviceSuite, MixOutputSamples) {
  struct cras_iodev iodev, *piodev = &iodev;
  struct cras_rstream rstream1;
  struct cras_rstream rstream2;
  struct open_dev* adev;
  struct dev_stream* dev_stream;

  ResetGlobalStubData();

  SetupDevice(&iodev, CRAS_STREAM_OUTPUT);
  SetupRstream(&rstream1, CRAS_STREAM_OUTPUT);
  SetupRstream(&rstream2, CRAS_STREAM_OUTPUT);

  // Setup the output buffer for device.
  cras_iodev_get_output_buffer_area = cras_audio_area_create(2);

  // Add the device and add the stream.
  thread_add_open_dev(thread_, &iodev);
  thread_add_stream(thread_, &rstream1, &piodev, 1);
  adev = thread_->open_devs[CRAS_STREAM_OUTPUT];

  // Assume device is running.
  iodev.state = CRAS_IODEV_STATE_NORMAL_RUN;

  // Assume device in normal run stream state.
  cras_iodev_prepare_output_before_write_samples_state =
      CRAS_IODEV_STATE_NORMAL_RUN;

  // cras_iodev should not mix samples because the stream has not started
  // running.
  frames_queued_ = 0;
  dev_stream_playback_frames_ret = 100;
  dev_stream = iodev.streams;
  write_output_samples(&thread_->open_devs[CRAS_STREAM_OUTPUT], adev, nullptr);
  EXPECT_EQ(1, cras_iodev_prepare_output_before_write_samples_called);
  EXPECT_EQ(1, cras_iodev_get_output_buffer_called);
  EXPECT_EQ(0, dev_stream_mix_called);

  // Set rstream1 to be running. cras_iodev should mix samples from rstream1.
  dev_stream_set_running(dev_stream);
  write_output_samples(&thread_->open_devs[CRAS_STREAM_OUTPUT], adev, nullptr);
  EXPECT_EQ(2, cras_iodev_prepare_output_before_write_samples_called);
  EXPECT_EQ(2, cras_iodev_get_output_buffer_called);
  EXPECT_EQ(1, dev_stream_mix_called);

  // Add rstream2. cras_iodev should mix samples from rstream1 but not from
  // rstream2.
  thread_add_stream(thread_, &rstream2, &piodev, 1);
  write_output_samples(&thread_->open_devs[CRAS_STREAM_OUTPUT], adev, nullptr);
  EXPECT_EQ(3, cras_iodev_prepare_output_before_write_samples_called);
  EXPECT_EQ(3, cras_iodev_get_output_buffer_called);
  EXPECT_EQ(2, dev_stream_mix_called);

  // Set rstream2 to be running. cras_iodev should mix samples from rstream1
  // and rstream2.
  dev_stream = iodev.streams->prev;
  dev_stream_set_running(dev_stream);
  write_output_samples(&thread_->open_devs[CRAS_STREAM_OUTPUT], adev, nullptr);
  EXPECT_EQ(4, cras_iodev_prepare_output_before_write_samples_called);
  EXPECT_EQ(4, cras_iodev_get_output_buffer_called);
  EXPECT_EQ(4, dev_stream_mix_called);

  thread_rm_open_dev(thread_, CRAS_STREAM_OUTPUT, iodev.info.idx);
  TearDownRstream(&rstream1);
  TearDownRstream(&rstream2);
}

TEST_F(StreamDeviceSuite, DoPlaybackNoStream) {
  struct cras_iodev iodev;

  ResetGlobalStubData();

  SetupDevice(&iodev, CRAS_STREAM_OUTPUT);

  // Add the device.
  thread_add_open_dev(thread_, &iodev);

  // Assume device is started.
  iodev.state = CRAS_IODEV_STATE_NO_STREAM_RUN;
  // Assume device remains in no stream state;
  cras_iodev_prepare_output_before_write_samples_state =
      CRAS_IODEV_STATE_NO_STREAM_RUN;
  // Add 10 frames in queue to prevent underrun
  frames_queued_ = 10;

  // cras_iodev should handle no stream playback.
  dev_io_playback_write(&thread_->open_devs[CRAS_STREAM_OUTPUT], nullptr);
  EXPECT_EQ(1, cras_iodev_prepare_output_before_write_samples_called);
  // cras_iodev_get_output_buffer in audio_thread write_output_samples is not
  // called.
  EXPECT_EQ(0, cras_iodev_get_output_buffer_called);

  EXPECT_EQ(0, cras_iodev_output_underrun_called);
  // cras_iodev_frames_to_play_in_sleep should be called from
  // update_dev_wakeup_time.
  EXPECT_EQ(1, cras_iodev_frames_to_play_in_sleep_called);

  thread_rm_open_dev(thread_, CRAS_STREAM_OUTPUT, iodev.info.idx);
}

TEST_F(StreamDeviceSuite, DoPlaybackUnderrun) {
  struct cras_iodev iodev, *piodev = &iodev;
  struct cras_rstream rstream;

  ResetGlobalStubData();

  SetupDevice(&iodev, CRAS_STREAM_OUTPUT);
  SetupRstream(&rstream, CRAS_STREAM_OUTPUT);

  // Setup the output buffer for device.
  cras_iodev_get_output_buffer_area = cras_audio_area_create(2);

  // Add the device and add the stream.
  thread_add_open_dev(thread_, &iodev);
  thread_add_stream(thread_, &rstream, &piodev, 1);

  // Assume device is running and there is an underrun.
  // It wrote 11 frames into device but new hw_level is only 10.
  // It means underrun may happened because 10 - 11 < 0.
  // Audio thread should ask iodev to handle output underrun.
  iodev.state = CRAS_IODEV_STATE_NORMAL_RUN;
  frames_queued_ = 10;
  cras_iodev_all_streams_written_ret = 11;

  // Assume device in normal run stream state;
  cras_iodev_prepare_output_before_write_samples_state =
      CRAS_IODEV_STATE_NORMAL_RUN;

  EXPECT_EQ(0, cras_iodev_output_underrun_called);
  dev_io_playback_write(&thread_->open_devs[CRAS_STREAM_OUTPUT], nullptr);
  EXPECT_EQ(1, cras_iodev_output_underrun_called);

  thread_rm_open_dev(thread_, CRAS_STREAM_OUTPUT, iodev.info.idx);
  TearDownRstream(&rstream);
}

TEST_F(StreamDeviceSuite, DoPlaybackSevereUnderrun) {
  struct cras_iodev iodev, *piodev = &iodev;
  struct cras_rstream rstream;

  ResetGlobalStubData();

  SetupDevice(&iodev, CRAS_STREAM_OUTPUT);
  SetupRstream(&rstream, CRAS_STREAM_OUTPUT);

  // Setup the output buffer for device.
  cras_iodev_get_output_buffer_area = cras_audio_area_create(2);

  // Add the device and add the stream.
  thread_add_open_dev(thread_, &iodev);
  thread_add_stream(thread_, &rstream, &piodev, 1);

  // Assume device is running and there is a severe underrun.
  cras_audio_thread_event_severe_underrun_called = 0;
  iodev.state = CRAS_IODEV_STATE_NORMAL_RUN;
  frames_queued_ = -EPIPE;

  // Assume device in normal run stream state;
  cras_iodev_prepare_output_before_write_samples_state =
      CRAS_IODEV_STATE_NORMAL_RUN;

  dev_io_playback_write(&thread_->open_devs[CRAS_STREAM_OUTPUT], nullptr);

  // Audio thread should ask main thread to reset device.
  EXPECT_EQ(1, cras_iodev_reset_request_called);
  EXPECT_EQ(&iodev, cras_iodev_reset_request_iodev);
  EXPECT_EQ(1, cras_audio_thread_event_severe_underrun_called);

  thread_rm_open_dev(thread_, CRAS_STREAM_OUTPUT, iodev.info.idx);
  TearDownRstream(&rstream);
}

TEST(AudioThreadStreams, DrainStream) {
  struct cras_rstream rstream;
  struct cras_audio_shm_header* shm_header;
  struct audio_thread thread;

  SetupRstream(&rstream, CRAS_STREAM_OUTPUT);
  shm_header = rstream.shm->header;

  shm_header->write_offset[0] = 1 * 4;
  EXPECT_EQ(1, thread_drain_stream_ms_remaining(&thread, &rstream));

  shm_header->write_offset[0] = 479 * 4;
  EXPECT_EQ(10, thread_drain_stream_ms_remaining(&thread, &rstream));

  shm_header->write_offset[0] = 0;
  EXPECT_EQ(0, thread_drain_stream_ms_remaining(&thread, &rstream));

  rstream.direction = CRAS_STREAM_INPUT;
  shm_header->write_offset[0] = 479 * 4;
  EXPECT_EQ(0, thread_drain_stream_ms_remaining(&thread, &rstream));
  TearDownRstream(&rstream);
}

TEST(BusyloopDetectSuite, CheckerTest) {
  continuous_zero_sleep_count = 0;
  cras_audio_thread_event_busyloop_called = 0;
  timespec wait_ts;
  wait_ts.tv_sec = 0;
  wait_ts.tv_nsec = 0;

  check_busyloop(&wait_ts);
  EXPECT_EQ(continuous_zero_sleep_count, 1);
  EXPECT_EQ(cras_audio_thread_event_busyloop_called, 0);
  check_busyloop(&wait_ts);
  EXPECT_EQ(continuous_zero_sleep_count, 2);
  EXPECT_EQ(cras_audio_thread_event_busyloop_called, 1);
  check_busyloop(&wait_ts);
  EXPECT_EQ(continuous_zero_sleep_count, 3);
  EXPECT_EQ(cras_audio_thread_event_busyloop_called, 1);

  wait_ts.tv_sec = 1;
  check_busyloop(&wait_ts);
  EXPECT_EQ(continuous_zero_sleep_count, 0);
  EXPECT_EQ(cras_audio_thread_event_busyloop_called, 1);
}

extern "C" {

int cras_iodev_add_stream(struct cras_iodev* iodev, struct dev_stream* stream) {
  DL_APPEND(iodev->streams, stream);
  return 0;
}

void cras_iodev_start_stream(struct cras_iodev* iodev,
                             struct dev_stream* stream) {
  dev_stream_set_running(stream);
  cras_iodev_start_stream_called++;
}

unsigned int cras_iodev_all_streams_written(struct cras_iodev* iodev) {
  return cras_iodev_all_streams_written_ret;
}

int cras_iodev_close(struct cras_iodev* iodev) {
  return 0;
}

void cras_iodev_free_format(struct cras_iodev* iodev) {
  return;
}

double cras_iodev_get_est_rate_ratio(const struct cras_iodev* iodev) {
  return 1.0;
}

unsigned int cras_iodev_max_stream_offset(const struct cras_iodev* iodev) {
  return 0;
}

int cras_iodev_open(struct cras_iodev* iodev,
                    unsigned int cb_level,
                    const struct cras_audio_format* fmt) {
  return 0;
}

int cras_iodev_put_buffer(struct cras_iodev* iodev, unsigned int nframes) {
  return 0;
}

struct dev_stream* cras_iodev_rm_stream(struct cras_iodev* iodev,
                                        const struct cras_rstream* stream) {
  struct dev_stream* out;
  DL_FOREACH (iodev->streams, out) {
    if (out->stream == stream) {
      DL_DELETE(iodev->streams, out);
      return out;
    }
  }
  return NULL;
}

int cras_iodev_set_format(struct cras_iodev* iodev,
                          const struct cras_audio_format* fmt) {
  return 0;
}

unsigned int cras_iodev_stream_offset(struct cras_iodev* iodev,
                                      struct dev_stream* stream) {
  return 0;
}

int cras_iodev_is_zero_volume(const struct cras_iodev* iodev) {
  return cras_iodev_is_zero_volume_ret;
}

int dev_stream_attached_devs(const struct dev_stream* dev_stream) {
  return 1;
}

void cras_iodev_stream_written(struct cras_iodev* iodev,
                               struct dev_stream* stream,
                               unsigned int nwritten) {}

int cras_iodev_update_rate(struct cras_iodev* iodev,
                           unsigned int level,
                           struct timespec* level_tstamp) {
  return 0;
}

int cras_iodev_put_input_buffer(struct cras_iodev* iodev) {
  return 0;
}

int cras_iodev_put_output_buffer(struct cras_iodev* iodev,
                                 uint8_t* frames,
                                 unsigned int nframes,
                                 int* non_empty,
                                 struct cras_fmt_conv* output_converter) {
  cras_iodev_put_output_buffer_called++;
  cras_iodev_put_output_buffer_nframes = nframes;
  return 0;
}

int cras_iodev_get_input_buffer(struct cras_iodev* iodev, unsigned* frames) {
  return 0;
}

int cras_iodev_get_output_buffer(struct cras_iodev* iodev,
                                 struct cras_audio_area** area,
                                 unsigned* frames) {
  cras_iodev_get_output_buffer_called++;
  *area = cras_iodev_get_output_buffer_area;
  return 0;
}

int cras_iodev_get_dsp_delay(const struct cras_iodev* iodev) {
  return 0;
}

void cras_fmt_conv_destroy(struct cras_fmt_conv** conv) {}

struct cras_fmt_conv* cras_channel_remix_conv_create(unsigned int num_channels,
                                                     const float* coefficient) {
  return NULL;
}

void cras_rstream_dev_attach(struct cras_rstream* rstream,
                             unsigned int dev_id,
                             void* dev_ptr) {}

void cras_rstream_dev_detach(struct cras_rstream* rstream,
                             unsigned int dev_id) {}

void cras_rstream_destroy(struct cras_rstream* stream) {}

void cras_rstream_dev_offset_update(struct cras_rstream* rstream,
                                    unsigned int frames,
                                    unsigned int dev_id) {
  int i = cras_rstream_dev_offset_update_called;
  if (i < MAX_CALLS) {
    cras_rstream_dev_offset_update_rstream_val[i] = rstream;
    cras_rstream_dev_offset_update_frames_val[i] = frames;
    cras_rstream_dev_offset_update_dev_id_val[i] = dev_id;
    cras_rstream_dev_offset_update_called++;
  }
}

unsigned int cras_rstream_dev_offset(const struct cras_rstream* rstream,
                                     unsigned int dev_id) {
  int i = cras_rstream_dev_offset_called;
  if (i < MAX_CALLS) {
    cras_rstream_dev_offset_rstream_val[i] = rstream;
    cras_rstream_dev_offset_dev_id_val[i] = dev_id;
    cras_rstream_dev_offset_called++;
    return cras_rstream_dev_offset_ret[i];
  }
  return 0;
}

void cras_rstream_record_fetch_interval(struct cras_rstream* rstream,
                                        const struct timespec* now) {}

int cras_rstream_is_pending_reply(const struct cras_rstream* stream) {
  return cras_rstream_is_pending_reply_ret;
}

float cras_rstream_get_volume_scaler(struct cras_rstream* rstream) {
  return 1.0f;
}

int cras_set_rt_scheduling(int rt_lim) {
  return 0;
}

int cras_set_thread_priority(int priority) {
  return 0;
}

void cras_system_rm_select_fd(int fd) {}

unsigned int dev_stream_capture(struct dev_stream* dev_stream,
                                const struct cras_audio_area* area,
                                unsigned int area_offset,
                                float software_gain_scaler) {
  return 0;
}

unsigned int dev_stream_capture_avail(const struct dev_stream* dev_stream) {
  return 0;
}
unsigned int dev_stream_cb_threshold(const struct dev_stream* dev_stream) {
  return 0;
}

int dev_stream_capture_update_rstream(struct dev_stream* dev_stream) {
  return 0;
}

struct dev_stream* dev_stream_create(struct cras_rstream* stream,
                                     unsigned int dev_id,
                                     const struct cras_audio_format* dev_fmt,
                                     struct cras_iodev* iodev,
                                     struct timespec* cb_ts,
                                     const struct timespec* sleep_interval_ts) {
  struct dev_stream* out = static_cast<dev_stream*>(calloc(1, sizeof(*out)));
  out->stream = stream;
  init_cb_ts_ = *cb_ts;
  if (sleep_interval_ts) {
    sleep_interval_ts_ = *sleep_interval_ts;
  } else {
    sleep_interval_ts_ = {.tv_sec = -1, .tv_nsec = -1};
  }
  return out;
}

void dev_stream_destroy(struct dev_stream* dev_stream) {
  free(dev_stream);
}

int dev_stream_mix(struct dev_stream* dev_stream,
                   const struct cras_audio_format* fmt,
                   uint8_t* dst,
                   unsigned int num_to_write) {
  dev_stream_mix_called++;
  return num_to_write;
}

int dev_stream_playback_frames(const struct dev_stream* dev_stream) {
  return dev_stream_playback_frames_ret;
}

int dev_stream_playback_update_rstream(struct dev_stream* dev_stream) {
  return 0;
}

int dev_stream_poll_stream_fd(const struct dev_stream* dev_stream) {
  return dev_stream->stream->fd;
}

int dev_stream_request_playback_samples(struct dev_stream* dev_stream,
                                        const struct timespec* now) {
  dev_stream_request_playback_samples_called++;
  return 0;
}

void dev_stream_set_delay(const struct dev_stream* dev_stream,
                          unsigned int delay_frames) {}

void dev_stream_set_dev_rate(struct dev_stream* dev_stream,
                             unsigned int dev_rate,
                             double dev_rate_ratio,
                             double main_rate_ratio,
                             int coarse_rate_adjust) {}

void dev_stream_update_frames(const struct dev_stream* dev_stream) {}

void dev_stream_update_next_wake_time(struct dev_stream* dev_stream) {
  dev_stream_update_next_wake_time_called++;
}

int dev_stream_wake_time(struct dev_stream* dev_stream,
                         unsigned int curr_level,
                         struct timespec* level_tstamp,
                         unsigned int cap_limit,
                         int is_cap_limit_stream,
                         struct timespec* wake_time) {
  if (dev_stream_wake_time_val.find(dev_stream) !=
      dev_stream_wake_time_val.end()) {
    wake_time->tv_sec = dev_stream_wake_time_val[dev_stream].tv_sec;
    wake_time->tv_nsec = dev_stream_wake_time_val[dev_stream].tv_nsec;
  }
  return 0;
}

int dev_stream_is_pending_reply(const struct dev_stream* dev_stream) {
  return 0;
}

int dev_stream_flush_old_audio_messages(struct dev_stream* dev_stream) {
  return 0;
}

int cras_iodev_frames_queued(struct cras_iodev* iodev,
                             struct timespec* tstamp) {
  return iodev->frames_queued(iodev, tstamp);
}

int cras_iodev_buffer_avail(struct cras_iodev* iodev, unsigned hw_level) {
  struct timespec tstamp;
  return iodev->buffer_size - iodev->frames_queued(iodev, &tstamp);
}

int cras_iodev_fill_odev_zeros(struct cras_iodev* odev,
                               unsigned int frames,
                               bool underrun) {
  cras_iodev_fill_odev_zeros_frames = frames;
  return 0;
}

int cras_iodev_output_underrun(struct cras_iodev* odev,
                               unsigned int hw_level,
                               unsigned int frames_written) {
  cras_iodev_output_underrun_called++;
  return 0;
}

int cras_iodev_prepare_output_before_write_samples(struct cras_iodev* odev) {
  cras_iodev_prepare_output_before_write_samples_called++;
  odev->state = cras_iodev_prepare_output_before_write_samples_state;
  return cras_iodev_prepare_output_before_write_samples_ret;
}

float cras_iodev_get_software_gain_scaler(const struct cras_iodev* iodev) {
  return 1.0f;
}

unsigned int cras_iodev_frames_to_play_in_sleep(struct cras_iodev* odev,
                                                unsigned int* hw_level,
                                                struct timespec* hw_tstamp) {
  *hw_level = cras_iodev_frames_queued(odev, hw_tstamp);
  cras_iodev_frames_to_play_in_sleep_called++;
  return 0;
}

int cras_iodev_odev_should_wake(const struct cras_iodev* odev) {
  return 1;
}

struct cras_audio_area* cras_audio_area_create(int num_channels) {
  struct cras_audio_area* area;
  size_t sz;

  sz = sizeof(*area) + num_channels * sizeof(struct cras_channel_area);
  area = (cras_audio_area*)calloc(1, sz);
  area->num_channels = num_channels;
  area->channels[0].buf = (uint8_t*)calloc(1, BUFFER_SIZE * 2 * num_channels);

  return area;
}

enum CRAS_IODEV_STATE cras_iodev_state(const struct cras_iodev* iodev) {
  return iodev->state;
}

unsigned int cras_iodev_get_num_underruns(const struct cras_iodev* iodev) {
  return 0;
}

int cras_iodev_get_valid_frames(struct cras_iodev* iodev,
                                struct timespec* hw_tstamp) {
  clock_gettime(CLOCK_MONOTONIC_RAW, hw_tstamp);
  return cras_iodev_get_valid_frames_ret;
}

int cras_iodev_reset_request(struct cras_iodev* iodev) {
  cras_iodev_reset_request_called++;
  cras_iodev_reset_request_iodev = iodev;
  return 0;
}

unsigned int cras_iodev_get_num_severe_underruns(
    const struct cras_iodev* iodev) {
  return 0;
}

void cras_iodev_update_highest_hw_level(struct cras_iodev* iodev,
                                        unsigned int hw_level) {}

int cras_iodev_start_ramp(struct cras_iodev* odev,
                          enum CRAS_IODEV_RAMP_REQUEST request) {
  cras_iodev_start_ramp_odev = odev;
  cras_iodev_start_ramp_request = request;
  return 0;
}

int input_data_get_for_stream(struct input_data* data,
                              struct cras_rstream* stream,
                              struct buffer_share* offsets,
                              float preprocessing_gain_scalar,
                              struct cras_audio_area** area,
                              unsigned int* offset) {
  return 0;
}

int input_data_put_for_stream(struct input_data* data,
                              struct cras_rstream* stream,
                              struct buffer_share* offsets,
                              unsigned int frames) {
  return 0;
}

int cras_device_monitor_set_device_mute_state(unsigned int dev_idx) {
  cras_device_monitor_set_device_mute_state_called++;
  return 0;
}
int cras_device_monitor_error_close(unsigned int dev_idx) {
  return 0;
}

int cras_iodev_drop_frames_by_time(struct cras_iodev* iodev,
                                   struct timespec ts) {
  return 0;
}

bool cras_iodev_is_on_internal_card(const struct cras_ionode* node) {
  return 0;
}

//  From librt.
int clock_gettime(clockid_t clk_id, struct timespec* tp) {
  *tp = clock_gettime_retspec;
  return 0;
}

uint64_t cras_stream_apm_get_effects(struct cras_stream_apm* stream) {
  return 0;
}

void cras_stream_apm_set_aec_dump(struct cras_stream_apm* stream,
                                  const struct cras_iodev* idev,
                                  int start,
                                  int fd) {}

int cras_audio_thread_event_busyloop() {
  cras_audio_thread_event_busyloop_called++;
  return 0;
}

int cras_audio_thread_event_drop_samples() {
  return 0;
}

int cras_audio_thread_event_severe_underrun() {
  cras_audio_thread_event_severe_underrun_called++;
  return 0;
}

struct input_data_gain input_data_get_software_gain_scaler(
    struct input_data* data,
    float ui_gain_scalar,
    float idev_sw_gain_scaler,
    struct cras_rstream* stream) {
  return input_data_gain{
      .preprocessing_scalar = 1,
      .postprocessing_scalar = 1,
  };
}
}  // extern "C"
