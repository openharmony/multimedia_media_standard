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

#include "player_service_unit_test.h"
#include "media_errors.h"
#include "i_media_service.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media{
namespace {
    const string MEDIA_ROOT = "file://data/media/";
    const string VIDEO_FILE1 = MEDIA_ROOT + "test_1920_1080_1.mp4";
}

HWTEST(PlayerServiceUnitTest, PlayerService_Create_001, TestSize.Level0) 
{   
    auto server = MediaServiceFactory::GetInstance().CreatePlayerService();
    ASSERT_NE(nullptr, server);
}

HWTEST(PlayerServiceUnitTest, PlayerService_SetSource_001, TestSize.Level0) 
{
    auto server = MediaServiceFactory::GetInstance().CreatePlayerService();
    ASSERT_NE(nullptr, server);
    int ret = server->SetSource(VIDEO_FILE1);
    EXPECT_EQ(MSERR_OK, ret);
}

}
}
