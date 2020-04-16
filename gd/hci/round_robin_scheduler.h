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

#include <stdint.h>

#include "common/bidi_queue.h"
#include "hci/acl_manager.h"
#include "hci/controller.h"
#include "hci/hci_packets.h"
#include "os/handler.h"

namespace bluetooth {
namespace hci {

class RoundRobinScheduler {
 public:
  RoundRobinScheduler(os::Handler* handler, Controller* controller,
                      common::BidiQueueEnd<AclPacketBuilder, AclPacketView>* hci_queue_end);
  ~RoundRobinScheduler();

  enum ConnectionType { CLASSIC, LE };

  struct acl_queue_handler {
    ConnectionType connection_type_;
    AclConnection::QueueDownEnd* queue_down_end_;
    bool dequeue_is_registered_ = false;
    uint16_t number_of_sent_packets_ = 0;  // Track credits
    bool is_disconnected_ = false;
  };

  void Register(ConnectionType connection_type, uint16_t handle, AclConnection::QueueDownEnd* queue_down_end);
  void Unregister(uint16_t handle);
  void SetDisconnect(uint16_t handle);

 private:
  void StartRoundRobin();
  void BufferPacket(std::map<uint16_t, acl_queue_handler>::iterator acl_queue_handler);
  void UnregisterAllConnections();
  void SendNextFragment();
  std::unique_ptr<AclPacketBuilder> HandleEnqueueNextFragment();
  void IncomingAclCredits(uint16_t handle, uint16_t credits);

  os::Handler* handler_ = nullptr;
  Controller* controller_ = nullptr;
  std::map<uint16_t, acl_queue_handler> acl_queue_handlers_;
  std::queue<std::pair<ConnectionType, std::unique_ptr<AclPacketBuilder>>> fragments_to_send_;
  uint16_t max_acl_packet_credits_ = 0;
  uint16_t acl_packet_credits_ = 0;
  uint16_t le_max_acl_packet_credits_ = 0;
  uint16_t le_acl_packet_credits_ = 0;
  size_t hci_mtu_{0};
  size_t le_hci_mtu_{0};
  std::atomic_bool enqueue_registered_ = false;
  common::BidiQueueEnd<AclPacketBuilder, AclPacketView>* hci_queue_end_ = nullptr;
  // first register queue end for the Round-robin schedule
  std::map<uint16_t, acl_queue_handler>::iterator starting_point_;
};

}  // namespace hci
}  // namespace bluetooth