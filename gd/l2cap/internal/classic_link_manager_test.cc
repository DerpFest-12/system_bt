/*
 * Copyright 2018 The Android Open Source Project
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

#include "l2cap/internal/classic_link_manager.h"

#include <future>

#include "common/bind.h"
#include "common/testing/bind_test_util.h"
#include "hci/acl_manager_mock.h"
#include "hci/address.h"
#include "l2cap/cid.h"
#include "l2cap/classic_fixed_channel_manager.h"
#include "l2cap/internal/classic_fixed_channel_service_impl_mock.h"
#include "l2cap/internal/classic_fixed_channel_service_manager_impl_mock.h"
#include "os/handler.h"
#include "os/thread.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace bluetooth {
namespace l2cap {
namespace internal {

using hci::testing::MockAclConnection;
using hci::testing::MockAclManager;
using ::testing::_;  // Matcher to any value
using ::testing::ByMove;
using ::testing::DoAll;
using testing::MockClassicFixedChannelServiceImpl;
using testing::MockClassicFixedChannelServiceManagerImpl;
using ::testing::Return;
using ::testing::SaveArg;

class L2capClassicLinkManagerTest : public ::testing::Test {
 public:
  void OnFailCallback(ClassicFixedChannelManager::ConnectionResult result) {}

  static void SyncHandler(os::Handler* handler) {
    std::promise<void> promise;
    auto future = promise.get_future();
    handler->Post(common::BindOnce(&std::promise<void>::set_value, common::Unretained(&promise)));
    future.wait_for(std::chrono::milliseconds(3));
  }

 protected:
  void SetUp() override {
    thread_ = new os::Thread("test_thread", os::Thread::Priority::NORMAL);
    l2cap_handler_ = new os::Handler(thread_);
  }

  void TearDown() override {
    l2cap_handler_->Clear();
    delete l2cap_handler_;
    delete thread_;
  }

  os::Thread* thread_ = nullptr;
  os::Handler* l2cap_handler_ = nullptr;
};

TEST_F(L2capClassicLinkManagerTest, connect_fixed_channel_service_without_acl) {
  MockClassicFixedChannelServiceManagerImpl mock_classic_fixed_channel_service_manager;
  MockAclManager mock_acl_manager;
  hci::Address device{{0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
  auto user_handler = std::make_unique<os::Handler>(thread_);

  // Step 1: Verify callback registration with HCI
  hci::ConnectionCallbacks* hci_connection_callbacks = nullptr;
  os::Handler* hci_callback_handler = nullptr;
  EXPECT_CALL(mock_acl_manager, RegisterCallbacks(_, _))
      .WillOnce(DoAll(SaveArg<0>(&hci_connection_callbacks), SaveArg<1>(&hci_callback_handler), Return(true)));
  ClassicLinkManager classic_link_manager(l2cap_handler_, &mock_acl_manager,
                                          &mock_classic_fixed_channel_service_manager);
  EXPECT_EQ(hci_connection_callbacks, &classic_link_manager);
  EXPECT_EQ(hci_callback_handler, l2cap_handler_);

  // Register fake services
  MockClassicFixedChannelServiceImpl mock_service_1, mock_service_2;
  std::vector<std::pair<Cid, ClassicFixedChannelServiceImpl*>> results;
  results.emplace_back(kSmpBrCid, &mock_service_1);
  results.emplace_back(kConnectionlessCid, &mock_service_2);
  EXPECT_CALL(mock_classic_fixed_channel_service_manager, GetRegisteredServices()).WillRepeatedly(Return(results));

  // Step 2: Connect to fixed channel without ACL connection should trigger ACL connection process
  EXPECT_CALL(mock_acl_manager, CreateConnection(device)).Times(1);
  ClassicLinkManager::PendingFixedChannelConnection pending_fixed_channel_connection{
      .handler_ = user_handler.get(),
      .on_fail_callback_ = common::BindOnce([](ClassicFixedChannelManager::ConnectionResult result) { FAIL(); })};
  classic_link_manager.ConnectFixedChannelServices(device, std::move(pending_fixed_channel_connection));

  // Step 3: ACL connection success event should trigger channel creation for all registered services

  std::unique_ptr<MockAclConnection> acl_connection = std::make_unique<MockAclConnection>();
  hci::AclConnection::Queue link_queue{10};
  EXPECT_CALL(*acl_connection, GetAclQueueEnd()).WillRepeatedly((Return(link_queue.GetUpEnd())));
  EXPECT_CALL(*acl_connection, GetAddress()).WillRepeatedly(Return(device));
  std::unique_ptr<ClassicFixedChannel> channel_1, channel_2;
  EXPECT_CALL(mock_service_1, NotifyChannelCreation(_))
      .WillOnce([&channel_1](std::unique_ptr<ClassicFixedChannel> channel) { channel_1 = std::move(channel); });
  EXPECT_CALL(mock_service_2, NotifyChannelCreation(_))
      .WillOnce([&channel_2](std::unique_ptr<ClassicFixedChannel> channel) { channel_2 = std::move(channel); });
  hci_callback_handler->Post(common::BindOnce(&hci::ConnectionCallbacks::OnConnectSuccess,
                                              common::Unretained(hci_connection_callbacks), std::move(acl_connection)));
  SyncHandler(hci_callback_handler);
  EXPECT_NE(channel_1, nullptr);
  EXPECT_NE(channel_2, nullptr);

  // Step 4: Calling ConnectServices() to the same device will no trigger another connection attempt
  ClassicFixedChannelManager::ConnectionResult my_result;
  ClassicLinkManager::PendingFixedChannelConnection pending_fixed_channel_connection_2{
      .handler_ = user_handler.get(),
      .on_fail_callback_ = common::testing::BindLambdaForTesting(
          [&my_result](ClassicFixedChannelManager::ConnectionResult result) { my_result = result; })};
  classic_link_manager.ConnectFixedChannelServices(device, std::move(pending_fixed_channel_connection_2));
  SyncHandler(user_handler.get());
  EXPECT_EQ(my_result.connection_result_code,
            ClassicFixedChannelManager::ConnectionResultCode::FAIL_ALL_SERVICES_HAVE_CHANNEL);

  // Step 5: Register new service will cause new channels to be created during ConnectServices()
  MockClassicFixedChannelServiceImpl mock_service_3;
  results.emplace_back(kSmpBrCid + 1, &mock_service_3);
  EXPECT_CALL(mock_classic_fixed_channel_service_manager, GetRegisteredServices()).WillRepeatedly(Return(results));
  ClassicLinkManager::PendingFixedChannelConnection pending_fixed_channel_connection_3{
      .handler_ = user_handler.get(),
      .on_fail_callback_ = common::BindOnce([](ClassicFixedChannelManager::ConnectionResult result) { FAIL(); })};
  std::unique_ptr<ClassicFixedChannel> channel_3;
  EXPECT_CALL(mock_service_3, NotifyChannelCreation(_))
      .WillOnce([&channel_3](std::unique_ptr<ClassicFixedChannel> channel) { channel_3 = std::move(channel); });
  classic_link_manager.ConnectFixedChannelServices(device, std::move(pending_fixed_channel_connection_3));
  EXPECT_NE(channel_3, nullptr);

  user_handler->Clear();

  classic_link_manager.OnDisconnect(device, hci::ErrorCode::SUCCESS);
}

TEST_F(L2capClassicLinkManagerTest, connect_fixed_channel_service_without_acl_with_no_service) {
  MockClassicFixedChannelServiceManagerImpl mock_classic_fixed_channel_service_manager;
  MockAclManager mock_acl_manager;
  hci::Address device{{0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
  auto user_handler = std::make_unique<os::Handler>(thread_);

  // Step 1: Verify callback registration with HCI
  hci::ConnectionCallbacks* hci_connection_callbacks = nullptr;
  os::Handler* hci_callback_handler = nullptr;
  EXPECT_CALL(mock_acl_manager, RegisterCallbacks(_, _))
      .WillOnce(DoAll(SaveArg<0>(&hci_connection_callbacks), SaveArg<1>(&hci_callback_handler), Return(true)));
  ClassicLinkManager classic_link_manager(l2cap_handler_, &mock_acl_manager,
                                          &mock_classic_fixed_channel_service_manager);
  EXPECT_EQ(hci_connection_callbacks, &classic_link_manager);
  EXPECT_EQ(hci_callback_handler, l2cap_handler_);

  // Make sure no service is registered
  std::vector<std::pair<Cid, ClassicFixedChannelServiceImpl*>> results;
  EXPECT_CALL(mock_classic_fixed_channel_service_manager, GetRegisteredServices()).WillRepeatedly(Return(results));

  // Step 2: Connect to fixed channel without any service registered will result in failure
  EXPECT_CALL(mock_acl_manager, CreateConnection(device)).Times(0);
  ClassicFixedChannelManager::ConnectionResult my_result;
  ClassicLinkManager::PendingFixedChannelConnection pending_fixed_channel_connection{
      .handler_ = user_handler.get(),
      .on_fail_callback_ = common::testing::BindLambdaForTesting(
          [&my_result](ClassicFixedChannelManager::ConnectionResult result) { my_result = result; })};
  classic_link_manager.ConnectFixedChannelServices(device, std::move(pending_fixed_channel_connection));
  SyncHandler(user_handler.get());
  EXPECT_EQ(my_result.connection_result_code,
            ClassicFixedChannelManager::ConnectionResultCode::FAIL_NO_SERVICE_REGISTERED);

  user_handler->Clear();
}

TEST_F(L2capClassicLinkManagerTest, connect_fixed_channel_service_without_acl_with_hci_failure) {
  MockClassicFixedChannelServiceManagerImpl mock_classic_fixed_channel_service_manager;
  MockAclManager mock_acl_manager;
  hci::Address device{{0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
  auto user_handler = std::make_unique<os::Handler>(thread_);

  // Step 1: Verify callback registration with HCI
  hci::ConnectionCallbacks* hci_connection_callbacks = nullptr;
  os::Handler* hci_callback_handler = nullptr;
  EXPECT_CALL(mock_acl_manager, RegisterCallbacks(_, _))
      .WillOnce(DoAll(SaveArg<0>(&hci_connection_callbacks), SaveArg<1>(&hci_callback_handler), Return(true)));
  ClassicLinkManager classic_link_manager(l2cap_handler_, &mock_acl_manager,
                                          &mock_classic_fixed_channel_service_manager);
  EXPECT_EQ(hci_connection_callbacks, &classic_link_manager);
  EXPECT_EQ(hci_callback_handler, l2cap_handler_);

  // Register fake services
  MockClassicFixedChannelServiceImpl mock_service_1;
  std::vector<std::pair<Cid, ClassicFixedChannelServiceImpl*>> results;
  results.emplace_back(kSmpBrCid, &mock_service_1);
  EXPECT_CALL(mock_classic_fixed_channel_service_manager, GetRegisteredServices()).WillRepeatedly(Return(results));

  // Step 2: Connect to fixed channel without ACL connection should trigger ACL connection process
  EXPECT_CALL(mock_acl_manager, CreateConnection(device)).Times(1);
  ClassicFixedChannelManager::ConnectionResult my_result;
  ClassicLinkManager::PendingFixedChannelConnection pending_fixed_channel_connection{
      .handler_ = user_handler.get(),
      .on_fail_callback_ = common::testing::BindLambdaForTesting(
          [&my_result](ClassicFixedChannelManager::ConnectionResult result) { my_result = result; })};
  classic_link_manager.ConnectFixedChannelServices(device, std::move(pending_fixed_channel_connection));

  // Step 3: ACL connection failure event should trigger connection failure callback
  EXPECT_CALL(mock_service_1, NotifyChannelCreation(_)).Times(0);
  hci_callback_handler->Post(common::BindOnce(&hci::ConnectionCallbacks::OnConnectFail,
                                              common::Unretained(hci_connection_callbacks), device,
                                              hci::ErrorCode::PAGE_TIMEOUT));
  SyncHandler(hci_callback_handler);
  SyncHandler(user_handler.get());
  EXPECT_EQ(my_result.connection_result_code, ClassicFixedChannelManager::ConnectionResultCode::FAIL_HCI_ERROR);
  EXPECT_EQ(my_result.hci_error, hci::ErrorCode::PAGE_TIMEOUT);

  user_handler->Clear();
}

}  // namespace internal
}  // namespace l2cap
}  // namespace bluetooth
