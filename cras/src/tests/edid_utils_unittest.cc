// Copyright 2012 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cras/src/common/edid_utils.h"

#include <gtest/gtest.h>
#include <stdint.h>
#include <stdio.h>

#include <string>

#include "cras_util.h"

namespace {

class EDIDTestSuite : public testing::Test {
 protected:
  virtual void SetUp() {
    static const uint8_t header[] = {
        0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
    };

    memcpy(edid_, header, sizeof(header));
    SetChecksum();
  }

  void SetChecksum() {
    uint8_t sum = 0;

    for (unsigned int i = 0; i < 127; i++)
      sum += edid_[i];

    edid_[127] = 256 - sum;
  }

  uint8_t edid_[2048];
};

TEST_F(EDIDTestSuite, EDIDValid) {
  EXPECT_TRUE(edid_valid(edid_));
}

TEST_F(EDIDTestSuite, EDIDBadHeader) {
  static const uint8_t bad_header[] = {
      0x00, 0xff, 0xff, 0xff, 0xff, 0xee, 0xff, 0x00,
  };

  memcpy(edid_, bad_header, sizeof(bad_header));
  SetChecksum();

  EXPECT_FALSE(edid_valid(edid_));
}

// Actual EDIDs read from sinks.

static const uint8_t test_no_aud_edid1[256] = {
    0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x06, 0xaf, 0x5c, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x12, 0x01, 0x03, 0x80, 0x1a, 0x0e, 0x78,
    0x0a, 0x99, 0x85, 0x95, 0x55, 0x56, 0x92, 0x28, 0x22, 0x50, 0x54, 0x00,
    0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x96, 0x19, 0x56, 0x28, 0x50, 0x00,
    0x08, 0x30, 0x18, 0x10, 0x24, 0x00, 0x00, 0x90, 0x10, 0x00, 0x00, 0x18,
    0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x41,
    0x55, 0x4f, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x00, 0x00, 0x00, 0xfe, 0x00, 0x42, 0x31, 0x31, 0x36, 0x58, 0x57, 0x30,
    0x32, 0x20, 0x56, 0x30, 0x20, 0x0a, 0x00, 0xf8};

static const uint8_t test_no_aud_edid2[256] = {
    0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x30, 0xe4, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x01, 0x03, 0x80, 0x1a, 0x0e, 0x78,
    0x0a, 0xbf, 0x45, 0x95, 0x58, 0x52, 0x8a, 0x28, 0x25, 0x50, 0x54, 0x00,
    0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x84, 0x1c, 0x56, 0xa8, 0x50, 0x00,
    0x19, 0x30, 0x30, 0x20, 0x35, 0x00, 0x00, 0x90, 0x10, 0x00, 0x00, 0x1b,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x4c,
    0x47, 0x20, 0x44, 0x69, 0x73, 0x70, 0x6c, 0x61, 0x79, 0x0a, 0x20, 0x20,
    0x00, 0x00, 0x00, 0xfc, 0x00, 0x4c, 0x50, 0x31, 0x31, 0x36, 0x57, 0x48,
    0x31, 0x2d, 0x54, 0x4c, 0x4e, 0x31, 0x00, 0x4e};

/* Has DTD that is too wide */
static const uint8_t test_no_aud_edid3[256] = {
    0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x10, 0xac, 0x63, 0x40,
    0x4c, 0x35, 0x31, 0x33, 0x0c, 0x15, 0x01, 0x03, 0x80, 0x40, 0x28, 0x78,
    0xea, 0x8d, 0x85, 0xad, 0x4f, 0x35, 0xb1, 0x25, 0x0e, 0x50, 0x54, 0xa5,
    0x4b, 0x00, 0x71, 0x4f, 0x81, 0x00, 0x81, 0x80, 0xa9, 0x40, 0xd1, 0x00,
    0xd1, 0x40, 0x01, 0x01, 0x01, 0x01, 0xe2, 0x68, 0x00, 0xa0, 0xa0, 0x40,
    0x2e, 0x60, 0x30, 0x20, 0x36, 0x00, 0x81, 0x91, 0x21, 0x00, 0x00, 0x1a,
    0x00, 0x00, 0x00, 0xff, 0x00, 0x50, 0x48, 0x35, 0x4e, 0x59, 0x31, 0x33,
    0x4d, 0x33, 0x31, 0x35, 0x4c, 0x0a, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x44,
    0x45, 0x4c, 0x4c, 0x20, 0x55, 0x33, 0x30, 0x31, 0x31, 0x0a, 0x20, 0x20,
    0x00, 0x00, 0x00, 0xfd, 0x00, 0x31, 0x56, 0x1d, 0x71, 0x1c, 0x00, 0x0a,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0xb0};

static const uint8_t test_edid1[256] = {
    0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x4d, 0xd9, 0x02, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x01, 0x03, 0x80, 0x00, 0x00, 0x78,
    0x0a, 0x0d, 0xc9, 0xa0, 0x57, 0x47, 0x98, 0x27, 0x12, 0x48, 0x4c, 0x00,
    0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1d, 0x80, 0xd0, 0x72, 0x1c,
    0x16, 0x20, 0x10, 0x2c, 0x25, 0x80, 0xc4, 0x8e, 0x21, 0x00, 0x00, 0x9e,
    0x01, 0x1d, 0x80, 0x18, 0x71, 0x1c, 0x16, 0x20, 0x58, 0x2c, 0x25, 0x00,
    0xc4, 0x8e, 0x21, 0x00, 0x00, 0x9e, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x48,
    0x44, 0x4d, 0x49, 0x20, 0x4c, 0x4c, 0x43, 0x0a, 0x20, 0x20, 0x20, 0x20,
    0x00, 0x00, 0x00, 0xfd, 0x00, 0x3b, 0x3d, 0x0f, 0x2d, 0x08, 0x00, 0x0a,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0xc0, 0x02, 0x03, 0x1e, 0x47,
    0x4f, 0x94, 0x13, 0x05, 0x03, 0x04, 0x02, 0x01, 0x16, 0x15, 0x07, 0x06,
    0x11, 0x10, 0x12, 0x1f, 0x23, 0x09, 0x07, 0x01, 0x65, 0x03, 0x0c, 0x00,
    0x10, 0x00, 0x8c, 0x0a, 0xd0, 0x90, 0x20, 0x40, 0x31, 0x20, 0x0c, 0x40,
    0x55, 0x00, 0x13, 0x8e, 0x21, 0x00, 0x00, 0x18, 0x01, 0x1d, 0x00, 0xbc,
    0x52, 0xd0, 0x1e, 0x20, 0xb8, 0x28, 0x55, 0x40, 0xc4, 0x8e, 0x21, 0x00,
    0x00, 0x1e, 0x8c, 0x0a, 0xd0, 0x8a, 0x20, 0xe0, 0x2d, 0x10, 0x10, 0x3e,
    0x96, 0x00, 0xc4, 0x8e, 0x21, 0x00, 0x00, 0x18, 0x01, 0x1d, 0x00, 0x72,
    0x51, 0xd0, 0x1e, 0x20, 0x6e, 0x28, 0x55, 0x00, 0xc4, 0x8e, 0x21, 0x00,
    0x00, 0x1e, 0x8c, 0x0a, 0xd0, 0x8a, 0x20, 0xe0, 0x2d, 0x10, 0x10, 0x3e,
    0x96, 0x00, 0x13, 0x8e, 0x21, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xfb};

static const uint8_t test_edid2[256] = {
    0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x4c, 0x2d, 0x10, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x31, 0x0f, 0x01, 0x03, 0x80, 0x10, 0x09, 0x8c,
    0x0a, 0xe2, 0xbd, 0xa1, 0x5b, 0x4a, 0x98, 0x24, 0x15, 0x47, 0x4a, 0x20,
    0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1d, 0x00, 0x72, 0x51, 0xd0,
    0x1e, 0x20, 0x6e, 0x28, 0x55, 0x00, 0xa0, 0x5a, 0x00, 0x00, 0x00, 0x1e,
    0x01, 0x1d, 0x80, 0x18, 0x71, 0x1c, 0x16, 0x20, 0x58, 0x2c, 0x25, 0x00,
    0xa0, 0x5a, 0x00, 0x00, 0x00, 0x9e, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x3b,
    0x3d, 0x1e, 0x2e, 0x08, 0x00, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x00, 0x00, 0x00, 0xfc, 0x00, 0x53, 0x41, 0x4d, 0x53, 0x55, 0x4e, 0x47,
    0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x8d, 0x02, 0x03, 0x16, 0x71,
    0x43, 0x84, 0x05, 0x03, 0x23, 0x09, 0x07, 0x07, 0x83, 0x01, 0x00, 0x00,
    0x65, 0x03, 0x0c, 0x00, 0x20, 0x00, 0x8c, 0x0a, 0xd0, 0x8a, 0x20, 0xe0,
    0x2d, 0x10, 0x10, 0x3e, 0x96, 0x00, 0xa0, 0x5a, 0x00, 0x00, 0x00, 0x18,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x30};

static const uint8_t test_edid3[256] = {
    0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x3d, 0xcb, 0x61, 0x07,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x01, 0x03, 0x80, 0x00, 0x00, 0x78,
    0x0a, 0x0d, 0xc9, 0xa0, 0x57, 0x47, 0x98, 0x27, 0x12, 0x48, 0x4c, 0x00,
    0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1d, 0x80, 0x18, 0x71, 0x1c,
    0x16, 0x20, 0x58, 0x2c, 0x25, 0x00, 0xc4, 0x8e, 0x21, 0x00, 0x00, 0x9e,
    0x01, 0x1d, 0x80, 0xd0, 0x72, 0x1c, 0x16, 0x20, 0x10, 0x2c, 0x25, 0x80,
    0xc4, 0x8e, 0x21, 0x00, 0x00, 0x9e, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x54,
    0x58, 0x2d, 0x53, 0x52, 0x36, 0x30, 0x35, 0x0a, 0x20, 0x20, 0x20, 0x20,
    0x00, 0x00, 0x00, 0xfd, 0x00, 0x17, 0xf0, 0x0f, 0x7e, 0x11, 0x00, 0x0a,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x93, 0x02, 0x03, 0x3b, 0x72,
    0x55, 0x85, 0x04, 0x03, 0x02, 0x0e, 0x0f, 0x07, 0x23, 0x24, 0x10, 0x94,
    0x13, 0x12, 0x11, 0x1d, 0x1e, 0x16, 0x25, 0x26, 0x01, 0x1f, 0x35, 0x09,
    0x7f, 0x07, 0x0f, 0x7f, 0x07, 0x17, 0x07, 0x50, 0x3f, 0x06, 0xc0, 0x57,
    0x06, 0x00, 0x5f, 0x7e, 0x01, 0x67, 0x5e, 0x00, 0x83, 0x4f, 0x00, 0x00,
    0x66, 0x03, 0x0c, 0x00, 0x20, 0x00, 0x80, 0x8c, 0x0a, 0xd0, 0x8a, 0x20,
    0xe0, 0x2d, 0x10, 0x10, 0x3e, 0x96, 0x00, 0xc4, 0x8e, 0x21, 0x00, 0x00,
    0x18, 0x8c, 0x0a, 0xd0, 0x90, 0x20, 0x40, 0x31, 0x20, 0x0c, 0x40, 0x55,
    0x00, 0xc4, 0x8e, 0x21, 0x00, 0x00, 0x18, 0x01, 0x1d, 0x00, 0x72, 0x51,
    0xd0, 0x1e, 0x20, 0x6e, 0x28, 0x55, 0x00, 0xc4, 0x8e, 0x21, 0x00, 0x00,
    0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xdd};

static const uint8_t test_edid4[256] = {
    0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x04, 0x72, 0x30, 0x02,
    0x01, 0x00, 0x00, 0x00, 0x18, 0x14, 0x01, 0x03, 0x80, 0x33, 0x1d, 0x78,
    0x0a, 0xdc, 0x55, 0xa3, 0x59, 0x48, 0x9e, 0x24, 0x11, 0x50, 0x54, 0xbf,
    0x6f, 0x00, 0x71, 0x4f, 0x81, 0xc0, 0xd1, 0xc0, 0xb3, 0x00, 0x81, 0x80,
    0x95, 0x00, 0x01, 0x01, 0x01, 0x01, 0x02, 0x3a, 0x80, 0x18, 0x71, 0x38,
    0x2d, 0x40, 0x58, 0x2c, 0x45, 0x00, 0xfe, 0x22, 0x11, 0x00, 0x00, 0x18,
    0x01, 0x1d, 0x00, 0x72, 0x51, 0xd0, 0x1e, 0x20, 0x6e, 0x28, 0x55, 0x00,
    0xfe, 0x22, 0x11, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x38,
    0x4c, 0x1e, 0x4b, 0x0f, 0x00, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x00, 0x00, 0x00, 0xfc, 0x00, 0x4d, 0x32, 0x33, 0x30, 0x41, 0x0a, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0xf0, 0x02, 0x03, 0x18, 0x74,
    0x45, 0x04, 0x05, 0x90, 0x03, 0x01, 0x23, 0x09, 0x17, 0x07, 0x83, 0x01,
    0x00, 0x00, 0x65, 0x03, 0x0c, 0x00, 0x30, 0x00, 0x01, 0x1d, 0x80, 0x18,
    0x71, 0x1c, 0x16, 0x20, 0x58, 0x2c, 0x25, 0x00, 0xfe, 0x22, 0x11, 0x00,
    0x00, 0x9e, 0x8c, 0x0a, 0xd0, 0x8a, 0x20, 0xe0, 0x2d, 0x10, 0x10, 0x3e,
    0x96, 0x00, 0xfe, 0x22, 0x11, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xd0,
};

static const uint8_t test_monitor_edid[256] = {
    0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x4d, 0xd9, 0x02, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x01, 0x03, 0x80, 0x00, 0x00, 0x78,
    0x0a, 0x0d, 0xc9, 0xa0, 0x57, 0x47, 0x98, 0x27, 0x12, 0x48, 0x4c, 0x00,
    0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1d, 0x80, 0xd0, 0x72, 0x1c,
    0x16, 0x20, 0x10, 0x2c, 0x25, 0x80, 0xc4, 0x8e, 0x21, 0x00, 0x00, 0x9e,
    0x01, 0x1d, 0x80, 0x18, 0x71, 0x1c, 0x16, 0x20, 0x58, 0x2c, 0x25, 0x00,
    0xc4, 0x8e, 0x21, 0x00, 0x00, 0x9e, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x48,
    0x44, 0x4d, 0x49, 0x20, 0x4c, 0x4c, 0x43, 0x20, 0x41, 0x42, 0x43, 0x44,
    0x00, 0x00, 0x00, 0xfd, 0x00, 0x3b, 0x3d, 0x0f, 0x2d, 0x08, 0x00, 0x0a,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0xc0, 0x02, 0x03, 0x1e, 0x47,
    0x4f, 0x94, 0x13, 0x05, 0x03, 0x04, 0x02, 0x01, 0x16, 0x15, 0x07, 0x06,
    0x11, 0x10, 0x12, 0x1f, 0x23, 0x09, 0x07, 0x01, 0x65, 0x03, 0x0c, 0x00,
    0x10, 0x00, 0x8c, 0x0a, 0xd0, 0x90, 0x20, 0x40, 0x31, 0x20, 0x0c, 0x40,
    0x55, 0x00, 0x13, 0x8e, 0x21, 0x00, 0x00, 0x18, 0x01, 0x1d, 0x00, 0xbc,
    0x52, 0xd0, 0x1e, 0x20, 0xb8, 0x28, 0x55, 0x40, 0xc4, 0x8e, 0x21, 0x00,
    0x00, 0x1e, 0x8c, 0x0a, 0xd0, 0x8a, 0x20, 0xe0, 0x2d, 0x10, 0x10, 0x3e,
    0x96, 0x00, 0xc4, 0x8e, 0x21, 0x00, 0x00, 0x18, 0x01, 0x1d, 0x00, 0x72,
    0x51, 0xd0, 0x1e, 0x20, 0x6e, 0x28, 0x55, 0x00, 0xc4, 0x8e, 0x21, 0x00,
    0x00, 0x1e, 0x8c, 0x0a, 0xd0, 0x8a, 0x20, 0xe0, 0x2d, 0x10, 0x10, 0x3e,
    0x96, 0x00, 0x13, 0x8e, 0x21, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xfb};

static const uint8_t* test_no_aud_edids[] = {
    test_no_aud_edid1,
    test_no_aud_edid2,
    test_no_aud_edid3,
};

static const uint8_t* test_edids[] = {
    test_edid1,
    test_edid2,
    test_edid3,
    test_edid4,
};

static const char* monitor_names[] = {
    "HDMI LLC",
    "SAMSUNG",
    "TX-SR605",
    "M230A",
};

TEST_F(EDIDTestSuite, NoAudEDID) {
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE(test_no_aud_edids); i++) {
    EXPECT_TRUE(edid_valid(test_no_aud_edids[i]));
    EXPECT_FALSE(edid_lpcm_support(test_no_aud_edids[i], 1));
  }
}

TEST_F(EDIDTestSuite, AudEDID) {
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE(test_edids); i++) {
    EXPECT_TRUE(edid_valid(test_edids[i]));
    EXPECT_TRUE(edid_lpcm_support(test_edids[i], 1));
  }
}

TEST_F(EDIDTestSuite, EDIDMonitorName) {
  unsigned int i;
  char buf[DTD_SIZE];

  for (i = 0; i < ARRAY_SIZE(test_edids); i++) {
    EXPECT_EQ(0, edid_get_monitor_name(test_edids[i], buf, DTD_SIZE));
    EXPECT_STREQ(monitor_names[i], buf);
  }

  EXPECT_EQ(0, edid_get_monitor_name(test_monitor_edid, buf, DTD_SIZE));
  EXPECT_STREQ("HDMI LLC ABCD", buf);
  EXPECT_EQ(0, edid_get_monitor_name(test_monitor_edid, buf, 11));
  EXPECT_STREQ("HDMI LLC A", buf);
}

std::string get_mfg_id(const edid_device_id& id) {
  uint16_t mfg_id = (id.mfg_id[0] << 8) + id.mfg_id[1];
  return std::string{
      static_cast<char>((mfg_id >> 10 & 0x1f) + 'A' - 1),
      static_cast<char>((mfg_id >> 5 & 0x1f) + 'A' - 1),
      static_cast<char>((mfg_id & 0x1f) + 'A' - 1),
  };
}

TEST_F(EDIDTestSuite, EDIDDeviceID) {
  struct edid_device_id device_id = edid_get_device_id(test_no_aud_edid1);
  EXPECT_EQ(get_mfg_id(device_id), "AUO");
  EXPECT_EQ(device_id.prod_code, 8284);
  EXPECT_EQ(device_id.serial, 0);

  device_id = edid_get_device_id(test_no_aud_edid2);
  EXPECT_EQ(get_mfg_id(device_id), "LGD");
  EXPECT_EQ(device_id.prod_code, 0);
  EXPECT_EQ(device_id.serial, 0);

  device_id = edid_get_device_id(test_no_aud_edid3);
  EXPECT_EQ(get_mfg_id(device_id), "DEL");
  EXPECT_EQ(device_id.prod_code, 16483);
  EXPECT_EQ(device_id.serial, 858862924);

  device_id = edid_get_device_id(test_edid1);
  EXPECT_EQ(get_mfg_id(device_id), "SNY");
  EXPECT_EQ(device_id.prod_code, 2);
  EXPECT_EQ(device_id.serial, 0);

  device_id = edid_get_device_id(test_edid2);
  EXPECT_EQ(get_mfg_id(device_id), "SAM");
  EXPECT_EQ(device_id.prod_code, 528);
  EXPECT_EQ(device_id.serial, 0);

  device_id = edid_get_device_id(test_edid3);
  EXPECT_EQ(get_mfg_id(device_id), "ONK");
  EXPECT_EQ(device_id.prod_code, 1889);
  EXPECT_EQ(device_id.serial, 0);

  device_id = edid_get_device_id(test_edid4);
  EXPECT_EQ(get_mfg_id(device_id), "ACR");
  EXPECT_EQ(device_id.prod_code, 560);
  EXPECT_EQ(device_id.serial, 1);

  device_id = edid_get_device_id(test_monitor_edid);
  EXPECT_EQ(get_mfg_id(device_id), "SNY");
  EXPECT_EQ(device_id.prod_code, 2);
  EXPECT_EQ(device_id.serial, 0);
}

}  //  namespace
