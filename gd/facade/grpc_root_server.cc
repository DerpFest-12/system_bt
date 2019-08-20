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

#include "facade/grpc_root_server.h"

#include <string>

#include "facade/rootservice.grpc.pb.h"
#include "grpc/grpc_module.h"
#include "hal/facade.h"
#include "hci/facade.h"
#include "l2cap/facade.h"
#include "os/log.h"
#include "os/thread.h"
#include "stack_manager.h"

namespace bluetooth {
namespace facade {

using ::bluetooth::grpc::GrpcModule;
using ::bluetooth::os::Thread;

namespace {
class RootFacadeService : public ::bluetooth::facade::RootFacade::Service {
 public:
  RootFacadeService(int grpc_port) : grpc_port_(grpc_port) {}

  ::grpc::Status StartStack(::grpc::ServerContext* context, const ::bluetooth::facade::StartStackRequest* request,
                            ::bluetooth::facade::StartStackResponse* response) override {
    if (is_running_) {
      return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "stack is running");
    }

    ModuleList modules;
    modules.add<::bluetooth::grpc::GrpcModule>();

    BluetoothModule module_under_test = request->module_under_test();
    switch (module_under_test) {
      case BluetoothModule::HAL:
        modules.add<::bluetooth::hal::HciHalFacadeModule>();
        break;
      case BluetoothModule::HCI:
        modules.add<::bluetooth::hci::AclManagerFacadeModule>();
        modules.add<::bluetooth::hci::ClassicSecurityManagerFacadeModule>();
        break;
      case BluetoothModule::L2CAP:
        modules.add<::bluetooth::l2cap::L2capModuleFacadeModule>();
        break;
      default:
        return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "invalid module under test");
    }

    stack_thread_ = new Thread("stack_thread", Thread::Priority::NORMAL);
    stack_manager_.StartUp(&modules, stack_thread_);

    GrpcModule* grpc_module = stack_manager_.GetInstance<GrpcModule>();
    grpc_module->StartServer("0.0.0.0", grpc_port_);

    grpc_loop_thread_ = new std::thread([grpc_module] { grpc_module->RunGrpcLoop(); });
    is_running_ = true;

    return ::grpc::Status::OK;
  }

  ::grpc::Status StopStack(::grpc::ServerContext* context, const ::bluetooth::facade::StopStackRequest* request,
                           ::bluetooth::facade::StopStackResponse* response) override {
    if (!is_running_) {
      return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "stack is not running");
    }

    stack_manager_.GetInstance<GrpcModule>()->StopServer();
    grpc_loop_thread_->join();

    stack_manager_.ShutDown();
    delete stack_thread_;
    is_running_ = false;
    return ::grpc::Status::OK;
  }

 private:
  Thread* stack_thread_ = nullptr;
  bool is_running_ = false;
  std::thread* grpc_loop_thread_ = nullptr;
  StackManager stack_manager_;
  int grpc_port_ = 8898;
};

RootFacadeService* root_facade_service;
}  // namespace

void GrpcRootServer::StartServer(const std::string& address, int grpc_root_server_port, int grpc_port) {
  ASSERT(!started_);
  started_ = true;

  std::string listening_port = address + ":" + std::to_string(grpc_root_server_port);
  ::grpc::ServerBuilder builder;
  root_facade_service = new RootFacadeService(grpc_port);
  builder.RegisterService(root_facade_service);
  builder.AddListeningPort(listening_port, ::grpc::InsecureServerCredentials());
  server_ = builder.BuildAndStart();

  ASSERT(server_ != nullptr);
}

void GrpcRootServer::StopServer() {
  ASSERT(started_);
  server_->Shutdown();
  started_ = false;
  server_.reset();
  delete root_facade_service;
}

void GrpcRootServer::RunGrpcLoop() {
  ASSERT(started_);
  server_->Wait();
}

}  // namespace facade
}  // namespace bluetooth
