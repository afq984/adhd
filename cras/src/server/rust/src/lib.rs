// Copyright 2022 The ChromiumOS Authors.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

mod rate_estimator;
pub mod rate_estimator_bindings;

#[cfg(feature = "cras_dlc")]
pub use cras_dlc::{cras_dlc_sr_bt_get_root, cras_dlc_sr_bt_is_available};
