/* Copyright 2017 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <time.h>
#include <unordered_map>

extern "C" {
#include "cras/src/server/cras_iodev.h"
#include "cras/src/server/dev_stream.h"
#include "third_party/utlist/utlist.h"
}

namespace {
struct cb_data {
  int frames_queued_ret;
  timespec frames_queued_ts;
};
std::unordered_map<cras_iodev*, cb_data> frames_queued_map;
std::unordered_map<cras_iodev*, cb_data> valid_frames_map;
std::unordered_map<cras_iodev*, timespec> drop_time_map;
std::unordered_map<const cras_iodev*, double> est_rate_ratio_map;
std::unordered_map<const cras_iodev*, int> update_rate_map;
std::unordered_map<const cras_ionode*, int> on_internal_card_map;
}  // namespace

void iodev_stub_reset() {
  frames_queued_map.clear();
  valid_frames_map.clear();
  drop_time_map.clear();
  est_rate_ratio_map.clear();
  update_rate_map.clear();
  on_internal_card_map.clear();
}

void iodev_stub_est_rate_ratio(cras_iodev* iodev, double ratio) {
  est_rate_ratio_map.insert({iodev, ratio});
}

void iodev_stub_update_rate(cras_iodev* iodev, int data) {
  update_rate_map.insert({iodev, data});
}

void iodev_stub_on_internal_card(cras_ionode* node, int data) {
  on_internal_card_map.insert({node, data});
}

void iodev_stub_frames_queued(cras_iodev* iodev, int ret, timespec ts) {
  cb_data data = {ret, ts};
  frames_queued_map.insert({iodev, data});
}

void iodev_stub_valid_frames(cras_iodev* iodev, int ret, timespec ts) {
  cb_data data = {ret, ts};
  valid_frames_map.insert({iodev, data});
}

bool iodev_stub_get_drop_time(cras_iodev* iodev, timespec* ts) {
  auto elem = drop_time_map.find(iodev);
  if (elem != drop_time_map.end()) {
    *ts = elem->second;
    return true;
  }
  return false;
}

extern "C" {

int cras_iodev_add_stream(struct cras_iodev* iodev, struct dev_stream* stream) {
  DL_APPEND(iodev->streams, stream);
  return 0;
}

int cras_iodev_get_valid_frames(struct cras_iodev* iodev,
                                struct timespec* tstamp) {
  auto elem = valid_frames_map.find(iodev);
  if (elem != valid_frames_map.end()) {
    *tstamp = elem->second.frames_queued_ts;
    return elem->second.frames_queued_ret;
  }
  clock_gettime(CLOCK_MONOTONIC_RAW, tstamp);
  return 0;
}

double cras_iodev_get_est_rate_ratio(const struct cras_iodev* iodev) {
  auto elem = est_rate_ratio_map.find(iodev);
  if (elem != est_rate_ratio_map.end()) {
    return elem->second;
  }
  return 1.0f;
}

int cras_iodev_get_dsp_delay(const struct cras_iodev* iodev) {
  return 0;
}

int cras_iodev_frames_queued(struct cras_iodev* iodev,
                             struct timespec* tstamp) {
  auto elem = frames_queued_map.find(iodev);
  if (elem != frames_queued_map.end()) {
    *tstamp = elem->second.frames_queued_ts;
    return elem->second.frames_queued_ret;
  }
  clock_gettime(CLOCK_MONOTONIC_RAW, tstamp);
  return 0;
}

struct dev_stream* cras_iodev_rm_stream(struct cras_iodev* iodev,
                                        const struct cras_rstream* stream) {
  return NULL;
}

int cras_iodev_update_rate(struct cras_iodev* iodev,
                           unsigned int level,
                           struct timespec* level_tstamp) {
  auto elem = update_rate_map.find(iodev);
  if (elem != update_rate_map.end()) {
    return elem->second;
  }
  return 0;
}

enum CRAS_IODEV_STATE cras_iodev_state(const struct cras_iodev* iodev) {
  return iodev->state;
}

unsigned int cras_iodev_all_streams_written(struct cras_iodev* iodev) {
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
  return 0;
}

int cras_iodev_get_input_buffer(struct cras_iodev* iodev, unsigned* frames) {
  return 0;
}

int cras_iodev_get_output_buffer(struct cras_iodev* iodev,
                                 struct cras_audio_area** area,
                                 unsigned* frames) {
  return 0;
}

float cras_iodev_get_software_gain_scaler(const struct cras_iodev* iodev) {
  return 0.0;
}

void cras_iodev_stream_written(struct cras_iodev* iodev,
                               struct dev_stream* stream,
                               unsigned int nwritten) {}

int cras_iodev_prepare_output_before_write_samples(struct cras_iodev* odev) {
  return 0;
}

int cras_iodev_buffer_avail(struct cras_iodev* iodev, unsigned hw_level) {
  return 0;
}

unsigned int cras_iodev_max_stream_offset(const struct cras_iodev* iodev) {
  return 0;
}

int cras_iodev_odev_should_wake(const struct cras_iodev* odev) {
  return 1;
}

int cras_iodev_output_underrun(struct cras_iodev* odev,
                               unsigned int hw_level,
                               unsigned int frames_written) {
  return 0;
}

int cras_iodev_reset_request(struct cras_iodev* iodev) {
  return 0;
}

unsigned int cras_iodev_stream_offset(struct cras_iodev* iodev,
                                      struct dev_stream* stream) {
  return 0;
}

unsigned int cras_iodev_get_num_underruns(const struct cras_iodev* iodev) {
  return 0;
}

unsigned int cras_iodev_frames_to_play_in_sleep(struct cras_iodev* odev,
                                                unsigned int* hw_level,
                                                struct timespec* hw_tstamp) {
  clock_gettime(CLOCK_MONOTONIC_RAW, hw_tstamp);
  *hw_level = 0;
  return 0;
}

void cras_iodev_update_highest_hw_level(struct cras_iodev* iodev,
                                        unsigned int hw_level) {}

void cras_iodev_start_stream(struct cras_iodev* iodev,
                             struct dev_stream* stream) {}

int cras_iodev_drop_frames_by_time(struct cras_iodev* iodev,
                                   struct timespec ts) {
  drop_time_map.insert({iodev, ts});
  return 0;
}

bool cras_iodev_is_on_internal_card(const struct cras_ionode* node) {
  auto elem = on_internal_card_map.find(node);
  if (elem != on_internal_card_map.end()) {
    return elem->second;
  }
  return 1;
}
}  // extern "C"
