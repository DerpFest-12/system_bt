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

#define LOG_TAG "smp"

#include <memory>
#include "module.h"
#include "os/handler.h"
#include "os/log.h"

#include "l2cap/classic/l2cap_classic_module.h"
#include "l2cap/le/l2cap_le_module.h"
#include "security/internal/security_manager_impl.h"
#include "security/security_module.h"

namespace bluetooth {
namespace security {

const ModuleFactory SecurityModule::Factory = ModuleFactory([]() { return new SecurityModule(); });

struct SecurityModule::impl {
  impl(os::Handler* security_handler, l2cap::le::L2capLeModule* l2cap_le_module,
       l2cap::classic::L2capClassicModule* l2cap_classic_module)
      : security_handler_(security_handler), l2cap_le_module_(l2cap_le_module),
        l2cap_classic_module_(l2cap_classic_module) {}
  os::Handler* security_handler_;
  l2cap::le::L2capLeModule* l2cap_le_module_;
  l2cap::classic::L2capClassicModule* l2cap_classic_module_;
  internal::SecurityManagerImpl security_manager_impl{security_handler_, l2cap_le_module_, l2cap_classic_module_};
};

void SecurityModule::ListDependencies(ModuleList* list) {
  list->add<l2cap::le::L2capLeModule>();
  list->add<l2cap::classic::L2capClassicModule>();
}

void SecurityModule::Start() {
  pimpl_ = std::make_unique<impl>(GetHandler(), GetDependency<l2cap::le::L2capLeModule>(),
                                  GetDependency<l2cap::classic::L2capClassicModule>());
}

void SecurityModule::Stop() {
  pimpl_.reset();
}

std::unique_ptr<SecurityManager> SecurityModule::GetSecurityManager() {
  return std::unique_ptr<SecurityManager>(
      new SecurityManager(pimpl_->security_handler_, &pimpl_->security_manager_impl));
}

}  // namespace security
}  // namespace bluetooth