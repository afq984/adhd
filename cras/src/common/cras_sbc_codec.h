/* Copyright 2013 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_SRC_COMMON_CRAS_SBC_CODEC_H_
#define CRAS_SRC_COMMON_CRAS_SBC_CODEC_H_

#include <sbc/sbc.h>

#include "cras/src/common/cras_audio_codec.h"

/* Creates an sbc codec.
 * Args:
 *    freq: frequency for sbc encoder settings.
 *    mode: mode for sbc encoder settings.
 *    subbands: subbands for sbc encoder settings.
 *    alloc: allocation method for sbc encoder settings.
 *    blocks: blocks for sbc encoder settings.
 *    bitpool: bitpool for sbc encoder settings.
 */
struct cras_audio_codec* cras_sbc_codec_create(uint8_t freq,
                                               uint8_t mode,
                                               uint8_t subbands,
                                               uint8_t alloc,
                                               uint8_t blocks,
                                               uint8_t bitpool);

/* Creates an mSBC codec, which is a version of SBC codec used for
 * wideband speech mode of HFP. */
struct cras_audio_codec* cras_msbc_codec_create();

/* Destroys an sbc codec.
 * Args:
 *    codec: the codec to destroy.
 */
void cras_sbc_codec_destroy(struct cras_audio_codec* codec);

/* Gets codesize, the input block size of sbc codec in bytes.
 */
int cras_sbc_get_codesize(struct cras_audio_codec* codec);

/* Gets frame_length, the output block size of sbc codec in bytes.
 */
int cras_sbc_get_frame_length(struct cras_audio_codec* codec);

#endif  // COMMON_CRAS_SRC_COMMON_CRAS_SBC_CODEC_H_
