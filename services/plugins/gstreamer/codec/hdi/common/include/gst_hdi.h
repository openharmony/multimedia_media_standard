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

#ifndef GST_HDI_H
#define GST_HDI_H

#include "config.h"
#include <gmodule.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/allocators/gstdmabuf.h>
#include <gst/video/gstvideodecoder.h>
#include <unistd.h>

#include "codec_interface.h"
#include "codec_type.h"

#include "hi_comm_vb.h"
#include "mpi_vb.h"
#include "mpi_sys.h"

#define HDI_USECOND_PER_SECOND 1000000

typedef struct _GstHDICodec GstHDICodec;
typedef struct _GstHDIClassData GstHDIClassData;

struct _GstHDICodec {
  GstMiniObject mini_object;

  GstObject *parent;

  CODEC_HANDLETYPE handle;
  GMutex start_lock;
  GMutex lock;
  gboolean hdi_started;
  gboolean hdi_flushing;
};

struct _GstHDIClassData {
  const gchar *codec_name;
  CodecType codec_type;
  AvCodecMime mime;
  gboolean is_soft;
  CodecCapbility caps;
  gboolean get_caps;

  const gchar *default_src_template_caps;
  const gchar *default_sink_template_caps;
};

typedef enum {
  HDI_FAILURE = -1,
  HDI_SUCCESS = 0,
  HDI_ERR_STREAM_BUF_FULL = 100,
  HDI_ERR_FRAME_BUF_EMPTY,
  HDI_RECEIVE_EOS,
  HDI_ERR_INVALID_OP,
} HDI_ERRORTYPE;

typedef enum {
    GST_HDI_BUFFER_INTERNAL_MODE,
    GST_HDI_BUFFER_EXTERNAL_MODE,
} GstHdiBufferMode;

int32_t gst_hdi_get_caps (AvCodecMime mime, CodecType type, uint32_t flags, CodecCapbility *cap);
GstHDICodec * gst_hdi_codec_new (const gchar * codec_name, Param * params, int param_cnt);
int32_t gst_hdi_codec_set_meta (const GstHDICodec * codec, Param *params, int param_cnt);
int32_t gst_hdi_codec_get_meta (const GstHDICodec * codec, Param *params, int param_cnt);
gboolean gst_hdi_buffer_map_buffer (CodecBufferInfo * buffer, GstBuffer * input);
GstHDICodec * gst_hdi_codec_ref (GstHDICodec * codec);
int32_t gst_hdi_codec_start (GstHDICodec * codec);
int32_t gst_hdi_codec_stop (GstHDICodec * codec);
void gst_hdi_codec_unref (GstHDICodec * codec);
int32_t gst_hdi_port_flush (GstHDICodec * codec, DirectionType directType);
int32_t gst_hdi_port_flush_input (GstHDICodec * codec);
int32_t gst_hdi_port_flush_output (GstHDICodec * codec);
int32_t queue_input_buffer (const GstHDICodec * codec, InputInfo * inputData, uint32_t timeoutMs);
int32_t deque_input_buffer (const GstHDICodec * codec, InputInfo * inputData, uint32_t timeoutMs);
int32_t queue_output_buffer (const GstHDICodec * codec, OutputInfo * outInfo, uint32_t timeoutMs);
int32_t deque_output_buffer (const GstHDICodec * codec, OutputInfo * outInfo, uint32_t timeoutMs);
void gst_hdi_set_codec_name (GstHDIClassData * classData, const gchar * codecName);
int32_t gst_hdi_class_data_init (GstHDIClassData * classData, AvCodecMime mime,
    CodecType type, uint32_t flags, const gchar * codec_name);
const gchar * gst_hdi_error_to_string (int32_t err);

GType gst_hdi_codec_get_type(void);
#endif /* GST_HDI_H */