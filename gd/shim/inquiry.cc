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
#define LOG_TAG "bt_gd_shim"

#include <functional>
#include <memory>

#include "common/bidi_queue.h"
#include "hci/address.h"
#include "hci/controller.h"
#include "hci/hci_packets.h"
#include "module.h"
#include "neighbor/inquiry.h"
#include "neighbor/scan_parameters.h"
#include "os/handler.h"
#include "os/log.h"
#include "shim/inquiry.h"

namespace bluetooth {
namespace shim {

constexpr size_t kMaxExtendedInquiryResponse = 240;

struct Inquiry::impl {
  void Result(hci::InquiryResultView view);
  void ResultWithRssi(hci::InquiryResultWithRssiView view);
  void ExtendedResult(hci::ExtendedInquiryResultView view);
  void Complete(hci::ErrorCode status);

  void RegisterInquiryCallbacks(LegacyInquiryCallbacks callbacks);
  void UnregisterInquiryCallbacks();

  LegacyInquiryCallbacks callbacks_;

  neighbor::InquiryModule* module_{nullptr};

  impl(neighbor::InquiryModule* module);
  ~impl();

  neighbor::ScanParameters params_{
      .interval = static_cast<neighbor::ScanInterval>(0),
      .window = static_cast<neighbor::ScanWindow>(0),
  };
  bool general_inquiry_active_{false};
  bool limited_inquiry_active_{false};
  bool general_periodic_inquiry_active_{false};
  bool limited_periodic_inquiry_active_{false};
};

const ModuleFactory Inquiry::Factory = ModuleFactory([]() { return new Inquiry(); });

void Inquiry::impl::Result(hci::InquiryResultView view) {
  ASSERT(callbacks_.result_callback != nullptr);

  for (auto& response : view.GetInquiryResults()) {
    callbacks_.result_callback(response.bd_addr_.ToString(), static_cast<uint8_t>(response.page_scan_repetition_mode_),
                               response.class_of_device_.ToString(), response.clock_offset_);
  }
}

void Inquiry::impl::ResultWithRssi(hci::InquiryResultWithRssiView view) {
  ASSERT(callbacks_.result_with_rssi_callback != nullptr);

  for (auto& response : view.GetInquiryResults()) {
    callbacks_.result_with_rssi_callback(response.address_.ToString(),
                                         static_cast<uint8_t>(response.page_scan_repetition_mode_),
                                         response.class_of_device_.ToString(), response.clock_offset_, response.rssi_);
  }
}

void Inquiry::impl::ExtendedResult(hci::ExtendedInquiryResultView view) {
  ASSERT(callbacks_.extended_result_callback != nullptr);

  uint8_t gap_data_buffer[kMaxExtendedInquiryResponse];
  uint8_t* data = nullptr;
  size_t data_len = 0;

  if (!view.GetExtendedInquiryResponse().empty()) {
    bzero(gap_data_buffer, sizeof(gap_data_buffer));
    uint8_t* p = gap_data_buffer;
    for (auto gap_data : view.GetExtendedInquiryResponse()) {
      *p++ = gap_data.data_.size() + sizeof(gap_data.data_type_);
      *p++ = static_cast<uint8_t>(gap_data.data_type_);
      p = (uint8_t*)memcpy(p, &gap_data.data_[0], gap_data.data_.size()) + gap_data.data_.size();
    }
    data = gap_data_buffer;
    data_len = p - data;
  }
  callbacks_.extended_result_callback(view.GetAddress().ToString(),
                                      static_cast<uint8_t>(view.GetPageScanRepetitionMode()),
                                      view.GetClassOfDevice().ToString(), static_cast<uint16_t>(view.GetClockOffset()),
                                      static_cast<int8_t>(view.GetRssi()), data, data_len);
}

void Inquiry::impl::Complete(hci::ErrorCode status) {
  ASSERT(callbacks_.complete_callback != nullptr);
  limited_inquiry_active_ = false;
  general_inquiry_active_ = false;
  callbacks_.complete_callback(static_cast<uint16_t>(status));
}

void Inquiry::impl::RegisterInquiryCallbacks(LegacyInquiryCallbacks callbacks) {
  callbacks_ = callbacks;
  ASSERT(callbacks_.result_callback);
  ASSERT(callbacks_.result_with_rssi_callback);
  ASSERT(callbacks_.extended_result_callback);
  ASSERT(callbacks_.complete_callback);
}

void Inquiry::impl::UnregisterInquiryCallbacks() {
  callbacks_ = {{}, {}, {}, {}};
}

Inquiry::impl::impl(neighbor::InquiryModule* inquiry_module) : module_(inquiry_module) {
  neighbor::InquiryCallbacks inquiry_callbacks;
  inquiry_callbacks.result = std::bind(&Inquiry::impl::Result, this, std::placeholders::_1);
  inquiry_callbacks.result_with_rssi = std::bind(&Inquiry::impl::ResultWithRssi, this, std::placeholders::_1);
  inquiry_callbacks.extended_result = std::bind(&Inquiry::impl::ExtendedResult, this, std::placeholders::_1);
  inquiry_callbacks.complete = std::bind(&Inquiry::impl::Complete, this, std::placeholders::_1);

  module_->RegisterCallbacks(inquiry_callbacks);
}

Inquiry::impl::~impl() {
  module_->UnregisterCallbacks();
}

void Inquiry::StartGeneralInquiry(uint8_t inquiry_length, uint8_t num_responses, LegacyInquiryCallbacks callbacks) {
  pimpl_->RegisterInquiryCallbacks(callbacks);
  pimpl_->general_inquiry_active_ = true;
  pimpl_->module_->StartGeneralInquiry(inquiry_length, num_responses);
}

void Inquiry::StartLimitedInquiry(uint8_t inquiry_length, uint8_t num_responses, LegacyInquiryCallbacks callbacks) {
  pimpl_->RegisterInquiryCallbacks(callbacks);
  pimpl_->limited_inquiry_active_ = true;
  return pimpl_->module_->StartLimitedInquiry(inquiry_length, num_responses);
}

void Inquiry::StopInquiry() {
  if (!pimpl_->limited_inquiry_active_ && !pimpl_->general_inquiry_active_) {
    LOG_WARN("Ignoring attempt to stop an inactive inquiry");
    return;
  }
  pimpl_->limited_inquiry_active_ = false;
  pimpl_->general_inquiry_active_ = false;
  pimpl_->module_->StopInquiry();
  pimpl_->UnregisterInquiryCallbacks();
}

bool Inquiry::IsGeneralInquiryActive() const {
  return pimpl_->general_inquiry_active_;
}

bool Inquiry::IsLimitedInquiryActive() const {
  return pimpl_->limited_inquiry_active_;
}

void Inquiry::StartGeneralPeriodicInquiry(uint8_t inquiry_length, uint8_t num_responses, uint16_t max_delay,
                                          uint16_t min_delay, LegacyInquiryCallbacks callbacks) {
  pimpl_->RegisterInquiryCallbacks(callbacks);
  pimpl_->general_periodic_inquiry_active_ = true;
  pimpl_->module_->StartGeneralPeriodicInquiry(inquiry_length, num_responses, max_delay, min_delay);
}

void Inquiry::StartLimitedPeriodicInquiry(uint8_t inquiry_length, uint8_t num_responses, uint16_t max_delay,
                                          uint16_t min_delay, LegacyInquiryCallbacks callbacks) {
  pimpl_->RegisterInquiryCallbacks(callbacks);
  pimpl_->limited_periodic_inquiry_active_ = true;
  return pimpl_->module_->StartLimitedPeriodicInquiry(inquiry_length, num_responses, max_delay, min_delay);
}

void Inquiry::StopPeriodicInquiry() {
  pimpl_->limited_periodic_inquiry_active_ = false;
  pimpl_->general_periodic_inquiry_active_ = false;
  pimpl_->module_->StopPeriodicInquiry();
  pimpl_->UnregisterInquiryCallbacks();
}

bool Inquiry::IsGeneralPeriodicInquiryActive() const {
  return pimpl_->general_periodic_inquiry_active_;
}

bool Inquiry::IsLimitedPeriodicInquiryActive() const {
  return pimpl_->limited_periodic_inquiry_active_;
}

void Inquiry::SetInterlacedScan() {
  pimpl_->module_->SetInterlacedScan();
}

void Inquiry::SetStandardScan() {
  pimpl_->module_->SetStandardScan();
}

void Inquiry::SetScanActivity(uint16_t interval, uint16_t window) {
  pimpl_->params_.interval = interval;
  pimpl_->params_.window = window;
  pimpl_->module_->SetScanActivity(pimpl_->params_);
}

void Inquiry::GetScanActivity(uint16_t& interval, uint16_t& window) const {
  interval = static_cast<uint16_t>(pimpl_->params_.interval);
  window = static_cast<uint16_t>(pimpl_->params_.window);
}

void Inquiry::SetStandardInquiryResultMode() {
  pimpl_->module_->SetStandardInquiryResultMode();
}

void Inquiry::SetInquiryWithRssiResultMode() {
  pimpl_->module_->SetInquiryWithRssiResultMode();
}

void Inquiry::SetExtendedInquiryResultMode() {
  pimpl_->module_->SetExtendedInquiryResultMode();
}

/**
 * Module methods
 */
void Inquiry::ListDependencies(ModuleList* list) {
  list->add<neighbor::InquiryModule>();
}

void Inquiry::Start() {
  pimpl_ = std::make_unique<impl>(GetDependency<neighbor::InquiryModule>());
}

void Inquiry::Stop() {
  pimpl_.reset();
}

}  // namespace shim
}  // namespace bluetooth
