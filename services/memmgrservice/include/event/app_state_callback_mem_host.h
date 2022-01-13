/*
 * Copyright (c) 2021 XXXX.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_MEMORY_MEMMGR_EVENT_MEM_HOST_H
#define OHOS_MEMORY_MEMMGR_EVENT_MEM_HOST_H

#include "single_instance.h"
#include "memmgr_log.h"
#include <string>

#include "app_state_callback_host.h"
#include "app_process_data.h"
#include "app_mgr_client.h"

namespace OHOS {
namespace Memory {
class AppStateCallbackMemHost : public AppExecFwk::AppStateCallbackHost {
public:
    AppStateCallbackMemHost();
    ~AppStateCallbackMemHost();
    bool ConnectAppMgrService();
    bool Register();
protected:
    virtual void OnAppStateChanged(const AppExecFwk::AppProcessData &) override;
private:
    std::unique_ptr<AppExecFwk::AppMgrClient> appMgrClient_;
};
} // namespace Memory
} // namespace OHOS
#endif // OHOS_MEMORY_MEMMGR_EVENT_MEM_HOST_H
