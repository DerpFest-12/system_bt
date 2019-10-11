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

#include "common/callback.h"
#include "hci/address.h"
#include "hci/hci_packets.h"
#include "module.h"
#include "os/handler.h"

namespace bluetooth {
namespace hci {

class Controller : public Module {
 public:
  Controller();
  virtual ~Controller();
  DISALLOW_COPY_AND_ASSIGN(Controller);

  virtual void RegisterCompletedAclPacketsCallback(
      common::Callback<void(uint16_t /* handle */, uint16_t /* num_packets */)> cb, os::Handler* handler);

  virtual std::string GetControllerLocalName();

  virtual LocalVersionInformation GetControllerLocalVersionInformation();

  virtual std::array<uint8_t, 64> GetControllerLocalSupportedCommands();

  virtual uint64_t GetControllerLocalSupportedFeatures();

  virtual uint8_t GetControllerLocalExtendedFeaturesMaxPageNumber();

  virtual uint64_t GetControllerLocalExtendedFeatures(uint8_t page_number);

  virtual uint16_t GetControllerAclPacketLength();

  virtual uint16_t GetControllerNumAclPacketBuffers();

  virtual uint8_t GetControllerScoPacketLength();

  virtual uint16_t GetControllerNumScoPacketBuffers();

  virtual Address GetControllerMacAddress();

  virtual void SetEventMask(uint64_t event_mask);

  virtual void Reset();

  virtual void SetEventFilterClearAll();

  virtual void SetEventFilterInquiryResultAllDevices();

  virtual void SetEventFilterInquiryResultClassOfDevice(ClassOfDevice class_of_device,
                                                        ClassOfDevice class_of_device_mask);

  virtual void SetEventFilterInquiryResultAddress(Address address);

  virtual void SetEventFilterConnectionSetupAllDevices(AutoAcceptFlag auto_accept_flag);

  virtual void SetEventFilterConnectionSetupClassOfDevice(ClassOfDevice class_of_device,
                                                          ClassOfDevice class_of_device_mask,
                                                          AutoAcceptFlag auto_accept_flag);

  virtual void SetEventFilterConnectionSetupAddress(Address address, AutoAcceptFlag auto_accept_flag);

  virtual void WriteLocalName(std::string local_name);

  virtual void HostBufferSize(uint16_t host_acl_data_packet_length, uint8_t host_synchronous_data_packet_length,
                              uint16_t host_total_num_acl_data_packets,
                              uint16_t host_total_num_synchronous_data_packets);

  // LE controller commands
  virtual void LeSetEventMask(uint64_t le_event_mask);

  LeBufferSize GetControllerLeBufferSize();

  uint64_t GetControllerLeLocalSupportedFeatures();

  uint64_t GetControllerLeSupportedStates();

  LeMaximumDataLength GetControllerLeMaximumDataLength();

  uint16_t GetControllerLeMaximumAdvertisingDataLength();

  uint16_t GetControllerLeNumberOfSupportedAdverisingSets();

  VendorCapabilities GetControllerVendorCapabilities();

  bool IsSupported(OpCode op_code);

  static const ModuleFactory Factory;

 protected:
  void ListDependencies(ModuleList* list) override;

  void Start() override;

  void Stop() override;

 private:
  struct impl;
  std::unique_ptr<impl> impl_;
};

}  // namespace hci
}  // namespace bluetooth
