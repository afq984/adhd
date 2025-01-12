/* Copyright 2012 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_INCLUDE_CRAS_CONFIG_H_
#define CRAS_INCLUDE_CRAS_CONFIG_H_

#include "cras_types.h"

#define CRAS_MIN_BUFFER_TIME_IN_US 1000  // 1 milliseconds
#define CRAS_MAX_BUFFER_TIME_IN_S 10     // 10 seconds

#define CRAS_SERVER_RT_THREAD_PRIORITY 12
#define CRAS_CLIENT_RT_THREAD_PRIORITY 10
#define CRAS_CLIENT_NICENESS_LEVEL -10
#define CRAS_SOCKET_FILE ".cras_socket"
#define CRAS_PLAYBACK_SOCKET_FILE ".cras_playback"
#define CRAS_CAPTURE_SOCKET_FILE ".cras_capture"
// Socket file paths for VMs.
#define CRAS_VMS_LEGACY_SOCKET_FILE "vms/.cras_socket"
#define CRAS_VMS_UNIFIED_SOCKET_FILE "vms/.cras_unified"
// Socket file paths for pluginVM.
#define CRAS_PLUGIN_PLAYBACK_SOCKET_FILE "vms/plugin/playback/.cras_socket"
#define CRAS_PLUGIN_UNIFIED_SOCKET_FILE "vms/plugin/unified/.cras_socket"

/* Maximum socket_path size, which is equals to sizeof(sun_path) in sockaddr_un
 * structure.
 */
#define CRAS_MAX_SOCKET_PATH_SIZE 108

/* CRAS_CONFIG_FILE_DIR is defined as $sysconfdir/cras by the configure
   script. */

// Gets the path to save UDS socket files.
const char* cras_config_get_system_socket_file_dir();

/* Fills sock_path by given connection type.
 *
 * Args:
 *    conn_type - server socket connection type.
 *    sock_path - socket path to be filled.
 *
 * Returns:
 *    0 for success, positive error code on error.
 */
int cras_fill_socket_path(enum CRAS_CONNECTION_TYPE conn_type, char* sock_path);

#endif  // CRAS_INCLUDE_CRAS_CONFIG_H_
