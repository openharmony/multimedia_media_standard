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
#include "sdk.h"

GST_DEFINE_MINI_OBJECT_TYPE (GstHDICodec, gst_hdi_codec);
static void gst_hdi_codec_free(GstHDICodec * codec);
static const int DEFAULT_VB_SIZE = 3840 * 2160 * 4;
static const int DEFAULT_VB_COUNT = 3;
static const int DEFAULT_VB_POOL_COUNT = 2;

static void
GetDefaultVbConfigParam(VB_CONFIG_S *pstVbConfig)
{
  g_return_if_fail (pstVbConfig != nullptr);
  pstVbConfig->u32MaxPoolCnt = DEFAULT_VB_POOL_COUNT;
  for (unsigned int i = 0; i < pstVbConfig->u32MaxPoolCnt; i++) {
      pstVbConfig->astCommPool[i].u64BlkSize = DEFAULT_VB_SIZE;
      pstVbConfig->astCommPool[i].u32BlkCnt = DEFAULT_VB_COUNT;
  }
}

static void
gst_mpi_init ()
{
  VB_CONFIG_S vbConfig = {};
  VB_SUPPLEMENT_CONFIG_S stSupplementConf = {};

  GetDefaultVbConfigParam(&vbConfig);
  (void)HI_MPI_VB_SetConfig(&vbConfig);
  stSupplementConf.u32SupplementConfig = 1;
  (void)HI_MPI_VB_SetSupplementConfig(&stSupplementConf);
  (void)HI_MPI_VB_Init();
  int32_t ret = HI_MPI_SYS_Init();
  if (ret != 0) {
    GST_ERROR_OBJECT(nullptr, "HI_MPI_SYS_Init failed, err = %d !\n", ret);
  }
  return;
}

void __attribute__((constructor))
gst_hdi_init ()
{
  gst_mpi_init();
  int32_t ret = CodecInit ();
  if (ret != HDI_SUCCESS) {
    GST_ERROR_OBJECT (nullptr, "fail to init hdi, in error %s", gst_hdi_error_to_string(ret));
  }
  return;
}

void __attribute__((destructor))
gst_hdi_deinit ()
{
  int32_t ret = CodecDeinit ();
  if (ret != HDI_SUCCESS) {
    GST_ERROR_OBJECT (nullptr, "fail to deinit hdi, in error %s", gst_hdi_error_to_string(ret));
  }

  return;
}

int32_t
gst_hdi_get_caps (AvCodecMime mime, CodecType type, uint32_t flags, CodecCapbility *cap)
{
  g_return_val_if_fail (cap != nullptr, HDI_FAILURE);
  GST_DEBUG_OBJECT (nullptr, "get hdi caps");
  int32_t ret = CodecGetCapbility (mime, type, flags, cap);
  if (ret != HDI_SUCCESS) {
    GST_ERROR_OBJECT (nullptr, "fail to get hdi caps, in error %s", gst_hdi_error_to_string(ret));
  }
  return ret;
}

GstHDICodec *
gst_hdi_codec_new (const gchar * codec_name, Param * params, int param_cnt)
{
  g_return_val_if_fail (codec_name != nullptr, nullptr);
  GST_DEBUG_OBJECT (nullptr, "create hdi codec");
  GstHDICodec * codec = g_slice_new0 (GstHDICodec);
  gst_mini_object_init (GST_MINI_OBJECT_CAST (codec), 0,
      gst_hdi_codec_get_type (), nullptr, nullptr,
      (GstMiniObjectFreeFunction) gst_hdi_codec_free);

  int32_t ret = CodecCreate (codec_name, params, param_cnt, &codec->handle);
  if (ret != HDI_SUCCESS) {
    GST_ERROR_OBJECT (nullptr, "fail to create codec, in error %s", gst_hdi_error_to_string(ret));
    g_slice_free (GstHDICodec, codec);
    return nullptr;
  }
  g_mutex_init (&codec->start_lock);
  g_mutex_init (&codec->lock);
  return codec;
}

int32_t
gst_hdi_codec_set_meta (const GstHDICodec * codec, Param * params, int param_cnt)
{
  GST_DEBUG_OBJECT (nullptr, "set hdi params");
  g_return_val_if_fail (codec != nullptr, HDI_FAILURE);
  g_return_val_if_fail (codec->handle != nullptr, HDI_FAILURE);
  int32_t ret = CodecSetParameter (codec->handle, params, param_cnt);
  if (ret != HDI_SUCCESS) {
    GST_ERROR_OBJECT (nullptr, "fail to set hdi params, in error %s", gst_hdi_error_to_string(ret));
  }
  return ret;
}

int32_t
gst_hdi_codec_get_meta (const GstHDICodec * codec, Param * params, int param_cnt)
{
  GST_DEBUG_OBJECT (nullptr, "get hdi params");
  g_return_val_if_fail (codec != nullptr, HDI_FAILURE);
  g_return_val_if_fail (codec->handle != nullptr, HDI_FAILURE);
  int32_t ret = CodecGetParameter (codec->handle, params, param_cnt);
  if (ret != HDI_SUCCESS) {
    GST_ERROR_OBJECT (nullptr, "fail to get hdi params, in error %s", gst_hdi_error_to_string(ret));
  }
  return ret;
}

int32_t
gst_hdi_codec_start (GstHDICodec * codec)
{
  GST_DEBUG_OBJECT (codec, "start hdi codec");
  g_return_val_if_fail (codec != nullptr, HDI_FAILURE);
  g_return_val_if_fail (codec->handle != nullptr, HDI_FAILURE);
  g_mutex_lock (&codec->start_lock);
  if (codec->hdi_started == TRUE) {
    g_mutex_unlock (&codec->start_lock);
    return HDI_SUCCESS;
  }
  codec->hdi_started = TRUE;
  int32_t ret = CodecStart (codec->handle);
  if (ret != HDI_SUCCESS) {
    codec->hdi_started = FALSE;
    GST_ERROR_OBJECT (nullptr, "fail to start hdi, in error %s", gst_hdi_error_to_string(ret));
  }
  g_mutex_unlock (&codec->start_lock);
  return ret;
}

int32_t
gst_hdi_codec_stop (GstHDICodec * codec)
{
  GST_DEBUG_OBJECT (codec, "stop hdi codec");
  g_return_val_if_fail (codec != nullptr, HDI_FAILURE);
  g_return_val_if_fail (codec->handle != nullptr, HDI_FAILURE);
  g_mutex_lock (&codec->start_lock);
  if (codec->hdi_started == FALSE) {
    g_mutex_unlock (&codec->start_lock);
    return HDI_SUCCESS;
  }
  codec->hdi_started = FALSE;
  int32_t ret = CodecStop (codec->handle);
  if (ret != HDI_SUCCESS) {
    codec->hdi_started = TRUE;
    GST_ERROR_OBJECT (nullptr, "fail to stop hdi, in error %s", gst_hdi_error_to_string(ret));
  }
  g_mutex_unlock (&codec->start_lock);
  return HDI_SUCCESS;
}

int32_t
gst_hdi_port_flush (GstHDICodec * codec, DirectionType directType)
{
  GST_DEBUG_OBJECT (codec, "flush hdi port");
  g_return_val_if_fail (codec != nullptr, HDI_FAILURE);
  g_return_val_if_fail (codec->handle != nullptr, HDI_FAILURE);
  g_mutex_lock (&codec->start_lock);
  if (!codec->hdi_started) {
    g_mutex_unlock (&codec->start_lock);
    return HDI_SUCCESS;
  }
  g_mutex_unlock (&codec->start_lock);
  int32_t ret = CodecFlush (codec->handle, ALL_TYPE);
  if (ret != HDI_SUCCESS) {
    GST_ERROR_OBJECT (nullptr, "fail to flush port, in error %s", gst_hdi_error_to_string(ret));
  }
  return ret;
}

int32_t
queue_input_buffer (const GstHDICodec * codec, InputInfo * inputData, uint32_t timeoutMs)
{
  GST_DEBUG_OBJECT (codec, "queue hdi inbuf");
  g_return_val_if_fail (codec != nullptr, HDI_FAILURE);
  g_return_val_if_fail (codec->handle != nullptr, HDI_FAILURE);
  int32_t ret = CodecQueueInput (codec->handle, inputData, timeoutMs);
  if (ret != HDI_SUCCESS) {
    GST_ERROR_OBJECT (nullptr, "fail to queue input buffer, in error %s", gst_hdi_error_to_string(ret));
  }
  return ret;
}

int32_t
deque_input_buffer (const GstHDICodec * codec, InputInfo * inputData, uint32_t timeoutMs)
{
  GST_DEBUG_OBJECT (codec, "deque hdi inbuf");
  g_return_val_if_fail (codec != nullptr, HDI_FAILURE);
  g_return_val_if_fail (codec->handle != nullptr, HDI_FAILURE);
  int32_t ret = CodecDequeInput (codec->handle, timeoutMs, inputData);
  if (ret != HDI_SUCCESS) {
    GST_ERROR_OBJECT (nullptr, "fail to deque input buffer, in error %s", gst_hdi_error_to_string(ret));
  }
  return ret;
}

int32_t
queue_output_buffer (const GstHDICodec * codec, OutputInfo * outInfo, uint32_t timeoutMs)
{
  GST_DEBUG_OBJECT (codec, "queue hdi outbuf");
  g_return_val_if_fail (codec != nullptr, HDI_FAILURE);
  g_return_val_if_fail (codec->handle != nullptr, HDI_FAILURE);
  int32_t ret = CodecQueueOutput (codec->handle, outInfo, timeoutMs, -1);
  if (ret != HDI_SUCCESS) {
    GST_ERROR_OBJECT (nullptr, "fail to queue output buffer, in error %s", gst_hdi_error_to_string(ret));
  }
  return ret;
}

int32_t
deque_output_buffer (const GstHDICodec * codec, OutputInfo * outInfo, uint32_t timeoutMs)
{
  GST_DEBUG_OBJECT (codec, "deque hdi outbuf");
  g_return_val_if_fail (codec != nullptr, HDI_FAILURE);
  g_return_val_if_fail (codec->handle != nullptr, HDI_FAILURE);
  int32_t ret = CodecDequeueOutput (codec->handle, timeoutMs, nullptr, outInfo);
  if (ret != HDI_SUCCESS) {
    GST_ERROR_OBJECT (nullptr, "fail to deque output buffer, in error %s", gst_hdi_error_to_string(ret));
  }
  return ret;
}

static void
gst_hdi_codec_free (GstHDICodec * codec)
{
  GST_DEBUG_OBJECT (codec, "destroy hdi");
  g_return_if_fail (codec != nullptr);
  g_return_if_fail (codec->handle != nullptr);
  int32_t ret = CodecDestroy (codec->handle);
  if (ret != HDI_SUCCESS) {
    GST_ERROR_OBJECT (nullptr, "fail to destroy hdi, in error %s", gst_hdi_error_to_string(ret));
  }
  g_return_if_fail (codec->parent != nullptr);
  gst_object_unref (codec->parent);
  g_mutex_clear (&codec->start_lock);
  g_mutex_clear (&codec->lock);
  g_slice_free (GstHDICodec, codec);
}

gboolean
gst_hdi_buffer_map_buffer (CodecBufferInfo * buffer, GstBuffer * input)
{
  g_return_val_if_fail (buffer != nullptr, FALSE);
  g_return_val_if_fail (input != nullptr, FALSE);
  GstMapInfo info = GST_MAP_INFO_INIT;

  if (!gst_buffer_map (input, &info, GST_MAP_READ))
    return FALSE;
  buffer->addr = info.data;
  buffer->length = info.size;

  return TRUE;
}

GstHDICodec *
gst_hdi_codec_ref (GstHDICodec * codec)
{
  g_return_val_if_fail (codec != nullptr, nullptr);

  gst_mini_object_ref (GST_MINI_OBJECT_CAST (codec));
  return codec;
}

void
gst_hdi_codec_unref (GstHDICodec * codec)
{
  g_return_if_fail (codec != nullptr);

  gst_mini_object_unref (GST_MINI_OBJECT_CAST (codec));
}

int32_t
gst_hdi_class_data_init (GstHDIClassData * classData, AvCodecMime mime,
    CodecType type, uint32_t flags, const gchar * codec_name)
{
  g_return_val_if_fail (classData != nullptr, HDI_FAILURE);
  g_return_val_if_fail (codec_name != nullptr, HDI_FAILURE);
  classData->is_soft = flags == 1 ? TRUE : FALSE;
  classData->codec_name = codec_name;
  classData->codec_type = type;
  classData->mime = mime;
  classData->get_caps = FALSE;
  int32_t ret = gst_hdi_get_caps(mime, type, flags, &classData->caps);
  if (ret == HDI_SUCCESS) {
    classData->get_caps = TRUE;
  }
  return ret;
}

const gchar *
gst_hdi_error_to_string (int32_t err)
{
  HDI_ERRORTYPE err_u = (HDI_ERRORTYPE) err;

  switch (err_u) {
    case HDI_FAILURE:
      return "Failed";
    case HDI_SUCCESS:
      return "Successs";
    case HDI_ERR_STREAM_BUF_FULL:
      return "Stream buffer is full";
    case HDI_ERR_FRAME_BUF_EMPTY:
      return "Frame buffer is empty";
    case HDI_RECEIVE_EOS:
      return "Eos";
    case HDI_ERR_INVALID_OP:
      return "Invalid";
    default:
      return "OTHERS";
  }
}


static gboolean
plugin_init (GstPlugin * plugin)
{
  gst_element_register (plugin, "hdih264dec",
      GST_RANK_PRIMARY + 1, GST_TYPE_HDI_H264_DEC);
  gst_element_register (plugin, "hdih265dec",
      GST_RANK_PRIMARY + 1, GST_TYPE_HDI_H265_DEC);

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    _hdi_codec,
    "GStreamer HDI Plug-ins",
    plugin_init,
    PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
