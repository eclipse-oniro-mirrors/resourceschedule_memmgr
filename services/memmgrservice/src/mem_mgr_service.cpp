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

#include "mem_mgr_service.h"

#include <unistd.h>

#include "ipc_skeleton.h"
#include "low_memory_killer.h"
#include "mem_mgr_event_center.h"
#include "memmgr_config_manager.h"
#include "memmgr_log.h"
#include "multi_account_manager.h"
#include "nandlife_controller.h"
#include "reclaim_priority_manager.h"
#include "reclaim_strategy_manager.h"
#include "system_ability_definition.h"
#include "window_visibility_observer.h"
#ifdef USE_PURGEABLE_MEMORY
#include "kernel_interface.h"
#include "purgeable_mem_manager.h"
#endif
#include "dump_command_dispatcher.h"

namespace OHOS {
namespace Memory {
namespace {
const std::string TAG = "MemMgrService";
}

IMPLEMENT_SINGLE_INSTANCE(MemMgrService);
const bool REGISTER_RESULT = SystemAbility::MakeAndRegisterAbility(&MemMgrService::GetInstance());

MemMgrService::MemMgrService() : SystemAbility(MEMORY_MANAGER_SA_ID, true)
{
}

bool MemMgrService::Init()
{
    MemmgrConfigManager::GetInstance().Init();

    // init reclaim priority manager
    if (!ReclaimPriorityManager::GetInstance().Init()) {
        HILOGE("ReclaimPriorityManager init failed");
        return false;
    }

    // init multiple account manager
    MultiAccountManager::GetInstance().Init();

    // init reclaim strategy manager
    if (!ReclaimStrategyManager::GetInstance().Init()) {
        HILOGE("ReclaimStrategyManager init failed");
        return false;
    }

    // init event center, then managers above can work by event trigger
    if (!MemMgrEventCenter::GetInstance().Init()) {
        HILOGE("MemMgrEventCenter init failed");
        return false;
    }

    // init nandlife controller
    NandLifeController::GetInstance().Init();
    HILOGI("init successed");
    return true;
}

void MemMgrService::OnStart()
{
    HILOGI("called");
    if (!Init()) {
        HILOGE("init failed");
        return;
    }
    if (!Publish(this)) {
        HILOGE("publish SA failed");
        return;
    }
    HILOGI("publish SA successed");

    AddSystemAbilityListener(COMMON_EVENT_SERVICE_ID);
    AddSystemAbilityListener(COMMON_EVENT_SERVICE_ABILITY_ID);
    AddSystemAbilityListener(BACKGROUND_TASK_MANAGER_SERVICE_ID);
    AddSystemAbilityListener(SUBSYS_ACCOUNT_SYS_ABILITY_ID_BEGIN);
    AddSystemAbilityListener(SUBSYS_APPLICATIONS_SYS_ABILITY_ID_BEGIN);
    AddSystemAbilityListener(APP_MGR_SERVICE_ID);
    AddSystemAbilityListener(ABILITY_MGR_SERVICE_ID);
}

void MemMgrService::OnStop()
{
    HILOGI("called");
}

// implements of innerkits list below

int32_t MemMgrService::GetBundlePriorityList(BundlePriorityList &bundlePrioList)
{
    HILOGI("called");
    ReclaimPriorityManager::BunldeCopySet bundleSet;
    ReclaimPriorityManager::GetInstance().GetBundlePrioSet(bundleSet);
    for (auto bundlePriorityInfo : bundleSet) {
        Memory::BundlePriority bi = Memory::BundlePriority(bundlePriorityInfo.uid_,
            bundlePriorityInfo.name_, bundlePriorityInfo.priority_, bundlePriorityInfo.accountId_);
        bundlePrioList.AddBundleInfo(bi);
    }
    bundlePrioList.SetCount(bundlePrioList.Size());
    return 0;
}

int32_t MemMgrService::NotifyDistDevStatus(int32_t pid, int32_t uid, const std::string &name, bool connected)
{
    HILOGI("called, pid=%{public}d, uid=%{public}d, name=%{public}s, connected=%{public}d", pid, uid, name.c_str(),
        connected);
    ReclaimHandleRequest request;
    request.pid = pid;
    request.uid = uid;
    request.bundleName = name;
    request.reason =
        connected ? AppStateUpdateReason::DIST_DEVICE_CONNECTED : AppStateUpdateReason::DIST_DEVICE_DISCONNECTED;
    ReclaimPriorityManager::GetInstance().UpdateReclaimPriority(
        SingleRequest({pid, uid, "", name},
            connected ? AppStateUpdateReason::DIST_DEVICE_CONNECTED : AppStateUpdateReason::DIST_DEVICE_DISCONNECTED));
    return 0;
}

int32_t MemMgrService::GetKillLevelOfLmkd(int32_t &killLevel)
{
    HILOGI("called");
    killLevel = LowMemoryKiller::GetInstance().GetKillLevel();
    return 0;
}

#ifdef USE_PURGEABLE_MEMORY
int32_t MemMgrService::RegisterActiveApps(int32_t pid, int32_t uid)
{
    HILOGI("called, pid=%{public}d, uid=%{public}d", pid, uid);
    PurgeableMemManager::GetInstance().RegisterActiveApps(pid, uid);
    return 0;
}

int32_t MemMgrService::DeregisterActiveApps(int32_t pid, int32_t uid)
{
    HILOGI("called, pid=%{public}d, uid=%{public}d", pid, uid);
    PurgeableMemManager::GetInstance().DeregisterActiveApps(pid, uid);
    return 0;
}

int32_t MemMgrService::SubscribeAppState(const sptr<IAppStateSubscriber> &subscriber)
{
    HILOGI("called");
    PurgeableMemManager::GetInstance().AddSubscriber(subscriber);
    return 0;
}

int32_t MemMgrService::UnsubscribeAppState(const sptr<IAppStateSubscriber> &subscriber)
{
    HILOGI("called");
    PurgeableMemManager::GetInstance().RemoveSubscriber(subscriber);
    return 0;
}

int32_t MemMgrService::GetAvailableMemory(int32_t &memSize)
{
    HILOGI("called");
    memSize = KernelInterface::GetInstance().GetCurrentBuffer();
    if (memSize < 0 || memSize >= MAX_BUFFER_KB) {
        return -1;
    }
    return 0;
}

int32_t MemMgrService::GetTotalMemory(int32_t &memSize)
{
    HILOGI("called");
    memSize = KernelInterface::GetInstance().GetTotalBuffer();
    if (memSize < 0 || memSize >= MAX_BUFFER_KB) {
        return -1;
    }
    return 0;
}
#endif // USE_PURGEABLE_MEMORY

void MemMgrService::OnAddSystemAbility(int32_t systemAbilityId, const std::string& deviceId)
{
    HILOGI("systemAbilityId: %{public}d add", systemAbilityId);
    MemMgrEventCenter::GetInstance().RetryRegisterEventObserver(systemAbilityId);
}

int32_t MemMgrService::OnWindowVisibilityChanged(const std::vector<sptr<MemMgrWindowInfo>> &MemMgrWindowInfo)
{
    HILOGI("called");
    int32_t callingUid = IPCSkeleton::GetCallingUid();
    if (callingUid != windowManagerUid_) {
        HILOGE("OnWindowVisibilityChanged refused for%{public}d", callingUid);
        return -1;
    }
    HILOGI("OnWindowVisibilityChanged called %{public}d", callingUid);
    WindowVisibilityObserver::GetInstance().UpdateWindowVisibilityPriority(MemMgrWindowInfo);
    return 0;
}

int32_t MemMgrService::GetReclaimPriorityByPid(int32_t pid, int32_t &priority)
{
    HILOGI("called");
    std::string path = KernelInterface::GetInstance().JoinPath("/proc/", std::to_string(pid), "/oom_score_adj");
    std::string contentStr;
    if (KernelInterface::GetInstance().ReadFromFile(path, contentStr) || contentStr.size() == 0) {
        HILOGE("read %{public}s failed, content=[%{public}s]", path.c_str(), contentStr.c_str());
        return -1;
    }
    HILOGD("read %{public}s succ, content=[%{public}s]", path.c_str(), contentStr.c_str());

    try {
        priority = std::stoi(contentStr);
    } catch (std::out_of_range&) {
        HILOGW("stoi() failed: out_of_range");
        return -1;
    } catch (std::invalid_argument&) {
        HILOGW("stoi() failed: invalid_argument");
        return -1;
    }
    return 0;
}


int32_t MemMgrService::NotifyProcessStateChangedSync(const MemMgrProcessStateInfo &processStateInfo)
{
    HILOGD("called");
    if (processStateInfo.reason_ == ProcPriorityUpdateReason::START_ABILITY) {
        HILOGD("callerpid=%{public}d,calleruid=%{public}d,pid=%{public}d,uid=%{public}d,reason=%{public}u",
            processStateInfo.callerPid_, processStateInfo.callerUid_, processStateInfo.pid_, processStateInfo.uid_,
            static_cast<uint32_t>(processStateInfo.reason_));
        UpdateRequest request = CallerRequest({processStateInfo.callerPid_, processStateInfo.callerUid_, "", ""},
            {processStateInfo.pid_, processStateInfo.uid_, "", ""}, AppStateUpdateReason::ABILITY_START);
        if (!ReclaimPriorityManager::GetInstance().UpdateRecalimPrioritySyncWithLock(request)) {
            HILOGE("NotifyProcessStateChangedSync <pid=%{public}d,uid=%{public}d,reason=%{public}u> failed",
                processStateInfo.pid_, processStateInfo.uid_, static_cast<uint32_t>(processStateInfo.reason_));
            return static_cast<int32_t>(MemMgrErrorCode::MEMMGR_SERVICE_ERR);
        }
    }
    return 0;
}

int32_t MemMgrService::NotifyProcessStateChangedAsync(const MemMgrProcessStateInfo &processStateInfo)
{
    HILOGD("called");
    if (processStateInfo.reason_ == ProcPriorityUpdateReason::START_ABILITY) {
        HILOGD("callerpid=%{public}d,calleruid=%{public}d,pid=%{public}d,uid=%{public}d,reason=%{public}u",
            processStateInfo.callerPid_, processStateInfo.callerUid_, processStateInfo.pid_, processStateInfo.uid_,
            static_cast<uint32_t>(processStateInfo.reason_));
        UpdateRequest request = CallerRequest({processStateInfo.callerPid_, processStateInfo.callerUid_, "", ""},
            {processStateInfo.pid_, processStateInfo.uid_, "", ""}, AppStateUpdateReason::ABILITY_START);
        if (!ReclaimPriorityManager::GetInstance().UpdateReclaimPriority(request)) {
            HILOGE("NotifyProcessStateChangedAsync <pid=%{public}d,uid=%{public}d,reason=%{public}u> failed",
                processStateInfo.pid_, processStateInfo.uid_, static_cast<uint32_t>(processStateInfo.reason_));
            return static_cast<int32_t>(MemMgrErrorCode::MEMMGR_SERVICE_ERR);
        }
    }
    return 0;
}

int32_t MemMgrService::NotifyProcessStatus(int32_t pid, int32_t type, int32_t status, int32_t saId)
{
    HILOGI("pid=%{public}d,type=%{public}d,status=%{public}d,saId=%{public}d",
        pid, type, status, saId);
    return 0;
}

int32_t MemMgrService::SetCritical(int32_t pid, bool critical, int32_t saId)
{
    HILOGI("pid=%{public}d,critical=%{public}d,saId=%{public}d", pid, critical, saId);
    return 0;
}

void MemMgrService::OnRemoveSystemAbility(int32_t systemAbilityId, const std::string& deviceId)
{
    HILOGI("systemAbilityId: %{public}d add", systemAbilityId);
    MemMgrEventCenter::GetInstance().RemoveEventObserver(systemAbilityId);
}

void ParseParams(const std::vector<std::string> &params,
                 std::map<std::string, std::vector<std::string>> &keyValuesMapping)
{
    std::string tmpKey;
    std::vector<std::string> tmpValue;
    for (auto i = 0; i < params.size(); i++) {
        if (params[i].empty())
            continue;
        if (params[i][0] == '-') {
            if (!tmpKey.empty()) {
                keyValuesMapping[tmpKey] = tmpValue;
                tmpValue.clear();
            }
            tmpKey = params[i];
        } else {
            tmpValue.emplace_back(params[i]);
        }
    }
    if (!tmpKey.empty()) {
        keyValuesMapping[tmpKey] = tmpValue;
    }

    HILOGD("keyValuesMapping.size()=%{public}zu\n", keyValuesMapping.size());
    for (auto &it : keyValuesMapping) {
        HILOGD("key=%{public}s", it.first.c_str());
        for (auto i = 0; i < it.second.size(); i++) {
            HILOGD("value[%{public}d]=%{public}s", i, it.second[i].c_str());
        }
    }
}

int MemMgrService::Dump(int fd, const std::vector<std::u16string> &args)
{
    HILOGI("called");
    std::vector<std::string> params;
    for (auto &arg : args) {
        params.emplace_back(Str16ToStr8(arg));
    }
    std::map<std::string, std::vector<std::string>> keyValuesMapping;
    ParseParams(params, keyValuesMapping);
    DispatchDumpCommand(fd, keyValuesMapping);
    return 0;
}
} // namespace Memory
} // namespace OHOS
