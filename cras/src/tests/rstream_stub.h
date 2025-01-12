/* Copyright 2017 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_SRC_TESTS_RSTREAM_STUB_H_
#define CRAS_SRC_TESTS_RSTREAM_STUB_H_

#include <time.h>

void rstream_stub_reset();

void rstream_stub_dev_offset(const cras_rstream* rstream,
                             unsigned int dev_id,
                             unsigned int offset);

// Stub that rstream is pending the reply from client or not.
void rstream_stub_pending_reply(const cras_rstream* rstream, int ret_value);

#endif  // CRAS_SRC_TESTS_RSTREAM_STUB_H_
