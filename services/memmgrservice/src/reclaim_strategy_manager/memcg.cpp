/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this->file except in compliance with the License.
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

#include <regex>

#include "memmgr_log.h"
#include "kernel_interface.h"
#include "reclaim_strategy_constants.h"
#include "memcg.h"

namespace OHOS {
namespace Memory {
namespace {
const std::string TAG = "Memcg";
} // namespace

SwapInfo::SwapInfo()
    : swapOutCount_(0),
      swapOutSize_(0),
      swapInCount_(0),
      swapInSize_(0),
      pageInCount_(0),
      swapSizeCurr_(0),
      swapSizeMax_(0) {}

SwapInfo::SwapInfo(unsigned int swapOutCount, unsigned int swapOutSize, unsigned int swapInCount,
                   unsigned int swapInSize, unsigned int pageInCount, unsigned int swapSizeCurr,
                   unsigned int swapSizeMax)
    : swapOutCount_(swapOutCount),
      swapOutSize_(swapOutSize),
      swapInCount_(swapInCount),
      swapInSize_(swapInSize),
      pageInCount_(pageInCount),
      swapSizeCurr_(swapSizeCurr),
      swapSizeMax_(swapSizeMax) {}

inline std::string SwapInfo::ToString() const
{
    std::string ret = "swapOutCount:" + std::to_string(swapOutCount_)
                    + " swapOutSize:" + std::to_string(swapOutSize_)
                    + " swapInCount:" + std::to_string(swapInCount_)
                    + " swapInSize:" + std::to_string(swapInSize_)
                    + " pageInCount:" + std::to_string(pageInCount_)
                    + " swapSizeCurr:" + std::to_string(swapSizeCurr_)
                    + " swapSizeMax:" + std::to_string(swapSizeMax_);
    return ret;
}

MemInfo::MemInfo() : anonKiB_(0), zramKiB_(0), eswapKiB_(0) {}

MemInfo::MemInfo(unsigned int anonKiB, unsigned int zramKiB, unsigned int eswapKiB)
    : anonKiB_(anonKiB),
      zramKiB_(zramKiB),
      eswapKiB_(eswapKiB) {}

inline std::string MemInfo::ToString() const
{
    std::string ret = "anonKiB:" + std::to_string(anonKiB_)
                    + " zramKiB:" + std::to_string(zramKiB_)
                    + " eswapKiB:" + std::to_string(eswapKiB_);
    return ret;
}

ReclaimRatios::ReclaimRatios()
    : mem2zramRatio_(MEMCG_MEM_2_ZRAM_RATIO),
      zram2ufsRatio_(MEMCG_ZRAM_2_UFS_RATIO),
      refaultThreshold_(MEMCG_REFAULT_THRESHOLD) {}

ReclaimRatios::ReclaimRatios(unsigned int mem2zramRatio, unsigned int zram2ufsRatio, unsigned int refaultThreshold)
    : mem2zramRatio_(mem2zramRatio),
      zram2ufsRatio_(zram2ufsRatio),
      refaultThreshold_(refaultThreshold) {}

void ReclaimRatios::SetRatios(unsigned int mem2zramRatio, unsigned int zram2ufsRatio, unsigned int refaultThreshold)
{
    mem2zramRatio_ = mem2zramRatio;
    zram2ufsRatio_ = zram2ufsRatio;
    refaultThreshold_ = refaultThreshold;
}

bool ReclaimRatios::SetRatios(ReclaimRatios * const ratios)
{
    if (ratios == nullptr) {
        return false;
    }
    SetRatios(ratios->mem2zramRatio_, ratios->zram2ufsRatio_, ratios->refaultThreshold_);
    return true;
}

inline std::string ReclaimRatios::NumsToString() const
{
    std::string ret = std::to_string(mem2zramRatio_) + " "
                    + std::to_string(zram2ufsRatio_) + " "
                    + std::to_string(refaultThreshold_);
    return ret;
}

std::string ReclaimRatios::ToString() const
{
    std::string ret = "mem2zramRatio:" + std::to_string(mem2zramRatio_)
                    + " zram2ufsRatio:" + std::to_string(zram2ufsRatio_)
                    + " refaultThreshold:" + std::to_string(refaultThreshold_);
    return ret;
}

Memcg::Memcg() : score_(0)
{
    swapInfo_ = new SwapInfo();
    memInfo_ = new MemInfo();
    reclaimRatios_ = new ReclaimRatios();
    HILOGI("init memcg success");
}

Memcg::~Memcg()
{
    delete swapInfo_;
    delete memInfo_;
    delete reclaimRatios_;
    HILOGI("release memcg success");
}

void Memcg::UpdateSwapInfoFromKernel()
{
    std::string path = KernelInterface::GetInstance().JoinPath(GetMemcgPath_(), "memory.eswap_stat");
    std::string content;
    if (!KernelInterface::GetInstance().ReadFromFile(path, content)) {
        return;
    }
    content = std::regex_replace(content, std::regex("\n+"), " "); // replace \n with space
    std::regex re(".*swapOutTotal:([[:d:]]+)[[:s:]]*"
                  "swapOutSize:([[:d:]]*) MB[[:s:]]*"
                  "swapInSize:([[:d:]]*) MB[[:s:]]*"
                  "swapInTotal:([[:d:]]*)[[:s:]]*"
                  "pageInTotal:([[:d:]]*)[[:s:]]*"
                  "swapSizeCur:([[:d:]]*) MB[[:s:]]*"
                  "swapSizeMax:([[:d:]]*) MB[[:s:]]*");
    std::smatch res;
    if (std::regex_match(content, res, re)) {
        swapInfo_->swapOutCount_ = std::stoi(res.str(1)); // 1: swapOutCount index
        swapInfo_->swapOutSize_ = std::stoi(res.str(2)); // 2: swapOutSize index
        swapInfo_->swapInSize_ = std::stoi(res.str(3)); // 3: swapInSize index
        swapInfo_->swapInCount_ = std::stoi(res.str(4)); // 4: swapInCount index
        swapInfo_->pageInCount_ = std::stoi(res.str(5)); // 5: pageInCount index
        swapInfo_->swapSizeCurr_ = std::stoi(res.str(6)); // 6: swapSizeCurr index
        swapInfo_->swapSizeMax_ = std::stoi(res.str(7)); // 7: swapSizeMax index
    }
    HILOGD("success. %{public}s", swapInfo_->ToString().c_str());
}

void Memcg::UpdateMemInfoFromKernel()
{
    std::string path = KernelInterface::GetInstance().JoinPath(GetMemcgPath_(), "memory.stat");
    std::string content;
    if (!KernelInterface::GetInstance().ReadFromFile(path, content)) {
        return;
    }
    content = std::regex_replace(content, std::regex("\n+"), " "); // replace \n with space
    std::regex re(".*Anon:[[:s:]]*([[:d:]]+) kB[[:s:]]*"
                  ".*Zram:[[:s:]]*([[:d:]]+) kB[[:s:]]*"
                  "Eswap:[[:s:]]*([[:d:]]+) kB[[:s:]]*");
    std::smatch res;
    if (std::regex_match(content, res, re)) {
        memInfo_->anonKiB_ = std::stoi(res.str(1)); // 1: anonKiB index
        memInfo_->zramKiB_ = std::stoi(res.str(2)); // 2: zramKiB index
        memInfo_->eswapKiB_ = std::stoi(res.str(3)); // 3: eswapKiB index
    }
    HILOGD("success. %{public}s", memInfo_->ToString().c_str());
}

void Memcg::SetScore(int score)
{
    score_ = score;
}

void Memcg::SetReclaimRatios(unsigned int mem2zramRatio, unsigned int zram2ufsRatio, unsigned int refaultThreshold)
{
    reclaimRatios_->mem2zramRatio_ = mem2zramRatio;
    reclaimRatios_->zram2ufsRatio_ = zram2ufsRatio;
    reclaimRatios_->refaultThreshold_ = refaultThreshold;
}

bool Memcg::SetReclaimRatios(ReclaimRatios * const ratios)
{
    return reclaimRatios_->SetRatios(ratios);
}

bool Memcg::SetScoreAndReclaimRatiosToKernel()
{
    bool ret = false;
    // write score
    std::string scorePath = KernelInterface::GetInstance().JoinPath(GetMemcgPath_(), "memory.app_score");
    ret = WriteToFile_(scorePath, std::to_string(score_));
    // write reclaim ratios
    std::string ratiosPath = KernelInterface::GetInstance().JoinPath(GetMemcgPath_(),
        "memory.zswapd_single_memcg_param");
    ret = ret && WriteToFile_(ratiosPath, reclaimRatios_->NumsToString());
    return ret;
}

bool Memcg::SwapIn()
{
    std::string zramPath = KernelInterface::GetInstance().JoinPath(GetMemcgPath_(), "memory.ub_ufs2zram_ratio");
    bool ret = WriteToFile_(zramPath, "100"); // 100 means 100% load to zram
    std::string swapinPath = KernelInterface::GetInstance().JoinPath(GetMemcgPath_(), "memory.force_swapin");
    ret = ret && WriteToFile_(swapinPath, "0"); // echo 0 to tigger force swapin
    return ret;
}

inline std::string Memcg::GetMemcgPath_()
{
    // memcg dir = /dev/memcg/
    return KernelInterface::MEMCG_BASE_PATH;
}

inline bool Memcg::WriteToFile_(const std::string& path, const std::string& content, bool truncated)
{
    std::string op = truncated ? ">" : ">>";
    if (!KernelInterface::GetInstance().WriteToFile(path, content, truncated)) {
        HILOGE("failed. %{public}s %{public}s %{public}s", content.c_str(), op.c_str(), path.c_str());
        return false;
    }
    HILOGD("success. %{public}s %{public}s %{public}s", content.c_str(), op.c_str(), path.c_str());
    return true;
}

UserMemcg::UserMemcg(int userId) : userId_(userId)
{
    HILOGI("init UserMemcg success");
}

UserMemcg::~UserMemcg()
{
    HILOGI("release UserMemcg success");
}

bool UserMemcg::CreateMemcgDir()
{
    std::string fullPath = GetMemcgPath_();
    if (!KernelInterface::GetInstance().CreateDir(fullPath)) {
        HILOGE("failed. %{public}s", fullPath.c_str());
        return false;
    }
    HILOGI("success. %{public}s", fullPath.c_str());
    return true;
}

bool UserMemcg::RemoveMemcgDir()
{
    std::string fullPath = GetMemcgPath_();
    if (!KernelInterface::GetInstance().RemoveDirRecursively(fullPath)) {
        HILOGE("failed. %{public}s", fullPath.c_str());
        return false;
    }
    HILOGI("success. %{public}s", fullPath.c_str());
    return true;
}

inline std::string UserMemcg::GetMemcgPath_()
{
    // memcg dir = /dev/memcg/${userId}
    return KernelInterface::GetInstance().JoinPath(KernelInterface::MEMCG_BASE_PATH, std::to_string(userId_));
}

bool UserMemcg::AddProc(const std::string& pid)
{
    std::string fullPath = KernelInterface::GetInstance().JoinPath(GetMemcgPath_(), "cgroup.procs");
    return WriteToFile_(fullPath, pid, false);
}
} // namespace Memory
} // namespace OHOS