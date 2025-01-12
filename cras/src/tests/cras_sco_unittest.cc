/* Copyright 2013 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <stdint.h>
#include <time.h>

using testing::MatchesRegex;
using testing::internal::CaptureStdout;
using testing::internal::GetCapturedStdout;

extern "C" {
#include "cras/src/server/cras_bt_policy.h"
#include "cras/src/server/cras_sco.c"
#include "cras/src/tests/sbc_codec_stub.h"
#include "cras/src/tests/sr_bt_util_stub.h"
#include "cras/src/tests/sr_stub.h"
}
static struct cras_sco* sco;
static struct cras_iodev dev;
static cras_audio_format format;

static int cras_msbc_plc_create_called;
static int cras_msbc_plc_handle_good_frames_called;
static int cras_msbc_plc_handle_bad_frames_called;

static thread_callback thread_cb;
static void* cb_data;
static timespec ts;

static struct cras_bt_device* fake_device;

void ResetStubData() {
  sbc_codec_stub_reset();
  cras_msbc_plc_create_called = 0;
  cras_msbc_plc_handle_good_frames_called = 0;
  cras_msbc_plc_handle_bad_frames_called = 0;

  format.format = SND_PCM_FORMAT_S16_LE;
  format.num_channels = 1;
  format.frame_rate = 8000;
  dev.format = &format;

  fake_device = reinterpret_cast<struct cras_bt_device*>(0x123);
}

namespace {

TEST(CrasSco, AddRmDev) {
  ResetStubData();

  sco = cras_sco_create(fake_device);
  ASSERT_NE(sco, (void*)NULL);
  dev.direction = CRAS_STREAM_OUTPUT;

  // Test add dev
  ASSERT_EQ(0, cras_sco_add_iodev(sco, dev.direction, dev.format));
  ASSERT_TRUE(cras_sco_has_iodev(sco));

  // Test remove dev
  ASSERT_EQ(0, cras_sco_rm_iodev(sco, dev.direction));
  ASSERT_FALSE(cras_sco_has_iodev(sco));

  cras_sco_destroy(sco);
}

TEST(CrasSco, AddRmDevInvalid) {
  ResetStubData();

  sco = cras_sco_create(fake_device);
  ASSERT_NE(sco, (void*)NULL);

  dev.direction = CRAS_STREAM_OUTPUT;

  // Remove an iodev which doesn't exist
  ASSERT_NE(0, cras_sco_rm_iodev(sco, dev.direction));

  // Adding an iodev twice returns error code
  ASSERT_EQ(0, cras_sco_add_iodev(sco, dev.direction, dev.format));
  ASSERT_NE(0, cras_sco_add_iodev(sco, dev.direction, dev.format));

  cras_sco_destroy(sco);
}

TEST(CrasSco, AcquirePlaybackBuffer) {
  unsigned buffer_frames, buffer_frames2, queued;
  uint8_t* samples;
  int sock[2];

  ResetStubData();

  ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sock));

  sco = cras_sco_create(fake_device);
  ASSERT_NE(sco, (void*)NULL);

  cras_sco_set_fd(sco, sock[1]);
  cras_sco_start(48, HFP_CODEC_ID_CVSD, sco);
  dev.direction = CRAS_STREAM_OUTPUT;
  ASSERT_EQ(0, cras_sco_add_iodev(sco, dev.direction, dev.format));

  buffer_frames = 500;
  cras_sco_buf_acquire(sco, dev.direction, &samples, &buffer_frames);
  ASSERT_EQ(500, buffer_frames);

  cras_sco_buf_release(sco, dev.direction, 500);
  ASSERT_EQ(500, cras_sco_buf_queued(sco, dev.direction));

  /* Assert the amount of frames of available buffer + queued buf is
   * greater than or equal to the buffer size, 2 bytes per frame
   */
  queued = cras_sco_buf_queued(sco, dev.direction);
  buffer_frames = 500;
  cras_sco_buf_acquire(sco, dev.direction, &samples, &buffer_frames);
  ASSERT_GE(sco->playback_buf->used_size / 2, buffer_frames + queued);

  // Consume all queued data from read buffer
  buf_increment_read(sco->playback_buf, queued * 2);

  queued = cras_sco_buf_queued(sco, dev.direction);
  ASSERT_EQ(0, queued);

  // Assert consecutive acquire buffer will acquire full used size of buffer
  buffer_frames = 500;
  cras_sco_buf_acquire(sco, dev.direction, &samples, &buffer_frames);
  cras_sco_buf_release(sco, dev.direction, buffer_frames);

  buffer_frames2 = 500;
  cras_sco_buf_acquire(sco, dev.direction, &samples, &buffer_frames2);
  cras_sco_buf_release(sco, dev.direction, buffer_frames2);

  ASSERT_GE(sco->playback_buf->used_size / 2, buffer_frames + buffer_frames2);

  cras_sco_stop(sco);
  cras_sco_close_fd(sco);
  cras_sco_destroy(sco);
}

TEST(CrasSco, AcquireCaptureBuffer) {
  unsigned buffer_frames, buffer_frames2;
  uint8_t* samples;
  int sock[2];

  ResetStubData();

  ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sock));
  sco = cras_sco_create(fake_device);
  ASSERT_NE(sco, (void*)NULL);

  cras_sco_set_fd(sco, sock[1]);
  cras_sco_start(48, HFP_CODEC_ID_CVSD, sco);
  dev.direction = CRAS_STREAM_INPUT;
  ASSERT_EQ(0, cras_sco_add_iodev(sco, dev.direction, dev.format));

  // Put fake data 100 bytes(50 frames) in capture buf for test
  buf_increment_write(sco->capture_buf, 100);

  // Assert successfully acquire and release 100 bytes of data
  buffer_frames = 50;
  cras_sco_buf_acquire(sco, dev.direction, &samples, &buffer_frames);
  ASSERT_EQ(50, buffer_frames);

  cras_sco_buf_release(sco, dev.direction, buffer_frames);
  ASSERT_EQ(0, cras_sco_buf_queued(sco, dev.direction));

  // Push fake data to capture buffer
  buf_increment_write(sco->capture_buf, sco->capture_buf->used_size - 100);
  buf_increment_write(sco->capture_buf, 100);

  // Assert consecutive acquire call will consume the whole buffer
  buffer_frames = 1000;
  cras_sco_buf_acquire(sco, dev.direction, &samples, &buffer_frames);
  cras_sco_buf_release(sco, dev.direction, buffer_frames);
  ASSERT_GE(1000, buffer_frames);

  buffer_frames2 = 1000;
  cras_sco_buf_acquire(sco, dev.direction, &samples, &buffer_frames2);
  cras_sco_buf_release(sco, dev.direction, buffer_frames2);

  ASSERT_GE(sco->capture_buf->used_size / 2, buffer_frames + buffer_frames2);

  cras_sco_stop(sco);
  cras_sco_close_fd(sco);
  cras_sco_destroy(sco);
}

TEST(CrasSco, HfpReadWriteFD) {
  int rc;
  int sock[2];
  uint8_t sample[480];
  uint8_t* buf;
  unsigned buffer_count;

  ResetStubData();

  ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sock));

  sco = cras_sco_create(fake_device);
  ASSERT_NE(sco, (void*)NULL);

  dev.direction = CRAS_STREAM_INPUT;
  cras_sco_set_fd(sco, sock[1]);
  cras_sco_start(48, HFP_CODEC_ID_CVSD, sco);
  ASSERT_EQ(0, cras_sco_add_iodev(sco, dev.direction, dev.format));

  // Mock the sco fd and send some fake data
  send(sock[0], sample, 48, 0);

  rc = sco_read(sco);
  ASSERT_EQ(48, rc);

  rc = cras_sco_buf_queued(sco, dev.direction);
  ASSERT_EQ(48 / 2, rc);

  // Fill the write buffer
  buffer_count = sco->capture_buf->used_size;
  buf = buf_write_pointer_size(sco->capture_buf, &buffer_count);
  buf_increment_write(sco->capture_buf, buffer_count);
  ASSERT_NE((void*)NULL, buf);

  rc = sco_read(sco);
  ASSERT_EQ(0, rc);

  ASSERT_EQ(0, cras_sco_rm_iodev(sco, dev.direction));
  dev.direction = CRAS_STREAM_OUTPUT;
  ASSERT_EQ(0, cras_sco_add_iodev(sco, dev.direction, dev.format));

  // Initial buffer is empty
  rc = sco_write(sco);
  ASSERT_EQ(0, rc);

  buffer_count = 1024;
  buf = buf_write_pointer_size(sco->playback_buf, &buffer_count);
  buf_increment_write(sco->playback_buf, buffer_count);

  rc = sco_write(sco);
  ASSERT_EQ(48, rc);

  rc = recv(sock[0], sample, 48, 0);
  ASSERT_EQ(48, rc);

  cras_sco_close_fd(sco);
  cras_sco_destroy(sco);
}

TEST(CrasSco, StartCrasSco) {
  int sock[2];

  ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sock));

  sco = cras_sco_create(fake_device);
  ASSERT_NE(sco, (void*)NULL);

  cras_sco_set_fd(sco, sock[0]);
  cras_sco_start(48, HFP_CODEC_ID_CVSD, sco);
  ASSERT_EQ(1, cras_sco_running(sco));
  ASSERT_EQ(cb_data, (void*)sco);

  cras_sco_stop(sco);
  ASSERT_EQ(0, cras_sco_running(sco));
  ASSERT_EQ(NULL, cb_data);

  cras_sco_close_fd(sco);
  cras_sco_destroy(sco);
}

TEST(CrasSco, StartCrasScoAndRead) {
  int rc;
  int sock[2];
  uint8_t sample[480];

  ResetStubData();

  ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sock));

  sco = cras_sco_create(fake_device);
  ASSERT_NE(sco, (void*)NULL);

  // Start and send two chunk of fake data
  cras_sco_set_fd(sco, sock[1]);
  cras_sco_start(48, HFP_CODEC_ID_CVSD, sco);
  send(sock[0], sample, 48, 0);
  send(sock[0], sample, 48, 0);

  // Trigger thread callback
  thread_cb((struct cras_sco*)cb_data, POLLIN);

  dev.direction = CRAS_STREAM_INPUT;
  ASSERT_EQ(0, cras_sco_add_iodev(sco, dev.direction, dev.format));

  // Expect no data read, since no idev present at previous thread callback
  rc = cras_sco_buf_queued(sco, dev.direction);
  ASSERT_EQ(0, rc);

  // Trigger thread callback after idev added.
  ts.tv_sec = 0;
  ts.tv_nsec = 5000000;
  thread_cb((struct cras_sco*)cb_data, POLLIN);

  rc = cras_sco_buf_queued(sco, dev.direction);
  ASSERT_EQ(48 / 2, rc);

  // Assert wait time is unchanged.
  ASSERT_EQ(0, ts.tv_sec);
  ASSERT_EQ(5000000, ts.tv_nsec);

  cras_sco_stop(sco);
  ASSERT_EQ(0, cras_sco_running(sco));

  cras_sco_close_fd(sco);
  cras_sco_destroy(sco);
}

TEST(CrasSco, StartCrasScoAndWrite) {
  int rc;
  int sock[2];
  uint8_t sample[480];

  ResetStubData();

  ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sock));

  sco = cras_sco_create(fake_device);
  ASSERT_NE(sco, (void*)NULL);

  cras_sco_set_fd(sco, sock[1]);
  cras_sco_start(48, HFP_CODEC_ID_CVSD, sco);
  send(sock[0], sample, 48, 0);
  send(sock[0], sample, 48, 0);

  // Trigger thread callback
  thread_cb((struct cras_sco*)cb_data, POLLIN);

  // Without odev in presence, zero packet should be sent.
  rc = recv(sock[0], sample, 48, 0);
  ASSERT_EQ(48, rc);

  dev.direction = CRAS_STREAM_OUTPUT;
  ASSERT_EQ(0, cras_sco_add_iodev(sco, dev.direction, dev.format));

  // Assert queued samples unchanged before output device added
  ASSERT_EQ(0, cras_sco_buf_queued(sco, dev.direction));

  // Put some fake data and trigger thread callback again
  buf_increment_write(sco->playback_buf, 1008);
  thread_cb((struct cras_sco*)cb_data, POLLIN);

  // Assert some samples written
  rc = recv(sock[0], sample, 48, 0);
  ASSERT_EQ(48, rc);
  ASSERT_EQ(480, cras_sco_buf_queued(sco, dev.direction));

  cras_sco_stop(sco);
  cras_sco_close_fd(sco);
  cras_sco_destroy(sco);
}

void send_mSBC_packet(int fd, unsigned seq, int broken_pkt) {
  /* The first three bytes of hci_sco_buf are h2 header, frame count and mSBC
   * sync word. The second octet of H2 header is composed by 4 bits fixed 0x8
   * and 4 bits sequence number 0000, 0011, 1100, 1111.
   */
  uint8_t headers[4] = {0x08, 0x38, 0xc8, 0xf8};
  uint8_t hci_sco_buf[] = {
      0x01, 0x00, 0xAD, 0xad, 0x00, 0x00, 0xc5, 0x00, 0x00, 0x00, 0x00, 0x77,
      0x6d, 0xb6, 0xdd, 0xdb, 0x6d, 0xb7, 0x76, 0xdb, 0x6d, 0xdd, 0xb6, 0xdb,
      0x77, 0x6d, 0xb6, 0xdd, 0xdb, 0x6d, 0xb7, 0x76, 0xdb, 0x6d, 0xdd, 0xb6,
      0xdb, 0x77, 0x6d, 0xb6, 0xdd, 0xdb, 0x6d, 0xb7, 0x76, 0xdb, 0x6d, 0xdd,
      0xb6, 0xdb, 0x77, 0x6d, 0xb6, 0xdd, 0xdb, 0x6d, 0xb7, 0x76, 0xdb, 0x6c};
  struct msghdr msg = {0};
  struct iovec iov;
  struct cmsghdr* cmsg;
  const unsigned int control_size = CMSG_SPACE(sizeof(int));
  char control[control_size] = {0};
  uint8_t pkt_status = 0;

  hci_sco_buf[1] = headers[seq % 4];

  // Assume typical 60 bytes case.
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  iov.iov_base = hci_sco_buf;
  iov.iov_len = 60;
  msg.msg_control = control;
  msg.msg_controllen = control_size;

  if (broken_pkt) {
    pkt_status = 0x11;
  }

  cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_level = SOL_BLUETOOTH;
  cmsg->cmsg_type = BT_SCM_PKT_STATUS;
  cmsg->cmsg_len = CMSG_LEN(sizeof(pkt_status));
  memcpy(CMSG_DATA(cmsg), &pkt_status, sizeof(pkt_status));

  sendmsg(fd, &msg, 0);
}

TEST(CrasSco, StartCrasScoAndReadMsbc) {
  int sock[2];
  int pkt_count = 0;
  int rc;
  uint8_t sample[480];
  ResetStubData();

  ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sock));

  set_sbc_codec_decoded_out(MSBC_CODE_SIZE);

  sco = cras_sco_create(fake_device);
  ASSERT_NE(sco, (void*)NULL);
  ASSERT_EQ(0, get_msbc_codec_create_called());
  ASSERT_EQ(0, cras_msbc_plc_create_called);

  // Start and send an mSBC packets with all zero samples
  cras_sco_set_fd(sco, sock[1]);
  cras_sco_start(63, HFP_CODEC_ID_MSBC, sco);
  ASSERT_EQ(2, get_msbc_codec_create_called());
  ASSERT_EQ(1, cras_msbc_plc_create_called);
  send_mSBC_packet(sock[0], pkt_count++, 0);

  // Trigger thread callback
  thread_cb((struct cras_sco*)cb_data, POLLIN);

  // Expect one empty mSBC packet is send, because no odev in presence.
  rc = recv(sock[0], sample, MSBC_PKT_SIZE, 0);
  ASSERT_EQ(MSBC_PKT_SIZE, rc);

  dev.direction = CRAS_STREAM_INPUT;
  ASSERT_EQ(0, cras_sco_add_iodev(sco, dev.direction, dev.format));

  // Expect no data read, since no idev present at previous thread callback
  ASSERT_EQ(0, cras_sco_buf_queued(sco, dev.direction));

  send_mSBC_packet(sock[0], pkt_count, 0);

  // Trigger thread callback after idev added.
  thread_cb((struct cras_sco*)cb_data, POLLIN);
  rc = recv(sock[0], sample, MSBC_PKT_SIZE, 0);
  ASSERT_EQ(MSBC_PKT_SIZE, rc);

  ASSERT_EQ(pkt_count * MSBC_CODE_SIZE / 2,
            cras_sco_buf_queued(sco, dev.direction));
  ASSERT_EQ(2, cras_msbc_plc_handle_good_frames_called);
  pkt_count++;
  /* When the third packet is lost, we should call the handle_bad_packet and
   * still have right size of samples queued
   */
  pkt_count++;
  send_mSBC_packet(sock[0], pkt_count, 0);
  thread_cb((struct cras_sco*)cb_data, POLLIN);
  rc = recv(sock[0], sample, MSBC_PKT_SIZE, 0);
  ASSERT_EQ(MSBC_PKT_SIZE, rc);

  // Packet 1, 2, 4 are all good frames
  ASSERT_EQ(3, cras_msbc_plc_handle_good_frames_called);
  ASSERT_EQ(1, cras_msbc_plc_handle_bad_frames_called);
  ASSERT_EQ(pkt_count * MSBC_CODE_SIZE / 2,
            cras_sco_buf_queued(sco, dev.direction));
  pkt_count++;
  /* If the erroneous data reporting marks the packet as broken, we should
   * also call the handle_bad_packet and have the right size of samples queued.
   */
  send_mSBC_packet(sock[0], pkt_count, 1);

  set_sbc_codec_decoded_fail(1);

  thread_cb((struct cras_sco*)cb_data, POLLIN);
  rc = recv(sock[0], sample, MSBC_PKT_SIZE, 0);
  ASSERT_EQ(MSBC_PKT_SIZE, rc);

  ASSERT_EQ(3, cras_msbc_plc_handle_good_frames_called);
  ASSERT_EQ(2, cras_msbc_plc_handle_bad_frames_called);
  ASSERT_EQ(pkt_count * MSBC_CODE_SIZE / 2,
            cras_sco_buf_queued(sco, dev.direction));
  pkt_count++;
  /* If we can't decode the packet, we should also call the handle_bad_packet
   * and have the right size of samples queued
   */
  send_mSBC_packet(sock[0], pkt_count, 0);

  set_sbc_codec_decoded_fail(1);

  thread_cb((struct cras_sco*)cb_data, POLLIN);
  rc = recv(sock[0], sample, MSBC_PKT_SIZE, 0);
  ASSERT_EQ(MSBC_PKT_SIZE, rc);

  ASSERT_EQ(3, cras_msbc_plc_handle_good_frames_called);
  ASSERT_EQ(3, cras_msbc_plc_handle_bad_frames_called);
  ASSERT_EQ(pkt_count * MSBC_CODE_SIZE / 2,
            cras_sco_buf_queued(sco, dev.direction));

  cras_sco_stop(sco);
  ASSERT_EQ(0, cras_sco_running(sco));

  cras_sco_close_fd(sco);
  cras_sco_destroy(sco);
}

class CrasScoWithSrBtTestSuite : public testing::Test {
 protected:
  void SetUp() override { enable_cras_sr_bt(); }

  void TearDown() override { disable_cras_sr_bt(); }
};

TEST_F(CrasScoWithSrBtTestSuite, StartCrasScoAndRead) {
  int rc;
  int sock[2];
  uint8_t sample[480];

  ResetStubData();

  ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sock));

  sco = cras_sco_create(fake_device);
  ASSERT_NE(sco, (void*)NULL);
  ASSERT_EQ(cras_sco_enable_cras_sr_bt(sco, SR_BT_NBS), 0);

  // Start and send two chunk of fake data
  cras_sco_set_fd(sco, sock[1]);
  cras_sco_start(48, HFP_CODEC_ID_CVSD, sco);
  send(sock[0], sample, 48, 0);
  send(sock[0], sample, 48, 0);

  // Trigger thread callback
  thread_cb((struct cras_sco*)cb_data, POLLIN);

  dev.direction = CRAS_STREAM_INPUT;
  ASSERT_EQ(0, cras_sco_add_iodev(sco, dev.direction, dev.format));

  // Expect no data read, since no idev present at previous thread callback
  rc = cras_sco_buf_queued(sco, dev.direction);
  ASSERT_EQ(0, rc);

  // Trigger thread callback after idev added.
  ts.tv_sec = 0;
  ts.tv_nsec = 5000000;
  thread_cb((struct cras_sco*)cb_data, POLLIN);

  rc = cras_sco_buf_queued(sco, dev.direction);
  EXPECT_EQ(48 * 3 / 2, rc);

  // Assert wait time is unchanged.
  ASSERT_EQ(0, ts.tv_sec);
  ASSERT_EQ(5000000, ts.tv_nsec);

  cras_sco_stop(sco);
  ASSERT_EQ(0, cras_sco_running(sco));

  cras_sco_close_fd(sco);
  cras_sco_destroy(sco);
}

TEST_F(CrasScoWithSrBtTestSuite, StartCrasScoAndReadMsbc) {
  int sock[2];
  int pkt_count = 0;
  int rc;
  uint8_t sample[480];
  ResetStubData();

  ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sock));

  set_sbc_codec_decoded_out(MSBC_CODE_SIZE);

  sco = cras_sco_create(fake_device);
  ASSERT_NE(sco, (void*)NULL);
  ASSERT_EQ(0, get_msbc_codec_create_called());
  ASSERT_EQ(0, cras_msbc_plc_create_called);
  ASSERT_EQ(cras_sco_enable_cras_sr_bt(sco, SR_BT_WBS), 0);

  // Start and send an mSBC packets with all zero samples
  cras_sco_set_fd(sco, sock[1]);
  cras_sco_start(63, HFP_CODEC_ID_MSBC, sco);
  ASSERT_EQ(2, get_msbc_codec_create_called());
  ASSERT_EQ(1, cras_msbc_plc_create_called);
  send_mSBC_packet(sock[0], pkt_count++, 0);

  // Trigger thread callback
  thread_cb((struct cras_sco*)cb_data, POLLIN);

  // Expect one empty mSBC packet is send, because no odev in presence.
  rc = recv(sock[0], sample, MSBC_PKT_SIZE, 0);
  ASSERT_EQ(MSBC_PKT_SIZE, rc);

  dev.direction = CRAS_STREAM_INPUT;
  ASSERT_EQ(0, cras_sco_add_iodev(sco, dev.direction, dev.format));

  // Expect no data read, since no idev present at previous thread callback
  ASSERT_EQ(0, cras_sco_buf_queued(sco, dev.direction));

  send_mSBC_packet(sock[0], pkt_count, 0);

  // Trigger thread callback after idev added.
  thread_cb((struct cras_sco*)cb_data, POLLIN);
  rc = recv(sock[0], sample, MSBC_PKT_SIZE, 0);
  ASSERT_EQ(MSBC_PKT_SIZE, rc);

  ASSERT_EQ(pkt_count * MSBC_CODE_SIZE / 2 * 1.5,
            cras_sco_buf_queued(sco, dev.direction));
  ASSERT_EQ(2, cras_msbc_plc_handle_good_frames_called);
  pkt_count++;
  /* When the third packet is lost, we should call the handle_bad_packet and
   * still have right size of samples queued
   */
  pkt_count++;
  send_mSBC_packet(sock[0], pkt_count, 0);
  thread_cb((struct cras_sco*)cb_data, POLLIN);
  rc = recv(sock[0], sample, MSBC_PKT_SIZE, 0);
  ASSERT_EQ(MSBC_PKT_SIZE, rc);

  // Packet 1, 2, 4 are all good frames
  ASSERT_EQ(3, cras_msbc_plc_handle_good_frames_called);
  ASSERT_EQ(1, cras_msbc_plc_handle_bad_frames_called);
  ASSERT_EQ(pkt_count * MSBC_CODE_SIZE / 2 * 1.5,
            cras_sco_buf_queued(sco, dev.direction));
  pkt_count++;
  /* If the erroneous data reporting marks the packet as broken, we should
   * also call the handle_bad_packet and have the right size of samples queued.
   */
  send_mSBC_packet(sock[0], pkt_count, 1);

  set_sbc_codec_decoded_fail(1);

  thread_cb((struct cras_sco*)cb_data, POLLIN);
  rc = recv(sock[0], sample, MSBC_PKT_SIZE, 0);
  ASSERT_EQ(MSBC_PKT_SIZE, rc);

  ASSERT_EQ(3, cras_msbc_plc_handle_good_frames_called);
  ASSERT_EQ(2, cras_msbc_plc_handle_bad_frames_called);
  ASSERT_EQ(pkt_count * MSBC_CODE_SIZE / 2 * 1.5,
            cras_sco_buf_queued(sco, dev.direction));
  pkt_count++;
  /* If we can't decode the packet, we should also call the handle_bad_packet
   * and have the right size of samples queued
   */
  send_mSBC_packet(sock[0], pkt_count, 0);

  set_sbc_codec_decoded_fail(1);

  thread_cb((struct cras_sco*)cb_data, POLLIN);
  rc = recv(sock[0], sample, MSBC_PKT_SIZE, 0);
  ASSERT_EQ(MSBC_PKT_SIZE, rc);

  ASSERT_EQ(3, cras_msbc_plc_handle_good_frames_called);
  ASSERT_EQ(3, cras_msbc_plc_handle_bad_frames_called);
  ASSERT_EQ(pkt_count * MSBC_CODE_SIZE / 2 * 1.5,
            cras_sco_buf_queued(sco, dev.direction));

  cras_sco_stop(sco);
  ASSERT_EQ(0, cras_sco_running(sco));

  cras_sco_close_fd(sco);
  cras_sco_destroy(sco);
}

TEST(CrasSco, StartCrasScoAndWriteMsbc) {
  int rc;
  int sock[2];
  uint8_t sample[480];

  ResetStubData();

  set_sbc_codec_encoded_out(57);
  ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sock));

  sco = cras_sco_create(fake_device);
  ASSERT_NE(sco, (void*)NULL);

  cras_sco_set_fd(sco, sock[1]);
  cras_sco_start(63, HFP_CODEC_ID_MSBC, sco);
  send(sock[0], sample, 63, 0);

  // Trigger thread callback
  thread_cb((struct cras_sco*)cb_data, POLLIN);

  dev.direction = CRAS_STREAM_OUTPUT;
  ASSERT_EQ(0, cras_sco_add_iodev(sco, dev.direction, dev.format));

  // Assert queued samples unchanged before output device added
  ASSERT_EQ(0, cras_sco_buf_queued(sco, dev.direction));

  // Put some fake data and trigger thread callback again
  send(sock[0], sample, 63, 0);
  buf_increment_write(sco->playback_buf, 240);
  thread_cb((struct cras_sco*)cb_data, POLLIN);

  // Assert some samples written
  rc = recv(sock[0], sample, 60, 0);
  ASSERT_EQ(60, rc);
  ASSERT_EQ(0, cras_sco_buf_queued(sco, dev.direction));

  cras_sco_stop(sco);
  cras_sco_close_fd(sco);
  cras_sco_destroy(sco);
}

TEST(CrasSco, WBSLoggerPacketStatusDumpBinary) {
  struct packet_status_logger logger;
  char log_regex[64];
  int num_wraps[5] = {0, 0, 0, 1, 1};
  int wp[5] = {40, 150, 162, 100, 32};

  // Expect the log line wraps at correct length to avoid feedback redact.
  snprintf(log_regex, 64, "([01D]{%d}\n)*", PACKET_STATUS_LOG_LINE_WRAP);

  packet_status_logger_init(&logger);
  logger.size = PACKET_STATUS_LEN_BYTES * 8;
  for (int i = 0; i < 5; i++) {
    CaptureStdout();
    logger.num_wraps = num_wraps[i];
    logger.wp = wp[i];
    packet_status_logger_dump_binary(&logger);
    EXPECT_THAT(GetCapturedStdout(), MatchesRegex(log_regex));
  }
}

}  // namespace

extern "C" {

struct audio_thread* cras_iodev_list_get_audio_thread() {
  return NULL;
}

void audio_thread_add_events_callback(int fd,
                                      thread_callback cb,
                                      void* data,
                                      int events) {
  thread_cb = cb;
  cb_data = data;
  return;
}

int audio_thread_rm_callback_sync(struct audio_thread* thread, int fd) {
  thread_cb = NULL;
  cb_data = NULL;
  return 0;
}

void audio_thread_rm_callback(int fd) {}

void cras_bt_device_hfp_reconnect(struct cras_bt_device* device) {}

struct cras_msbc_plc* cras_msbc_plc_create() {
  cras_msbc_plc_create_called++;
  return NULL;
}

void cras_msbc_plc_destroy(struct cras_msbc_plc* plc) {}

int cras_msbc_plc_handle_bad_frames(struct cras_msbc_plc* plc,
                                    struct cras_audio_codec* codec,
                                    uint8_t* output) {
  cras_msbc_plc_handle_bad_frames_called++;
  return MSBC_CODE_SIZE;
}

int cras_msbc_plc_handle_good_frames(struct cras_msbc_plc* plc,
                                     const uint8_t* input,
                                     uint8_t* output) {
  cras_msbc_plc_handle_good_frames_called++;
  return MSBC_CODE_SIZE;
}

void packet_status_logger_init(struct packet_status_logger* logger) {
  if (logger == NULL) {
    return;
  }

  memset(logger->data, 0, PACKET_STATUS_LEN_BYTES);
  logger->size = PACKET_STATUS_LEN_BYTES * 8;
  logger->wp = 0;
  logger->num_wraps = 0;
  clock_gettime(CLOCK_MONOTONIC_RAW, &logger->ts);
}

void packet_status_logger_update(struct packet_status_logger* logger,
                                 bool val) {}
}
