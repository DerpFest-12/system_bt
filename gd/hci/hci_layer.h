/*
 * Copyright 2019 The Android Open Source Project
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

#include <map>

#include "common/address.h"
#include "common/bidi_queue.h"
#include "common/class_of_device.h"
#include "hal/hci_hal.h"
#include "hci/hci_packets.h"
#include "module.h"
#include "os/utils.h"

namespace bluetooth {
namespace hci {

class HciLayer : public Module {
 public:
  HciLayer();
  virtual ~HciLayer();
  DISALLOW_COPY_AND_ASSIGN(HciLayer);

  virtual void EnqueueCommand(std::unique_ptr<CommandPacketBuilder> command,
                              std::function<void(CommandStatusView)> on_status,
                              std::function<void(CommandCompleteView)> on_complete, os::Handler* handler);

  virtual common::BidiQueueEnd<AclPacketBuilder, AclPacketView>* GetAclQueueEnd();

  virtual void RegisterEventHandler(EventCode event_code, std::function<void(EventPacketView)> event_handler,
                                    os::Handler* handler);

  virtual void UnregisterEventHandler(EventCode event_code);

  static const ModuleFactory Factory;

  void ListDependencies(ModuleList* list) override;

  void Start() override;

  void Stop() override;

 private:
  struct impl;
  std::unique_ptr<impl> impl_;
};
}  // namespace hci
}  // namespace bluetooth
