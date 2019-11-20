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

#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "stack/include/btm_api_types.h"

/* Discoverable modes */
static constexpr int kDiscoverableModeOff = 0;
static constexpr int kLimitedDiscoverableMode = 1;
static constexpr int kGeneralDiscoverableMode = 2;

/* Inquiry modes */
// NOTE: The inquiry general/limited are reversed from the discoverability
// constants
static constexpr int kInquiryModeOff = 0;
static constexpr int kGeneralInquiryMode = 1;
static constexpr int kLimitedInquiryMode = 2;

/* Connectable modes */
static constexpr int kConnectibleModeOff = 0;
static constexpr int kConnectibleModeOn = 1;

/* Inquiry and page scan modes */
static constexpr int kStandardScanType = 0;
static constexpr int kInterlacedScanType = 1;

/* Inquiry result modes */
static constexpr int kStandardInquiryResult = 0;
static constexpr int kInquiryResultWithRssi = 1;
static constexpr int kExtendedInquiryResult = 2;

/* Inquiry filter types */
static constexpr int kClearInquiryFilter = 0;
static constexpr int kFilterOnDeviceClass = 1;
static constexpr int kFilterOnAddress = 2;

using DiscoverabilityState = struct {
  int mode;
  uint16_t interval;
  uint16_t window;
};
using ConnectabilityState = DiscoverabilityState;

namespace bluetooth {
namespace shim {

using BtmStatus = enum : uint16_t {
  BTM_SUCCESS = 0,         /* 0  Command succeeded                 */
  BTM_CMD_STARTED,         /* 1  Command started OK.               */
  BTM_BUSY,                /* 2  Device busy with another command  */
  BTM_NO_RESOURCES,        /* 3  No resources to issue command     */
  BTM_MODE_UNSUPPORTED,    /* 4  Request for 1 or more unsupported modes */
  BTM_ILLEGAL_VALUE,       /* 5  Illegal parameter value           */
  BTM_WRONG_MODE,          /* 6  Device in wrong mode for request  */
  BTM_UNKNOWN_ADDR,        /* 7  Unknown remote BD address         */
  BTM_DEVICE_TIMEOUT,      /* 8  Device timeout                    */
  BTM_BAD_VALUE_RET,       /* 9  A bad value was received from HCI */
  BTM_ERR_PROCESSING,      /* 10 Generic error                     */
  BTM_NOT_AUTHORIZED,      /* 11 Authorization failed              */
  BTM_DEV_RESET,           /* 12 Device has been reset             */
  BTM_CMD_STORED,          /* 13 request is stored in control block */
  BTM_ILLEGAL_ACTION,      /* 14 state machine gets illegal command */
  BTM_DELAY_CHECK,         /* 15 delay the check on encryption */
  BTM_SCO_BAD_LENGTH,      /* 16 Bad SCO over HCI data length */
  BTM_SUCCESS_NO_SECURITY, /* 17 security passed, no security set  */
  BTM_FAILED_ON_SECURITY,  /* 18 security failed                   */
  BTM_REPEATED_ATTEMPTS,   /* 19 repeated attempts for LE security requests */
  BTM_MODE4_LEVEL4_NOT_SUPPORTED, /* 20 Secure Connections Only Mode can't be
                                     supported */
  BTM_DEV_BLACKLISTED             /* 21 The device is Blacklisted */
};

class ReadRemoteName {
 public:
  bool Start(RawAddress raw_address) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (in_progress_) {
      return false;
    }
    raw_address_ = raw_address;
    in_progress_ = true;
    return true;
  }

  void Stop() {
    std::unique_lock<std::mutex> lock(mutex_);
    raw_address_ = RawAddress::kEmpty;
    in_progress_ = false;
  }

  bool IsInProgress() const { return in_progress_; }

  std::string AddressString() const { return raw_address_.ToString(); }

  ReadRemoteName() : in_progress_{false}, raw_address_(RawAddress::kEmpty) {}

 private:
  bool in_progress_;
  RawAddress raw_address_;
  std::mutex mutex_;
};

class Btm {
 public:
  Btm();

  // Callbacks
  void OnInquiryResult(std::vector<const uint8_t> result);
  void OnInquiryResultWithRssi(std::vector<const uint8_t> result);
  void OnExtendedInquiryResult(std::vector<const uint8_t> result);
  void OnInquiryComplete(uint16_t status);

  // Inquiry API
  bool SetInquiryFilter(uint8_t mode, uint8_t type, tBTM_INQ_FILT_COND data);
  void SetFilterInquiryOnAddress();
  void SetFilterInquiryOnDevice();
  void ClearInquiryFilter();

  bool SetStandardInquiryResultMode();
  bool SetInquiryWithRssiResultMode();
  bool SetExtendedInquiryResultMode();

  void SetInterlacedInquiryScan();
  void SetStandardInquiryScan();
  bool IsInterlacedScanSupported() const;

  bool StartInquiry(uint8_t mode, uint8_t duration, uint8_t max_responses);
  void CancelInquiry();
  bool IsInquiryActive() const;
  bool IsGeneralInquiryActive() const;
  bool IsLimitedInquiryActive() const;

  bool StartPeriodicInquiry(uint8_t mode, uint8_t duration,
                            uint8_t max_responses, uint16_t max_delay,
                            uint16_t min_delay,
                            tBTM_INQ_RESULTS_CB* p_results_cb);
  void CancelPeriodicInquiry();
  bool IsGeneralPeriodicInquiryActive() const;
  bool IsLimitedPeriodicInquiryActive() const;

  void SetClassicGeneralDiscoverability(uint16_t window, uint16_t interval);
  void SetClassicLimitedDiscoverability(uint16_t window, uint16_t interval);
  void SetClassicDiscoverabilityOff();
  DiscoverabilityState GetClassicDiscoverabilityState() const;

  void SetLeGeneralDiscoverability();
  void SetLeLimitedDiscoverability();
  void SetLeDiscoverabilityOff();
  DiscoverabilityState GetLeDiscoverabilityState() const;

  void SetClassicConnectibleOn();
  void SetClassicConnectibleOff();
  ConnectabilityState GetClassicConnectabilityState() const;
  void SetInterlacedPageScan();
  void SetStandardPageScan();

  void SetLeConnectibleOn();
  void SetLeConnectibleOff();
  ConnectabilityState GetLeConnectabilityState() const;

  bool IsLeAclConnected(const RawAddress& raw_address) const;

  // Remote device name
  BtmStatus ReadClassicRemoteDeviceName(const RawAddress& raw_address,
                                        tBTM_CMPL_CB* callback);
  BtmStatus ReadLeRemoteDeviceName(const RawAddress& raw_address,
                                   tBTM_CMPL_CB* callback);
  BtmStatus CancelAllReadRemoteDeviceName();

 private:
  ReadRemoteName le_read_remote_name_;
  ReadRemoteName classic_read_remote_name_;
  // TODO(cmanton) abort if there is no classic acl link up
  bool CheckClassicAclLink(const RawAddress& raw_address) { return true; }
  bool CheckLeAclLink(const RawAddress& raw_address) { return true; }
};

}  // namespace shim
}  // namespace bluetooth
