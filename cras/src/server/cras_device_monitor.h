/* Copyright 2016 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_SRC_SERVER_CRAS_DEVICE_MONITOR_H_
#define CRAS_SRC_SERVER_CRAS_DEVICE_MONITOR_H_

// Asks main thread to reset a device
int cras_device_monitor_reset_device(unsigned int dev_idx);

// Asks main thread to set mute/unmute state on a device.
int cras_device_monitor_set_device_mute_state(unsigned int dev_idx);

// Initializes device monitor and sets main thread callback.
int cras_device_monitor_init();

/* Asks main thread to close device because error has occured in audio
 * thread. */
int cras_device_monitor_error_close(unsigned int dev_idx);

#endif  // CRAS_SRC_SERVER_CRAS_DEVICE_MONITOR_H_
