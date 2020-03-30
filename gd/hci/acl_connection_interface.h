/*
 * Copyright 2020 The Android Open Source Project
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

#include "common/callback.h"
#include "hci/hci_packets.h"
#include "os/utils.h"

namespace bluetooth {
namespace hci {

class AclConnectionInterface {
 public:
  AclConnectionInterface() = default;
  virtual ~AclConnectionInterface() = default;
  DISALLOW_COPY_AND_ASSIGN(AclConnectionInterface);

  virtual void EnqueueCommand(std::unique_ptr<ConnectionManagementCommandBuilder> command,
                              common::OnceCallback<void(CommandCompleteView)> on_complete, os::Handler* handler) = 0;

  virtual void EnqueueCommand(std::unique_ptr<ConnectionManagementCommandBuilder> command,
                              common::OnceCallback<void(CommandStatusView)> on_status, os::Handler* handler) = 0;

  static constexpr EventCode AclConnectionEvents[] = {
      EventCode::CONNECTION_PACKET_TYPE_CHANGED,
      EventCode::ROLE_CHANGE,
      EventCode::CONNECTION_COMPLETE,
      EventCode::DISCONNECTION_COMPLETE,
      EventCode::CONNECTION_REQUEST,
      EventCode::CONNECTION_PACKET_TYPE_CHANGED,
      EventCode::AUTHENTICATION_COMPLETE,
      EventCode::READ_CLOCK_OFFSET_COMPLETE,
      EventCode::MODE_CHANGE,
      EventCode::QOS_SETUP_COMPLETE,
      EventCode::ROLE_CHANGE,
      EventCode::FLOW_SPECIFICATION_COMPLETE,
      EventCode::FLUSH_OCCURRED,
      EventCode::READ_REMOTE_SUPPORTED_FEATURES_COMPLETE,
      EventCode::READ_REMOTE_EXTENDED_FEATURES_COMPLETE,
      EventCode::READ_REMOTE_VERSION_INFORMATION_COMPLETE,
      EventCode::ENCRYPTION_CHANGE,
      EventCode::LINK_SUPERVISION_TIMEOUT_CHANGED,
  };
};
}  // namespace hci
}  // namespace bluetooth
