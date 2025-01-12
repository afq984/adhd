/* Copyright 2013 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_SRC_SERVER_CRAS_BT_ADAPTER_H_
#define CRAS_SRC_SERVER_CRAS_BT_ADAPTER_H_

#include <dbus/dbus.h>

struct cras_bt_adapter;

/* Creates an bt_adapter instance representing the bluetooth controller
 * on the system.
 * Args:
 *    conn - The dbus connection.
 *    object_path - Object path of the bluetooth controller.
 */
struct cras_bt_adapter* cras_bt_adapter_create(DBusConnection* conn,
                                               const char* object_path);
void cras_bt_adapter_destroy(struct cras_bt_adapter* adapter);
void cras_bt_adapter_reset();

struct cras_bt_adapter* cras_bt_adapter_get(const char* object_path);
size_t cras_bt_adapter_get_list(struct cras_bt_adapter*** adapter_list_out);

const char* cras_bt_adapter_object_path(const struct cras_bt_adapter* adapter);
const char* cras_bt_adapter_address(const struct cras_bt_adapter* adapter);
const char* cras_bt_adapter_name(const struct cras_bt_adapter* adapter);

int cras_bt_adapter_powered(const struct cras_bt_adapter* adapter);

/*
 * Returns true if adapter supports wide band speech feature.
 */
int cras_bt_adapter_wbs_supported(struct cras_bt_adapter* adapter);

void cras_bt_adapter_update_properties(struct cras_bt_adapter* adapter,
                                       DBusMessageIter* properties_array_iter,
                                       DBusMessageIter* invalidated_array_iter);

int cras_bt_adapter_on_usb(struct cras_bt_adapter* adapter);

/*
 * Queries adapter supported capabilies from bluetooth daemon. This shall
 * be called only after adapter powers on.
 */
int cras_bt_adapter_get_supported_capabilities(struct cras_bt_adapter* adapter);

#endif  // CRAS_SRC_SERVER_CRAS_BT_ADAPTER_H_
