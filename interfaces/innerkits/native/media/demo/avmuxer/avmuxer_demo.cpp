/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "avmuxer_demo.h"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include "securec.h"
#include "media_errors.h"

namespace {
    const int32_t H264_FRAME_SIZE[] = {
        646, 5855, 3185, 3797, 3055, 5204, 2620, 6262, 2272, 3702, 4108, 4356, 4975, 4590, 3083, 1930, 1801, 1945,
        3475, 4028, 1415, 1930, 2802, 2176, 1727, 2287, 2274, 2033, 2432, 4447, 4130, 5229, 5792, 4217, 5804,
        6586, 7506, 5128, 5549, 6685, 5248, 4819, 5385, 4818, 5239, 4148, 6980, 5124, 4255, 5666, 4756, 4975,
        3840, 4913, 3649, 4002, 4926, 4284, 5329, 4305, 3750, 4770, 4090, 4767, 3995, 5039, 3820, 4566, 5556,
        4029, 3755, 5059, 3888, 3572, 4680, 4662, 4259, 3869, 4306, 3519, 3160, 4400, 4426, 4370, 3489, 4907,
        4102, 3723, 4420, 4347, 4117, 4578, 4470, 4579, 4128, 4157, 4226, 4742, 3616, 4476, 4084, 4623, 3736,
        4207, 3644, 4349, 4948, 4009, 3583, 4658, 3974, 5441, 4049, 3786, 4093, 3375, 4207, 3787, 4365, 2905,
        4371, 4132, 3633, 3652, 2977, 4387, 3368, 3887, 3464, 4198, 4690, 4467, 2931, 3573, 4652, 3901, 4403,
        3120, 3494, 4666, 3898, 3607, 3272, 4070, 3151, 3237, 3936, 3962, 3637, 3716, 3735, 4371, 3141, 3322,
        4401, 3579, 4006, 2720, 3526, 4796, 3737, 3824, 3257, 4310, 2992, 3537, 3209, 3453, 3819, 3212, 4384,
        3571, 3682, 3344, 3017, 3960, 2737, 1970, 2433, 1442, 1560, 4710, 1070, 877, 833, 838, 776, 735, 1184,
        1172, 699, 723, 2828, 4257, 4329, 3567, 5365, 4213, 3612, 4833, 3388, 3553, 3535, 4937, 4057, 3990,
        5047, 4197, 4656, 3219, 3661, 3666, 3908, 4385, 4350, 3636, 4038, 5213, 3677, 3789, 4221, 4137, 4440,
        3447, 3836, 3912, 4806, 3100, 2963, 5204, 2394, 2391, 1772, 1586, 1598, 2558, 2663, 4537, 3530, 4045,
        4641, 5723, 3688, 4231, 3420, 3462, 3828, 4764, 3944, 4499, 4375, 4597, 4305, 3872, 3969, 2805, 4398,
        3480, 4105, 3890, 3761, 3652, 4356, 2771, 3972, 2930, 3456, 3236, 4648, 3627, 2689, 3827, 2254, 3492,
        2988, 4408, 3007, 4611, 3018, 4783, 2556, 3263, 4536, 4159, 3818, 5093, 3539, 4336, 3400, 3871, 4019,
        4619, 5520, 3781, 4026, 4864, 3340, 4153, 4641, 4292, 4071, 4144, 5109, 3695, 4512, 3882, 3943, 4152,
        4133, 3862, 4717, 3431, 4984, 4164, 4359, 3401, 3727, 4256, 3563, 4694, 3225, 3984, 2432, 3790, 2827,
        3595, 4124, 3854, 2890, 3477, 3989, 3251, 3714, 3345, 4742, 1967, 3931, 1985, 1737, 1854, 2192, 2370,
        2083, 3265, 3312, 3071, 4255, 3994, 4563, 4650, 4885, 3868, 4698, 3103, 3682, 4197, 5532, 3963, 4756,
        4067, 3917, 3667, 3812, 4793, 3260, 3763, 4670, 3184, 2930, 3558, 3245, 4120, 4700, 3671, 4442, 3406,
        4862, 4331, 5064, 4058, 4075, 3160, 3930, 5187, 3816, 3795, 3085, 3564, 3856, 3948, 4474, 3511, 4108,
        4789, 2944, 3323, 2162, 2657, 2219, 1653, 2824, 2716, 3523, 2760, 3328, 3042, 3828, 3759, 3950, 3830,
        3336, 4457, 3193, 3706, 4314, 3937, 3422, 4067, 5328, 3693, 4567, 3444, 4317, 4929, 3838, 4129, 2975,
        4227, 4639, 4348, 2935, 3999, 4745, 3919, 3694, 2602, 4538, 4637, 4250, 3716, 3513, 3856, 4916, 4460,
        4263, 4153, 4299, 3577, 5527, 2486, 3332, 4133, 4145, 3369, 3576, 3940, 4304, 3179, 5266, 3536, 3622,
        2684, 3449, 3621, 4363, 4216, 4913, 5026, 3336, 3057, 2782, 3716, 3036, 4438, 3904, 4823, 1761, 2045,
        1446, 3210, 1625, 2400, 3489, 4719, 3954, 3756, 4940, 2371, 4516, 3739, 3572, 2644, 3837, 4915, 2251,
        4248, 4019, 4407, 4217, 2913, 5106
    };
    const int32_t AAC_FRAME_SIZE[] = {
        361, 368, 22, 20, 20, 20, 20, 20, 18, 198, 513, 499, 534, 522, 541, 608, 596, 613, 631, 543, 
        563, 505, 411, 375, 402, 361, 396, 405, 372, 402, 382, 371, 363, 366, 401, 390, 519, 325, 367, 
        365, 389, 358, 389, 413, 327, 493, 378, 360, 359, 387, 356, 391, 362, 360, 393, 408, 384, 348, 
        361, 437, 494, 381, 397, 329, 365, 376, 354, 376, 347, 541, 366, 377, 370, 365, 368, 366, 358, 
        366, 401, 395, 393, 385, 348, 365, 386, 375, 381, 371, 549, 335, 376, 362, 390, 391, 350, 380, 
        368, 360, 387, 408, 392, 383, 390, 418, 353, 383, 375, 410, 355, 375, 437, 413, 393, 427, 397, 
        397, 363, 418, 393, 412, 373, 433, 323, 381, 396, 391, 372, 400, 397, 294, 280, 391, 405, 378, 
        410, 505, 411, 385, 380, 377, 345, 393, 375, 377, 378, 351, 401, 377, 404, 368, 370, 402, 386, 
        398, 393, 392, 386, 403, 360, 393, 373, 386, 362, 344, 416, 355, 380, 353, 398, 422, 386, 365, 
        335, 340, 383, 371, 386, 375, 399, 398, 352, 387, 380, 390, 391, 390, 385, 404, 384, 407, 365, 
        348, 388, 388, 385, 392, 383, 462, 463, 466, 392, 351, 358, 385, 358, 389, 378, 359, 349, 424, 
        335, 392, 347, 348, 379, 302, 295, 357, 458, 466, 460, 370, 390, 402, 405, 411, 378, 383, 351, 
        413, 420, 362, 343, 369, 367, 378, 355, 385, 410, 420, 375, 398, 394, 384, 376, 400, 395, 389, 
        395, 363, 422, 364, 365, 380, 395, 364, 395, 350, 319, 308, 374, 560, 503, 500, 438, 427, 445, 
        416, 366, 424, 331, 354, 376, 381, 387, 369, 382, 343, 366, 442, 419, 348, 362, 354, 405, 419, 
        332, 376, 388, 405, 365, 428, 379, 384, 387, 403, 385, 344, 366, 381, 366, 371, 286, 328, 470, 
        413, 404, 409, 406, 376, 370, 380, 393, 345, 400, 386, 397, 376, 407, 364, 362, 351, 377, 345, 
        375, 413, 353, 419, 382, 379, 383, 444, 392, 368, 374, 376, 349, 413, 405, 374, 355, 412, 385, 
        356, 277, 361, 461, 398, 431, 381, 405, 389, 374, 392, 377, 407, 377, 389, 406, 391, 378, 420, 
        388, 372, 372, 350, 373, 446, 399, 354, 368, 350, 373, 418, 390, 366, 367, 351, 414, 413, 362, 
        373, 364, 312, 400, 391, 371, 384, 478, 391, 400, 344, 360, 383, 349, 370, 393, 369, 364, 366, 
        401, 377, 360, 392, 398, 388, 358, 374, 386, 395, 374, 419, 376, 393, 376, 348, 416, 381, 363, 
        376, 381, 369, 378, 416, 366, 379, 363, 430, 368, 358, 470, 296, 358
    };
    const int32_t MPEG4_FRAME_SIZE[] = {
        40629, 42109, 7513, 2414, 867, 1342, 676, 599, 698, 600, 657, 610, 15298, 615, 403, 433, 480, 458, 485,
        536, 572, 524, 492, 608, 9497, 586, 423, 504, 526, 504, 538, 503, 494, 531, 534, 652, 9152, 682, 527,
        559, 552, 571, 624, 776, 713, 687, 687, 685, 9301, 721, 599, 702, 786, 728, 715, 739, 711, 872, 748,
        886, 9289, 777, 703, 716, 752, 807, 843, 827, 945, 880, 891, 803, 9334, 817, 758, 933, 917, 915, 935,
        969, 959, 974, 1054, 1147, 9385, 912, 849, 989, 957, 1075, 997, 1060, 1108, 1077, 1011, 1107, 9467, 988,
        1003, 1013, 1059, 1122, 1042, 1072, 1087, 1149, 1057, 1075, 9281, 948, 920, 1008, 1075, 1210, 1083, 1069,
        1183, 1123, 1133, 1111, 9196, 1044, 982, 1153, 1235, 1160, 1179, 1124, 1272, 1175, 1180, 1285, 9072, 1113,
        1000, 1045, 1147, 1033, 1057, 1240, 1241, 1127, 1065, 1118, 8856, 938, 882, 977, 1079, 1081, 1064, 964,
        1059, 932, 1037, 1177, 8742, 970, 1007, 905, 976, 916, 984, 1039, 1076, 1055, 955, 1002, 8544, 861, 765,
        854, 854, 902, 843, 757, 900, 720, 882, 914, 8272, 764, 627, 637, 736, 635, 751, 742, 841, 744, 822, 694,
        8307, 567, 495, 612, 737, 701, 646, 585, 671, 626, 638, 816, 8173, 669, 524, 531, 616, 606, 637, 569, 645,
        598, 650, 592, 8150, 606, 592, 503, 570, 577, 632, 585, 618, 678, 640, 617, 8062, 587, 527, 521, 578, 640,
        648, 587, 610, 665, 658, 613, 7945, 623, 486, 471, 590, 566, 599, 542, 654, 546, 845, 587, 7956, 553, 470,
        564, 553, 590, 558, 585, 619, 658, 628, 692, 7919, 633, 484, 556, 575, 637, 606, 615, 665, 643, 589, 654,
        7964, 634, 543, 576, 583, 655, 563, 676, 594, 690, 522, 646, 7896, 595, 491, 525, 614, 678, 558, 641, 652,
        673, 545, 683, 7834, 598, 457, 545, 574, 627, 553, 626, 664, 693, 604, 720, 7796, 591, 470, 566, 551, 592,
        491, 653, 624, 601, 526, 700, 7724, 596, 438, 524, 512, 572, 537, 665, 669, 641, 561, 706, 7918, 661, 552,
        579, 626, 628, 542, 664, 632, 711, 573, 767, 8016, 645, 472, 591, 573, 760, 607, 676, 628, 685, 596, 714,
        8051, 672, 496, 561, 622, 599, 594, 696, 736, 737, 622, 705, 8017, 645, 536, 644, 642, 669, 625, 708, 699,
        694, 758, 769, 8265, 715, 556, 629, 684, 684, 688, 674, 672, 667, 710, 643, 8383, 668, 598, 600, 673, 671,
        690, 513, 730, 777, 660, 724, 8375, 692, 557, 631, 626, 733, 677, 771, 749, 843, 727, 794, 8508, 788, 576,
        670, 706, 841, 714, 778, 744, 868, 765, 762, 8689, 752, 588, 749, 721, 825, 729, 794, 773, 875, 773, 898,
        8797, 822, 654, 775, 780, 869, 777, 849, 854, 938, 772, 844, 8943, 835, 731, 729, 758, 837, 735, 782, 821,
        896, 789, 848, 9095, 848, 652, 751, 818, 988, 828, 914, 854, 996, 888, 915, 9435, 909, 689, 767, 849, 966,
        879, 914, 939, 1026, 888, 943, 9538, 893, 706, 868, 873, 951, 850, 808, 1072, 918, 932, 1166, 10145, 882,
        815, 866, 886, 949, 990, 972, 943, 936, 1050, 955, 10352, 898, 916, 897, 904, 871, 962, 951, 945, 925, 1026,
        931, 10486, 856, 806, 861, 918, 907, 1076, 970, 979, 945, 1007, 1041, 10717, 940, 902, 927, 968, 1041, 1176,
        1139, 1101, 1121, 1187, 1068, 10897, 968, 973, 974, 990, 1040, 1162, 1053, 1075, 1214, 1262, 1141, 11030, 993,
        917, 970, 1008, 1054, 1166, 1090, 1106, 1012, 1165, 1124, 11088, 1057, 1038, 990, 1045, 982, 1131, 1078, 1078,
        1047, 1205, 1092, 10892, 928, 881, 962, 1001, 1004, 1058, 991, 1047, 1014, 1059, 1045, 10754, 984
    };
    const int32_t MP3_FRAME_SIZE[] = {
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192,
        192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192
    };
    std::map<std::string, std::tuple<uint32_t, uint32_t, const int32_t*, std::string>> CODEC_PARAMETER = {
        {"h264", {33333, 501, H264_FRAME_SIZE, "/data/media/test.h264"}},
        {"mpeg4", {16666, 602, MPEG4_FRAME_SIZE, "/data/media/test.mpeg4"}},
        {"aac", {23220, 433, AAC_FRAME_SIZE, "/data/media/test.aac"}},
        {"mp3", {23220, 575, MP3_FRAME_SIZE, "/data/media/test.mp3"}}
    };

    constexpr uint32_t SLEEP_UTIME = 50000;
    constexpr uint32_t H264_WIDTH = 480;
    constexpr uint32_t H264_HEIGHT = 640;
    constexpr uint32_t H264_FRAME_RATE = 30;
    constexpr uint32_t MPEG4_WIDTH = 720;
    constexpr uint32_t MPEG4_HEIGHT = 480;
    constexpr uint32_t MPEG4_FRAME_RATE = 60;
    constexpr uint32_t AAC_SAMPLE_RATE = 44100;
    constexpr uint32_t AAC_CHANNEL = 2;
    constexpr uint32_t MP3_SAMPLE_RATE = 48000;
    constexpr uint32_t MP3_CHANNEL = 2;
}
namespace OHOS {
namespace Media {
bool AVMuxerDemo::PushBuffer(std::shared_ptr<std::ifstream> File, const int32_t *FrameArray,
    int32_t i, int32_t TrakcId, int64_t stamp)
{
    if (FrameArray == nullptr) {
        std::cout << "Frame array error" << std::endl;
        return false;
    }
    uint8_t *buffer = (uint8_t *)malloc(sizeof(char) * (*FrameArray));
    if (buffer == nullptr) {
        std::cout << "no memory" << std::endl;
        return false;
    }
    (void)File->read((char *)buffer, *FrameArray);
    std::shared_ptr<AVMemory> aVMem = std::make_shared<AVMemory>(buffer, *FrameArray);
    aVMem->SetRange(0, *FrameArray);
    TrackSampleInfo info;
    info.size = *FrameArray;
    info.trackIdx = TrakcId;

    if (i == 0) {
        info.timeMs = 0;
        info.flags = AVCODEC_BUFFER_FLAG_CODEC_DATA;
    } else if ((i == 1 && TrakcId == videoTrakcId_) || TrakcId == audioTrackId_) {
        info.timeMs = stamp;
        info.flags = AVCODEC_BUFFER_FLAG_SYNC_FRAME;
    } else {
        info.timeMs = stamp;
        info.flags = AVCODEC_BUFFER_FLAG_PARTIAL_FRAME;
    }

    if (avmuxer_->WriteTrackSample(aVMem, info) != MSERR_OK) {
        free(buffer);
        return false;
    };
    free(buffer);
    usleep(SLEEP_UTIME);

    return true;
}

std::shared_ptr<std::ifstream> openFile(const std::string filePath) {
    auto file = std::make_unique<std::ifstream>();
    file->open(filePath, std::ios::in | std::ios::binary);
    return file;
}

void AVMuxerDemo::WriteTrackSample()
{
    double videoStamp = 0;
    double audioStamp = 0;
    int32_t i = 0;
    int32_t videoLen = videoFile_ == nullptr ? INT32_MAX : videoFrameNum_;
    int32_t audioLen = audioFile_ == nullptr ? INT32_MAX : audioFrameNum_;
    while (i < videoLen && i < audioLen) {
        if (videoFile_ != nullptr) {
            if (!PushBuffer(videoFile_, videoFrameArray_, i, videoTrakcId_, videoStamp)) {
                break;
            }
            videoFrameArray_++;
            videoStamp += videoTimeDuration_;
        }
        if (audioFile_ != nullptr) {
            if (!PushBuffer(audioFile_, audioFrameArray_, i, audioTrackId_, audioStamp)) {
                break;
            }
            audioFrameArray_++;
            audioStamp += audioTimeDuration_;
        }
        i++;
        std::cout << videoStamp << std::endl;
        std::cout << audioStamp << std::endl;
    }
}

void AVMuxerDemo::SetParameter(std::string type) {
    if (type == "h264" || type == "mpeg4") {
        videoTimeDuration_ = std::get<0>(CODEC_PARAMETER[type]);
        videoFrameNum_ = std::get<1>(CODEC_PARAMETER[type]);
        videoFrameArray_ = std::get<2>(CODEC_PARAMETER[type]);
        videoFile_ = openFile(std::get<3>(CODEC_PARAMETER[type]));
    } else {
        audioTimeDuration_ = std::get<0>(CODEC_PARAMETER[type]);
        audioFrameNum_ = std::get<1>(CODEC_PARAMETER[type]);
        audioFrameArray_ = std::get<2>(CODEC_PARAMETER[type]);
        audioFile_ = openFile(std::get<3>(CODEC_PARAMETER[type]));
    }
    
}

bool AVMuxerDemo::AddTrackVideo(std::string& videoType)
{
    MediaDescription trackDesc;
    if (videoType == "h264") {
        trackDesc.PutStringValue(std::string(MediaDescriptionKey::MD_KEY_CODEC_MIME), "video/avc");
        trackDesc.PutIntValue(std::string(MediaDescriptionKey::MD_KEY_WIDTH), H264_WIDTH);
        trackDesc.PutIntValue(std::string(MediaDescriptionKey::MD_KEY_HEIGHT), H264_HEIGHT);
        trackDesc.PutIntValue(std::string(MediaDescriptionKey::MD_KEY_FRAME_RATE), H264_FRAME_RATE);
        SetParameter("h264");
    } else if (videoType == "mpeg4") {
        trackDesc.PutStringValue(std::string(MediaDescriptionKey::MD_KEY_CODEC_MIME), "video/mp4v-es");
        trackDesc.PutIntValue(std::string(MediaDescriptionKey::MD_KEY_WIDTH), MPEG4_WIDTH);
        trackDesc.PutIntValue(std::string(MediaDescriptionKey::MD_KEY_HEIGHT), MPEG4_HEIGHT);
        trackDesc.PutIntValue(std::string(MediaDescriptionKey::MD_KEY_FRAME_RATE), MPEG4_FRAME_RATE);
        SetParameter("mpeg4");
    } else {
        std::cout << "Failed to check video type" << std::endl;
        return false;
    }
    avmuxer_->AddTrack(trackDesc, videoTrakcId_);
    std::cout << "trackId is: " << videoTrakcId_ << std::endl;

    return true;
}

bool AVMuxerDemo::AddTrackAudio(std::string& audioType)
{
    MediaDescription trackDesc;
    if (audioType == "aac") {
        trackDesc.PutStringValue(std::string(MediaDescriptionKey::MD_KEY_CODEC_MIME), "audio/mp4a-latm");
        trackDesc.PutIntValue(std::string(MediaDescriptionKey::MD_KEY_SAMPLE_RATE), AAC_SAMPLE_RATE);
        trackDesc.PutIntValue(std::string(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT), AAC_CHANNEL);
        SetParameter("aac");
    } else if (audioType == "mp3") {
        trackDesc.PutStringValue(std::string(MediaDescriptionKey::MD_KEY_CODEC_MIME), "audio/mpeg");
        trackDesc.PutIntValue(std::string(MediaDescriptionKey::MD_KEY_SAMPLE_RATE), MP3_SAMPLE_RATE);
        trackDesc.PutIntValue(std::string(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT), MP3_CHANNEL);
        SetParameter("mp3");
    } else {
        std::cout << "Failed to check audio type" << std::endl;
        return false;
    }
    avmuxer_->AddTrack(trackDesc, audioTrackId_);
    std::cout << "trackId is: " << audioTrackId_ << std::endl;

    return true;
}

void AVMuxerDemo::DoNext()
{
    std::string path;
    std::string format;
    std::cout << "Please enter mode, 0: video + audio, 1: video, 2: audio" << std::endl;
    int32_t mode;
    std::cin >> mode;
    switch (mode) {
        case 0:
            std::cout << "Please enter video type, note: only support h264 and mpeg4" << std::endl;
            std::cin >> videoType_;
            std::cout << "Please enter audio type, note: only support aac and mp3" << std::endl;
            std::cin >> audioType_;
            format = "mp4";
            break;
        case 1:
            std::cout << "Please enter video type, note: only support h264 and mpeg4" << std::endl;
            std::cin >> videoType_;
            format = "mp4";
            break;
        case 2:
            std::cout << "Please enter audio type, note: only support aac" << std::endl;
            std::cin >> audioType_;
            format = "m4a";
            break;
        default:
            std::cout << "Failed to check mode" << std::endl;
    }
    path = videoType_ + audioType_ + "." + format;
    int32_t fd = open(path.c_str(), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        std::cout << "Open file failed! filePath is: " << path << std::endl;
        return;
    }
    avmuxer_->SetOutput(fd, format);
    avmuxer_->SetLocation(30.1111, 150.22222);
    avmuxer_->SetOrientationHint(90);

    if ((mode == 0 && (AddTrackVideo(videoType_) == false || AddTrackAudio(audioType_) == false)) || 
        (mode == 1 && (AddTrackVideo(videoType_) == false)) ||
        (mode == 2 && (AddTrackAudio(audioType_) == false))) {
        return;
    }

    avmuxer_->Start();
    WriteTrackSample();

    avmuxer_->Stop();
    avmuxer_->Release();

    (void)::close(fd);
}

void AVMuxerDemo::RunCase()
{
    avmuxer_ = OHOS::Media::AVMuxerFactory::CreateAVMuxer();
    if (avmuxer_ == nullptr) {
        std::cout << "avmuxer_ is null" << std::endl;
        return;
    }
    DoNext();
}
}  // namespace Media
}  // namespace OHOS