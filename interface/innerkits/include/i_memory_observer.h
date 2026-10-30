/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEMORY_MEMMGR_INTERFACES_INNERKITS_INCLUDE_I_MEMORY_OBSERVER_H
#define OHOS_MEMORY_MEMMGR_INTERFACES_INNERKITS_INCLUDE_I_MEMORY_OBSERVER_H

#include "iremote_broker.h"
#include "iremote_object.h"

namespace OHOS {
namespace Memory {

enum class IMemoryObserverCode : uint32_t {
    ON_PROCESS_AVAILABLE = 2,
};

class IMemoryObserver : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.memory.IMemoryObserver");
    virtual void OnProcessAvailable(bool isAvailable) = 0;
};
}
}

#endif