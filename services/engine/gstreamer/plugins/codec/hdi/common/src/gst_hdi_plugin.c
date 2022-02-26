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
#include "gst_hdi.h"
#include "gst_hdi_h264_dec.h"
#include "gst_hdi_h265_dec.h"
#include "hi_comm_vb.h"
#include "mpi_vb.h"
#include "mpi_sys.h"
#include "sdk.h"

static constexpr gint DEFAULT_VB_SIZE = 3840 * 2160 * 4;
static constexpr gint DEFAULT_VB_COUNT = 3;
static constexpr gint DEFAULT_VB_POOL_COUNT = 2;
static void get_default_vb_config_param(VB_CONFIG_S *vb_config)
{
    g_return_if_fail(vb_config != NULL);
    vb_config->u32MaxPoolCnt = DEFAULT_VB_POOL_COUNT;
    for (unsigned int i = 0; i < vb_config->u32MaxPoolCnt; i++) {
        vb_config->astCommPool[i].u64BlkSize = DEFAULT_VB_SIZE;
        vb_config->astCommPool[i].u32BlkCnt = DEFAULT_VB_COUNT;
    }
}

static void gst_mpi_init()
{
    VB_CONFIG_S vb_config = {};
    VB_SUPPLEMENT_CONFIG_S st_supplement_config = {};

    get_default_vb_config_param(&vb_config);
    (void)HI_MPI_VB_SetConfig(&vb_config);
    st_supplement_config.u32SupplementConfig = 1;
    (void)HI_MPI_VB_SetSupplementConfig(&st_supplement_config);
    (void)HI_MPI_VB_Init();
    int32_t ret = HI_MPI_SYS_Init();
    if (ret != 0) {
        GST_ERROR_OBJECT(NULL, "HI_MPI_SYS_Init failed, err = %d !\n", ret);
    }
}

void __attribute__((constructor)) gst_hdi_init()
{
    gst_mpi_init();
    int32_t ret = CodecInit();
    if (ret != HDI_SUCCESS) {
        GST_ERROR_OBJECT(NULL, "fail to init hdi, in error %s", gst_hdi_error_to_string(ret));
    }
}

void __attribute__((destructor)) gst_hdi_deinit()
{
    int32_t ret = CodecDeinit();
    if (ret != HDI_SUCCESS) {
        GST_ERROR_OBJECT(NULL, "fail to deinit hdi, in error %s", gst_hdi_error_to_string(ret));
    }
}

static gboolean plugin_init(GstPlugin *plugin)
{
    GHashTable *caps_map = gst_hdi_init_caps_map();
    gboolean ret = FALSE;
    CodecCapbility *caps = g_hash_table_lookup(caps_map, "hdih264dec");
    if (caps != NULL) {
        GST_WARNING_OBJECT(NULL, "caps->whAlignment.widthAlginment = %d %d",
            caps->whAlignment.widthAlginment, caps->whAlignment.heightAlginment);
        if (gst_element_register(plugin, "hdih264dec", GST_RANK_PRIMARY + 1, GST_TYPE_HDI_H264_DEC)) {
            ret = TRUE;
        } else {
            GST_WARNING_OBJECT(NULL, "register hdih264dec failed");
        }
    } else {
        GST_WARNING_OBJECT(NULL, "caps map find hdih264dec failed");
    }
    caps = g_hash_table_lookup(caps_map, "hdih265dec");
    if (caps != NULL) {
        GST_WARNING_OBJECT(NULL, "caps->whAlignment.widthAlginment = %d %d",
            caps->whAlignment.widthAlginment, caps->whAlignment.heightAlginment);
        if (gst_element_register(plugin, "hdih265dec", GST_RANK_PRIMARY + 1, GST_TYPE_HDI_H265_DEC)) {
            ret = TRUE;
        } else {
            GST_WARNING_OBJECT(NULL, "register hdih265dec failed");
        }
    } else {
        GST_WARNING_OBJECT(NULL, "caps map find hdih265dec failed");
    }
    return ret;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    _hdi_codec,
    "GStreamer HDI Plug-ins",
    plugin_init,
    PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)