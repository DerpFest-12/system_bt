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

#include "hci/fuzz/hci_layer_fuzz_client.h"

using bluetooth::hci::AclPacketView;

namespace bluetooth {
namespace hci {
namespace fuzz {

const ModuleFactory HciLayerFuzzClient::Factory = ModuleFactory([]() { return new HciLayerFuzzClient(); });

void HciLayerFuzzClient::Start() {
  hci_ = GetDependency<hci::HciLayer>();
  aclDevNull_ = new os::fuzz::DevNullQueue<AclPacketView>(hci_->GetAclQueueEnd(), GetHandler());
  aclDevNull_->Start();
  aclInject_ = new os::fuzz::FuzzInjectQueue<AclPacketBuilder>(hci_->GetAclQueueEnd(), GetHandler());

  // Can't do security right now, due to the Encryption Change conflict between ACL manager & security
  // security_interface_ = hci_->GetSecurityInterface(common::Bind([](EventPacketView){}), GetHandler());
  le_security_interface_ = hci_->GetLeSecurityInterface(common::Bind([](LeMetaEventView) {}), GetHandler());
  acl_connection_interface_ = hci_->GetAclConnectionInterface(
      common::Bind([](EventPacketView) {}), common::Bind([](uint16_t, hci::ErrorCode) {}), GetHandler());
  le_acl_connection_interface_ = hci_->GetLeAclConnectionInterface(
      common::Bind([](LeMetaEventView) {}), common::Bind([](uint16_t, hci::ErrorCode) {}), GetHandler());
  le_advertising_interface_ = hci_->GetLeAdvertisingInterface(common::Bind([](LeMetaEventView) {}), GetHandler());
  le_scanning_interface_ = hci_->GetLeScanningInterface(common::Bind([](LeMetaEventView) {}), GetHandler());
}

void HciLayerFuzzClient::Stop() {
  aclDevNull_->Stop();
  delete aclDevNull_;
  delete aclInject_;
}

void HciLayerFuzzClient::injectAclData(std::vector<uint8_t> data) {
  hci::AclPacketView aclPacket = hci::AclPacketView::FromBytes(data);
  if (!aclPacket.IsValid()) {
    return;
  }

  aclInject_->Inject(AclPacketBuilder::FromView(aclPacket));
}

void HciLayerFuzzClient::injectHciCommand(std::vector<uint8_t> data) {
  inject_command<CommandPacketView, CommandPacketBuilder>(data, hci_);
}

void HciLayerFuzzClient::injectSecurityCommand(std::vector<uint8_t> data) {
  inject_command<SecurityCommandView, SecurityCommandBuilder>(data, security_interface_);
}

void HciLayerFuzzClient::injectLeSecurityCommand(std::vector<uint8_t> data) {
  inject_command<LeSecurityCommandView, LeSecurityCommandBuilder>(data, le_security_interface_);
}

void HciLayerFuzzClient::injectAclConnectionCommand(std::vector<uint8_t> data) {
  inject_command<ConnectionManagementCommandView, ConnectionManagementCommandBuilder>(data, acl_connection_interface_);
}

void HciLayerFuzzClient::injectLeAclConnectionCommand(std::vector<uint8_t> data) {
  inject_command<LeConnectionManagementCommandView, LeConnectionManagementCommandBuilder>(data,
                                                                                          le_acl_connection_interface_);
}

void HciLayerFuzzClient::injectLeAdvertisingCommand(std::vector<uint8_t> data) {
  inject_command<LeAdvertisingCommandView, LeAdvertisingCommandBuilder>(data, le_advertising_interface_);
}

void HciLayerFuzzClient::injectLeScanningCommand(std::vector<uint8_t> data) {
  inject_command<LeScanningCommandView, LeScanningCommandBuilder>(data, le_scanning_interface_);
}

}  // namespace fuzz
}  // namespace hci
}  // namespace bluetooth
