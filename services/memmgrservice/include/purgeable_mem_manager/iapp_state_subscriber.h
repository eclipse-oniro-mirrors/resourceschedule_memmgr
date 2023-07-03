/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEMORY_MEMMGR_IAPP_STATE_SUBSCRIBER_H
#define OHOS_MEMORY_MEMMGR_IAPP_STATE_SUBSCRIBER_H

#include <ipc_types.h>
#include <iremote_broker.h>
#include <nocopyable.h>

#include "errors.h"
#include "memmgrservice_ipc_interface_code.h"
#include "memory_level_constants.h"
#include "refbase.h"


namespace OHOS {
namespace Memory {
class IAppStateSubscriber : public IRemoteBroker {
public:
    IAppStateSubscriber() = default;
    ~IAppStateSubscriber() override = default;
    DISALLOW_COPY_AND_MOVE(IAppStateSubscriber);

    /* *
     * @brief Called back when the subscriber is connected to Memory Manager Service.
     */
    virtual void OnConnected() = 0;

    /* *
     * @brief Called back when the subscriber is disconnected to Memory Manager Service.
     */
    virtual void OnDisconnected() = 0;

    /* *
     * @brief Called back when app state change.
     *
     * @param pid pid of the process whose state is changed.
     * @param uid uid of the process whose state is changed.
     * @param state new state of the app.
     */
    virtual void OnAppStateChanged(int32_t pid, int32_t uid, int32_t state) = 0;

    /* *
     * @brief Called back when need to reclaim memory.
     *
     * @param pid pid of the process which need to reclaim.
     * @param uid uid of the process which need to reclaim.
     */
    virtual void ForceReclaim(int32_t pid, int32_t uid) = 0;

    /* *
     * @brief Called back when get systemMemoryLevel message.
     *
     * @param level current memory level.
     */
    virtual void OnTrim(SystemMemoryLevel level) = 0;

public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.resourceschedule.IAppStateSubscriber");
};
} // namespace Memory
} // namespace OHOS
#endif // OHOS_MEMORY_MEMMGR_IAPP_STATE_SUBSCRIBER_H
