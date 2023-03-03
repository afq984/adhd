/* Copyright 2015 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * An incremental version of the SuperFastHash hash function from
 * http://www.azillionmonkeys.com/qed/hash.html
 * The code did not come with its own header file, so declaring the function
 * here.
 */

#ifndef CRAS_SRC_COMMON_SFH_H_
#define CRAS_SRC_COMMON_SFH_H_

uint32_t SuperFastHash(const char* data, int len, uint32_t hash);

#endif  // CRAS_SRC_COMMON_SFH_H_
