/*
 * Copyright 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "btaa/activity_attribution.h"
#include "hal/snoop_logger.h"
#include "hci/address.h"

namespace bluetooth {
namespace activity_attribution {

struct BtaaHciPacket {
  Activity activity;
  hci::Address address;
  uint16_t byte_count;
  BtaaHciPacket() {}
  BtaaHciPacket(Activity activity, hci::Address address, uint16_t byte_count)
      : activity(activity), address(address), byte_count(byte_count) {}
} __attribute__((__packed__));

class HciProcessor {
 public:
  std::vector<BtaaHciPacket> OnHciPacket(hal::HciPacket packet, hal::SnoopLogger::PacketType type, uint16_t length);
};

}  // namespace activity_attribution
}  // namespace bluetooth
