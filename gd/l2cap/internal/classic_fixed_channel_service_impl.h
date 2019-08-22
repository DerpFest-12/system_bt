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

#include "l2cap/classic_fixed_channel.h"
#include "l2cap/classic_fixed_channel_manager.h"
#include "l2cap/classic_fixed_channel_service.h"

namespace bluetooth {
namespace l2cap {
namespace internal {

class ClassicFixedChannelServiceImpl {
 public:
  virtual ~ClassicFixedChannelServiceImpl() = default;

  struct PendingRegistration {
    os::Handler* user_handler_ = nullptr;
    ClassicFixedChannelManager::OnRegistrationCompleteCallback on_registration_complete_callback_;
    ClassicFixedChannelManager::OnConnectionOpenCallback on_connection_open_callback_;
  };

  virtual void NotifyChannelCreation(std::unique_ptr<ClassicFixedChannel> channel) {
    user_handler_->Post(common::BindOnce(on_connection_open_callback_, std::move(channel)));
  }

  friend class ClassicFixedChannelServiceManagerImpl;

 protected:
  // protected access for mocking
  ClassicFixedChannelServiceImpl(os::Handler* user_handler,
                                 ClassicFixedChannelManager::OnConnectionOpenCallback on_connection_open_callback)
      : user_handler_(user_handler), on_connection_open_callback_(std::move(on_connection_open_callback)) {}

 private:
  os::Handler* user_handler_ = nullptr;
  ClassicFixedChannelManager::OnConnectionOpenCallback on_connection_open_callback_;
};

}  // namespace internal
}  // namespace l2cap
}  // namespace bluetooth
