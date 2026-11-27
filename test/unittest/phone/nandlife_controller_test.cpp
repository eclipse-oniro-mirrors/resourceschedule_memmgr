/**
 * Copyright (c) 2022-2026 Huawei Device Co., Ltd.
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

#include "gtest/gtest.h"
#include <string>

#include "kernel_interface.h"
#include "utils.h"

#define private public
#define protected public
#include "nandlife_controller.h"
#undef private
#undef protected

namespace OHOS {
namespace Memory {
using namespace testing;
using namespace testing::ext;

static bool IsEswapInfoAvailable()
{
    std::string content;
    return KernelInterface::GetInstance().ReadFromFile("/dev/memcg/memory.eswap_info", content) &&
        content.find("Total Swapout Size") != std::string::npos;
}

static void SkipIfEswapInfoUnavailable()
{
    if (!IsEswapInfoAvailable()) {
        GTEST_SKIP() << "eswap info is unavailable";
    }
}

class NandLifeControllerTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NandLifeControllerTest::SetUpTestCase()
{
}

void NandLifeControllerTest::TearDownTestCase()
{
}

void NandLifeControllerTest::SetUp()
{
}

void NandLifeControllerTest::TearDown()
{
}

HWTEST_F(NandLifeControllerTest, GetSwapOutKBSinceKernelBootTest, TestSize.Level1)
{
    SkipIfEswapInfoUnavailable();
    unsigned long long swapOutKBSinceKernelBoot = 0;
    EXPECT_EQ(NandLifeController::GetInstance().GetSwapOutKBSinceKernelBoot(swapOutKBSinceKernelBoot), true);
}
} // namespace Memory
} // namespace OHOS
