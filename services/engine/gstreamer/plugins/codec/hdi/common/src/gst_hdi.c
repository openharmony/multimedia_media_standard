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
#ifdef GST_HDI_PARAM_PILE
#include "hi_comm_vb.h"
#include "mpi_vb.h"
#include "mpi_sys.h"
#include "sdk.h"
#endif
#define GST_HDI_SET_PARAM(PARAM, KEY, VAL, INDEX, MAX_NUM) \
g_return_if_fail(*INDEX < MAX_NUM); \
PARAM[*INDEX].key = KEY; \
PARAM[*INDEX].val = (void*)&(VAL); \
PARAM[*INDEX].size = sizeof(VAL); \
(*INDEX)++; \

#define GST_1080P_STREAM_WIDTH 1920
#define GST_1080P_STREAM_HEIGHT 1088

GST_DEFINE_MINI_OBJECT_TYPE(GstHDICodec, gst_hdi_codec);
static void gst_hdi_codec_free(GstHDICodec *codec);
static constexpr gint HDI_PARAM_MAX_NUM = 30;
static constexpr gint DEFUALT_BUFFER_NUM = 5;
static GHashTable *caps_map = NULL;
#ifdef GST_HDI_PARAM_PILE
static constexpr gint CODEC_TYPE_NUM = 2;
static constexpr gint CODEC_ATTR_NUM = 2;
//the first row show the mine, the second row show the type
static constexpr gint TABLE_CODEC_TYPE[CODEC_TYPE_NUM][CODEC_ATTR_NUM] =
{
    {MEDIA_MIMETYPE_VIDEO_AVC, VIDEO_DECODER},
    {MEDIA_MIMETYPE_VIDEO_HEVC, VIDEO_DECODER}
};
#endif
typedef enum {
    INPUT_DIRECTION,
    OUTPUT_DIRECTION,
} GstHDIDirect;

typedef struct _GstHDIBuffer
{
    InputInfo *input_info;
    OutputInfo *output_info;
    GstHDICodec *codec;
} GstHDIBuffer;

typedef enum {
    GST_HDI_IN,
    GST_HDI_OUT,
} GstHDIDirection;

static void gst_hdi_move_outbuffer_to_dirty_list(GstHDIBuffer *buffer);

#ifdef GST_HDI_PARAM_PILE
static gboolean get_hdi_video_frame_from_outInfo(GstHDIFormat *frame, const OutputInfo *outInfo)
{
    VIDEO_FRAME_INFO_S *stVFrame = (VIDEO_FRAME_INFO_S*)outInfo->vendorPrivate;
    VIDEO_FRAME_S *pVBuf = &stVFrame->stVFrame;
    frame->width = pVBuf->u32Width;
    frame->height = (guint)((pVBuf->u64PhyAddr[1] - pVBuf->u64PhyAddr[0]) / frame->width);
    if (((frame->width > GST_1080P_STREAM_WIDTH) || (frame->height > GST_1080P_STREAM_HEIGHT)) &&
        ((frame->width > GST_1080P_STREAM_HEIGHT) || (frame->height > GST_1080P_STREAM_WIDTH))) {
        GST_ERROR_OBJECT(NULL, "hdi buffer is too large than 1080p");
        return FALSE;
    }
    frame->pts = pVBuf->u64PTS;
    frame->stride = pVBuf->u32Stride[0];
    frame->phy_addr[0] = pVBuf->u64PhyAddr[0];
    frame->phy_addr[1] = pVBuf->u64PhyAddr[1];
    frame->pixel_format = YVU_SEMIPLANAR_420;
    frame->buffer_size = (frame->width * frame->height) * 3 >> 1;
    frame->gst_format = gst_hdi_video_pixelformat_to_gstvideoformat(frame->pixel_format);
    frame->vir_addr = (hi_u8 *)HI_MPI_SYS_MmapCache(frame->phy_addr[0], frame->buffer_size);
    return TRUE;
}

gint gst_hdi_deque_output_buffer_and_format(GstHDICodec *codec, GstBuffer **gst_buffer,
    GstHDIFormat *format, guint timeoutMs)
{
    GST_DEBUG_OBJECT(codec, "deque hdi outbuf");
    g_return_val_if_fail(codec != NULL, HDI_FAILURE);
    g_return_val_if_fail(format != NULL, HDI_FAILURE);
    g_return_val_if_fail(codec->handle != NULL, HDI_FAILURE);
    g_return_val_if_fail(gst_buffer != NULL, HDI_FAILURE);
    g_return_val_if_fail(codec->output_free_buffers != NULL, HDI_ERR_FRAME_BUF_EMPTY);
    OutputInfo *output_info = (OutputInfo*)codec->output_free_buffers->data;
    g_return_val_if_fail(output_info != NULL, HDI_FAILURE);
    int32_t ret = CodecDequeueOutput(codec->handle, timeoutMs, NULL, output_info);
    if (ret != HDI_SUCCESS) {
        GST_ERROR_OBJECT(NULL, "fail to deque output buffer, in error %s", gst_hdi_error_to_string(ret));
        return ret;
    }
    get_hdi_video_frame_from_outInfo(format, output_info);
    GstHDIBuffer *buffer = g_slice_new0(GstHDIBuffer);
    if (buffer == NULL) {
        ret = CodecQueueOutput(codec->handle, output_info, timeoutMs, -1);
        GST_ERROR_OBJECT(NULL, "new buffer failed and queue buffer %s", gst_hdi_error_to_string(ret));
        return HDI_FAILURE;
    }
    codec->output_free_buffers = g_list_remove(codec->output_free_buffers, output_info);
    buffer->codec = codec;
    buffer->output_info = output_info;
    *gst_buffer = gst_buffer_new_wrapped_full((GstMemoryFlags)0, (gpointer)format->vir_addr,
            format->buffer_size, 0, sizeof(GstHDIBuffer), (guint8*)buffer,
            (GDestroyNotify)gst_hdi_move_outbuffer_to_dirty_list);
    if (*gst_buffer == NULL) {
        gst_hdi_move_outbuffer_to_dirty_list(buffer);
        GST_ERROR_OBJECT(NULL, "new wrapped full buffer fail");
        return HDI_FAILURE;
    }
    GST_BUFFER_PTS(*gst_buffer) = gst_util_uint64_scale(output_info->timeStamp, GST_SECOND, G_USEC_PER_SEC);
    return ret;
}
#endif

static void gst_hdi_change_vdec_format_to_params(Param *param,
    const GstHDIFormat *format, gint *actual_size, const gint max_num)
{
    GST_HDI_SET_PARAM(param, KEY_MIMETYPE, format->mime, actual_size, max_num)
    GST_HDI_SET_PARAM(param, KEY_WIDTH, format->width, actual_size, max_num)
    GST_HDI_SET_PARAM(param, KEY_HEIGHT, format->height, actual_size, max_num)
    GST_HDI_SET_PARAM(param, KEY_BUFFERSIZE, format->buffer_size, actual_size, max_num)
    GST_HDI_SET_PARAM(param, KEY_CODEC_TYPE, format->codec_type, actual_size, max_num)
    GST_HDI_SET_PARAM(param, KEY_PIXEL_FORMAT, format->pixel_format, actual_size, max_num)
}

static void gst_hdi_set_format(const Param *param, GstHDIFormat *format)
{
    g_return_if_fail(param != NULL);
    g_return_if_fail(param->val != NULL);
    g_return_if_fail(format != NULL);
    switch (param->key) {
        case KEY_WIDTH:
            format->width = *((guint *)param->val);
            break;
        case KEY_HEIGHT:
            format->height = *((guint *)param->val);
            break;
        case KEY_STRIDE:
            format->stride = *((guint *)param->val);
            break;
        default:
            GST_INFO_OBJECT(NULL, "param key %d not in format", param->key);
    }
}

static void gst_hdi_change_params_to_format(const Param *param,
    GstHDIFormat *format, const gint max_num)
{
    g_return_if_fail(param != NULL);
    g_return_if_fail(format != NULL);
    for (gint index = 0; index < max_num; ++index) {
        gst_hdi_set_format(&param[index], format);
    }
}

GstHDICodec *gst_hdi_codec_new(const GstHDIClassData *cdata, const GstHDIFormat *format)
{
    g_return_val_if_fail(cdata != NULL, NULL);
    g_return_val_if_fail(format != NULL, NULL);
    g_return_val_if_fail(cdata->codec_name != NULL, NULL);
    gint max_size = HDI_PARAM_MAX_NUM;
    Param params[HDI_PARAM_MAX_NUM] = {};
    gint actual_size = 0;
    if (cdata->format_to_params != NULL) {
        cdata->format_to_params(params, format, &actual_size, max_size);
    }
    GstHDICodec *codec = g_slice_new0(GstHDICodec);
    g_return_val_if_fail(codec != NULL, NULL);
    CODEC_HANDLETYPE handle = NULL;

    int32_t ret = CodecCreate(cdata->codec_name, params, actual_size, &handle);
    if (ret != HDI_SUCCESS) {
        GST_ERROR_OBJECT(NULL, "fail to create codec, in error %s", gst_hdi_error_to_string(ret));
        g_slice_free(GstHDICodec, codec);
        return NULL;
    }
    gst_mini_object_init(GST_MINI_OBJECT_CAST(codec), 0, gst_hdi_codec_get_type(), NULL, NULL,
        (GstMiniObjectFreeFunction)gst_hdi_codec_free);
    codec->handle = handle;
    codec->format_to_params = cdata->format_to_params;
    codec->input_buffer_num = DEFUALT_BUFFER_NUM;
    codec->output_buffer_num = DEFUALT_BUFFER_NUM;
    codec->hdi_started = FALSE;
    g_mutex_init(&codec->start_lock);
    return codec;
}

gint gst_hdi_codec_set_params(const GstHDICodec *codec, const GstHDIFormat *format)
{
    g_return_val_if_fail(codec != NULL, HDI_FAILURE);
    g_return_val_if_fail(codec->handle != NULL, HDI_FAILURE);
    g_return_val_if_fail(format != NULL, HDI_FAILURE);
    gint max_size = HDI_PARAM_MAX_NUM;
    Param params[HDI_PARAM_MAX_NUM] = {};
    gint actual_size = 0;
    if (codec->format_to_params != NULL) {
        codec->format_to_params(params, format, &actual_size, max_size);
    }
    int32_t ret = CodecSetParameter(codec->handle, params, actual_size);
    if (ret != HDI_SUCCESS) {
        GST_ERROR_OBJECT(NULL, "fail to set hdi params, in error %s", gst_hdi_error_to_string(ret));
    }
    return ret;
}

gint gst_hdi_codec_get_params(const GstHDICodec *codec, GstHDIFormat *format)
{
    g_return_val_if_fail(codec != NULL, HDI_FAILURE);
    g_return_val_if_fail(codec->handle != NULL, HDI_FAILURE);
    gint max_size = HDI_PARAM_MAX_NUM;
    Param params[HDI_PARAM_MAX_NUM] = {};
    int32_t ret = CodecGetParameter(codec->handle, params, max_size);
    gst_hdi_change_params_to_format(params, format, max_size);
    if (ret != HDI_SUCCESS) {
        GST_ERROR_OBJECT(NULL, "fail to get hdi params, in error %s", gst_hdi_error_to_string(ret));
    }
    return ret;
}

gint gst_hdi_codec_start(GstHDICodec *codec)
{
    GST_DEBUG_OBJECT(codec, "start hdi codec");
    g_return_val_if_fail(codec != NULL, HDI_FAILURE);
    g_return_val_if_fail(codec->handle != NULL, HDI_FAILURE);
    g_mutex_lock(&codec->start_lock);
    if (codec->hdi_started == TRUE) {
        g_mutex_unlock(&codec->start_lock);
        return HDI_SUCCESS;
    }
    codec->hdi_started = TRUE;
    int32_t ret = CodecStart(codec->handle);
    if (ret != HDI_SUCCESS) {
        codec->hdi_started = FALSE;
        GST_ERROR_OBJECT(NULL, "fail to start hdi, in error %s", gst_hdi_error_to_string(ret));
    }
    g_mutex_unlock(&codec->start_lock);
    return ret;
}

gboolean gst_hdi_codec_is_start(GstHDICodec *codec)
{
    g_return_val_if_fail(codec != NULL, FALSE);
    gboolean start = FALSE;
    g_mutex_lock(&codec->start_lock);
    start = codec->hdi_started;
    g_mutex_unlock(&codec->start_lock);
    return start;
}

gint gst_hdi_codec_stop(GstHDICodec *codec)
{
    GST_DEBUG_OBJECT(codec, "stop hdi codec");
    g_return_val_if_fail(codec != NULL, HDI_FAILURE);
    g_return_val_if_fail(codec->handle != NULL, HDI_FAILURE);
    g_mutex_lock(&codec->start_lock);
    if (codec->hdi_started == FALSE) {
        g_mutex_unlock(&codec->start_lock);
        return HDI_SUCCESS;
    }
    codec->hdi_started = FALSE;
    int32_t ret = CodecStop(codec->handle);
    if (ret != HDI_SUCCESS) {
        codec->hdi_started = TRUE;
        GST_ERROR_OBJECT(NULL, "fail to stop hdi, in error %s", gst_hdi_error_to_string(ret));
    }
    g_mutex_unlock(&codec->start_lock);
    return HDI_SUCCESS;
}

gint gst_hdi_port_flush(GstHDICodec *codec, DirectionType directType)
{
    (void)directType;
    GST_DEBUG_OBJECT(codec, "flush hdi port");
    g_return_val_if_fail(codec != NULL, HDI_FAILURE);
    g_return_val_if_fail(codec->handle != NULL, HDI_FAILURE);
    g_mutex_lock(&codec->start_lock);
    if (!codec->hdi_started) {
        g_mutex_unlock(&codec->start_lock);
        return HDI_SUCCESS;
    }
    g_mutex_unlock(&codec->start_lock);
    int32_t ret = CodecFlush(codec->handle, ALL_TYPE);
    if (ret != HDI_SUCCESS) {
        GST_ERROR_OBJECT(NULL, "fail to flush port, in error %s", gst_hdi_error_to_string(ret));
    }
    return ret;
}

static void gst_hdi_release_input_buffers(GstHDICodec *codec)
{
    g_return_if_fail(codec != NULL);
    for (GList *buffer = codec->input_free_buffers; buffer != NULL; buffer = buffer->next) {
        InputInfo *input_info = buffer->data;
        g_slice_free(CodecBufferInfo, input_info->buffers);
        g_slice_free(InputInfo, input_info);
    }
    g_list_free(codec->input_free_buffers);
    codec->input_free_buffers = NULL;
}

static void gst_hdi_release_output_buffers(GstHDICodec *codec)
{
    g_return_if_fail(codec != NULL);
    for (GList *buffer = codec->output_free_buffers; buffer != NULL; buffer = buffer->next) {
        OutputInfo *output_info = buffer->data;
        g_slice_free(CodecBufferInfo, output_info->buffers);
        g_slice_free(OutputInfo, output_info);
    }
    g_list_free(codec->output_free_buffers);
    codec->output_free_buffers = NULL;
    for (GList *buffer = codec->output_dirty_buffers; buffer != NULL; buffer = buffer->next) {
        OutputInfo *output_info = buffer->data;
        if (output_info == NULL) {
            continue;
        }
        if (output_info->buffers != NULL) {
            g_slice_free(CodecBufferInfo, output_info->buffers);
        }
        g_slice_free(OutputInfo, output_info);
    }
    g_list_free(codec->output_dirty_buffers);
    codec->output_dirty_buffers = NULL;
}

static gboolean gst_hdi_alloc_buffer_inner(GstHDICodec *codec, GstHDIDirection direct)
{
    g_return_val_if_fail(codec != NULL, HDI_FAILURE);
    gint len = (direct == GST_HDI_IN) ? codec->input_buffer_num : codec->output_buffer_num;
    gint size = (direct == GST_HDI_IN) ? sizeof(InputInfo) : sizeof(OutputInfo);
    for (gint index = 0; index < len; ++index) {
        void *info = g_malloc(size);
        CodecBufferInfo *buffer_info = g_malloc(sizeof(CodecBufferInfo));
        if (buffer_info == NULL || info == NULL) {
            g_free(info);
            g_free(buffer_info);
            gst_hdi_release_buffers(codec);
            return FALSE;
        }
        if (direct == GST_HDI_IN) {
            ((InputInfo*)info)->bufferCnt = 1;
            ((InputInfo*)info)->buffers = buffer_info;
            codec->input_free_buffers = g_list_append(codec->input_free_buffers, info);
        } else {
            ((OutputInfo*)info)->bufferCnt = 1;
            ((OutputInfo*)info)->buffers = buffer_info;
            codec->output_free_buffers = g_list_append(codec->output_free_buffers, info);
        }
    }
    return TRUE;
}

static gboolean gst_hdi_alloc_input_buffers(GstHDICodec *codec)
{
    g_return_val_if_fail(codec != NULL, HDI_FAILURE);
    return gst_hdi_alloc_buffer_inner(codec, GST_HDI_IN);
}

static gboolean gst_hdi_alloc_output_buffers(GstHDICodec *codec)
{
    g_return_val_if_fail(codec != NULL, HDI_FAILURE);
    return gst_hdi_alloc_buffer_inner(codec, GST_HDI_OUT);
}

gboolean gst_hdi_alloc_buffers(GstHDICodec *codec)
{
    if (!gst_hdi_alloc_input_buffers(codec)) {
        return FALSE;
    }
    if (!gst_hdi_alloc_output_buffers(codec)) {
        gst_hdi_release_input_buffers(codec);
        g_slice_free(GstHDICodec, codec);
        return FALSE;
    }
    return TRUE;
}

void gst_hdi_release_buffers(GstHDICodec *codec)
{
    gst_hdi_release_input_buffers(codec);
    gst_hdi_release_output_buffers(codec);
}

static void gst_hdi_get_gst_buffer_flag(InputInfo *input_info, GstBuffer *gst_buffer)
{
    g_return_if_fail(input_info != NULL);
    g_return_if_fail(gst_buffer != NULL);
    if (!gst_buffer_has_flags(gst_buffer, GST_BUFFER_FLAG_DELTA_UNIT)) {
        input_info->flag = STREAM_FLAG_KEYFRAME;
    }
}

gint gst_hdi_queue_input_buffer(const GstHDICodec *codec, GstBuffer *gst_buffer, guint timeoutMs)
{
    GST_DEBUG_OBJECT(codec, "queue hdi inbuf");
    g_return_val_if_fail(codec != NULL, HDI_FAILURE);
    g_return_val_if_fail(codec->handle != NULL, HDI_FAILURE);
    g_return_val_if_fail(codec->input_free_buffers != NULL, HDI_FAILURE);
    InputInfo *input_buffer = (InputInfo*)codec->input_free_buffers->data;
    g_return_val_if_fail(input_buffer != NULL, HDI_ERR_FRAME_BUF_EMPTY);
    g_return_val_if_fail(input_buffer->buffers != NULL, HDI_ERR_FRAME_BUF_EMPTY);
    if (gst_buffer == NULL) {
        input_buffer->buffers->addr = NULL;
        input_buffer->buffers->length = 0;
        input_buffer->flag = STREAM_FLAG_END_OF_FRAME;
    } else {
        input_buffer->pts = (int64_t)gst_util_uint64_scale(GST_BUFFER_PTS(gst_buffer), G_USEC_PER_SEC, GST_SECOND);
        gst_hdi_get_gst_buffer_flag(input_buffer, gst_buffer);
        gst_hdi_gst_buffer_to_buffer_info(input_buffer->buffers, gst_buffer);
    }
    int32_t ret = CodecQueueInput(codec->handle, input_buffer, timeoutMs);
    if (ret != HDI_SUCCESS) {
        GST_WARNING_OBJECT(NULL, "fail to queue input buffer, in error %s", gst_hdi_error_to_string(ret));
    }
    gst_buffer_unref(gst_buffer);
    return ret;
}

gint gst_hdi_deque_input_buffer(const GstHDICodec *codec, GstBuffer **gst_buffer, guint timeoutMs)
{
    GST_DEBUG_OBJECT(codec, "deque hdi inbuf");
    g_return_val_if_fail(codec != NULL, HDI_FAILURE);
    g_return_val_if_fail(gst_buffer != NULL, HDI_FAILURE);
    g_return_val_if_fail(codec->handle != NULL, HDI_FAILURE);
    g_return_val_if_fail(codec->input_free_buffers != NULL, HDI_ERR_FRAME_BUF_EMPTY);
    InputInfo *input_buffer = (InputInfo*)codec->input_free_buffers->data;
    g_return_val_if_fail(input_buffer != NULL, HDI_ERR_FRAME_BUF_EMPTY);
    int32_t ret = CodecDequeInput(codec->handle, timeoutMs, input_buffer);
    if (ret != HDI_SUCCESS) {
        GST_ERROR_OBJECT(NULL, "fail to deque input buffer, in error %s", gst_hdi_error_to_string(ret));
        return ret;
    }
    g_return_val_if_fail(input_buffer->buffers != NULL, HDI_FAILURE);
    (*gst_buffer) = gst_buffer_new_wrapped_full((GstMemoryFlags)0, (gpointer)input_buffer->buffers->addr,
        input_buffer->buffers->length, 0, 0, NULL, NULL);
    return ret;
}

gint gst_hdi_queue_output_buffers(GstHDICodec *codec, guint timeoutMs)
{
    GST_DEBUG_OBJECT(codec, "queue hdi outbuf");
    g_return_val_if_fail(codec != NULL, HDI_FAILURE);
    g_return_val_if_fail(codec->handle != NULL, HDI_FAILURE);
    int32_t ret = HDI_SUCCESS;
    while (codec->output_dirty_buffers) {
        OutputInfo *output_info = codec->output_dirty_buffers->data;
        g_return_val_if_fail(output_info != NULL, HDI_FAILURE);
        ret = CodecQueueOutput(codec->handle, output_info, timeoutMs, -1);
        if (ret != HDI_SUCCESS) {
            GST_ERROR_OBJECT(NULL, "fail to queue input buffer, in error %s", gst_hdi_error_to_string(ret));
            return ret;
        }
        codec->output_dirty_buffers = g_list_remove(codec->output_dirty_buffers, output_info);
        codec->output_free_buffers = g_list_append(codec->output_free_buffers, output_info);
    }
    return ret;
}

gint gst_hdi_queue_output_buffer(const GstHDICodec *codec, GstBuffer *gst_buffer, guint timeoutMs)
{
    GST_DEBUG_OBJECT(codec, "queue hdi outbuf");
    g_return_val_if_fail(codec != NULL, HDI_FAILURE);
    g_return_val_if_fail(codec->handle != NULL, HDI_FAILURE);
    g_return_val_if_fail(codec->output_free_buffers != NULL, HDI_FAILURE);
    OutputInfo *output_buffer = (OutputInfo*)codec->output_free_buffers->data;
    g_return_val_if_fail(output_buffer != NULL, HDI_FAILURE);
    gst_hdi_gst_buffer_to_buffer_info(output_buffer->buffers, gst_buffer);
    int32_t ret = CodecQueueOutput(codec->handle, output_buffer, timeoutMs, -1);
    if (ret != HDI_SUCCESS) {
        GST_ERROR_OBJECT(NULL, "fail to queue input buffer, in error %s", gst_hdi_error_to_string(ret));
    }
    gst_buffer_unref(gst_buffer);
    return ret;
}

static void gst_hdi_move_outbuffer_to_dirty_list(GstHDIBuffer *buffer)
{
    GstHDICodec *codec = buffer->codec;
    g_return_if_fail(codec != NULL);
    codec->output_dirty_buffers = g_list_append(codec->output_dirty_buffers, buffer->output_info);
    g_slice_free(GstHDIBuffer, buffer);
}

gint gst_hdi_deque_output_buffer(GstHDICodec *codec, GstBuffer **gst_buffer, guint timeoutMs)
{
    GST_DEBUG_OBJECT(codec, "deque hdi outbuf");
    g_return_val_if_fail(codec != NULL, HDI_FAILURE);
    g_return_val_if_fail(codec->handle != NULL, HDI_FAILURE);
    g_return_val_if_fail(gst_buffer != NULL, HDI_FAILURE);
    g_return_val_if_fail(codec->output_free_buffers != NULL, HDI_ERR_FRAME_BUF_EMPTY);
    OutputInfo *output_info = (OutputInfo*)codec->output_free_buffers->data;
    g_return_val_if_fail(output_info != NULL, HDI_FAILURE);
    int32_t ret = CodecDequeueOutput(codec->handle, timeoutMs, NULL, output_info);
    if (ret != HDI_SUCCESS) {
        GST_ERROR_OBJECT(NULL, "fail to deque output buffer, in error %s", gst_hdi_error_to_string(ret));
        return ret;
    }
    GstHDIBuffer *buffer = g_slice_new0(GstHDIBuffer);
    if (buffer == NULL) {
        ret = CodecQueueOutput(codec->handle, output_info, timeoutMs, -1);
        GST_ERROR_OBJECT(NULL, "new buffer failed and queue buffer %s", gst_hdi_error_to_string(ret));
        return HDI_FAILURE;
    }
    g_return_val_if_fail(buffer != NULL, HDI_FAILURE);
    g_return_val_if_fail(output_info->buffers != NULL, HDI_FAILURE);
    codec->output_free_buffers = g_list_remove(codec->output_free_buffers, output_info);
    buffer->codec = codec;
    buffer->output_info = output_info;
    *gst_buffer = gst_buffer_new_wrapped_full((GstMemoryFlags)0, (gpointer)output_info->buffers->addr,
        output_info->buffers->length, 0, sizeof(GstHDIBuffer), (guint8*)buffer,
        (GDestroyNotify)gst_hdi_move_outbuffer_to_dirty_list);
    if (*gst_buffer == NULL) {
        gst_hdi_move_outbuffer_to_dirty_list(buffer);
        GST_ERROR_OBJECT(NULL, "new wrapped full buffer fail");
        return HDI_FAILURE;
    }
    GST_BUFFER_PTS(*gst_buffer) = gst_util_uint64_scale(output_info->timeStamp, GST_SECOND, G_USEC_PER_SEC);
    return ret;
}

static void gst_hdi_codec_free(GstHDICodec *codec)
{
    GST_DEBUG_OBJECT(codec, "destroy hdi");
    g_return_if_fail(codec != NULL);
    if (codec->handle != NULL) {
        int32_t ret = CodecDestroy(codec->handle);
        if (ret != HDI_SUCCESS) {
            GST_ERROR_OBJECT(NULL, "fail to destroy hdi, in error %s", gst_hdi_error_to_string(ret));
        }
    }
    g_mutex_clear(&codec->start_lock);
    g_slice_free(GstHDICodec, codec);
}

gboolean gst_hdi_gst_buffer_to_buffer_info(CodecBufferInfo *buffer, GstBuffer *gst_buffer)
{
    g_return_val_if_fail(buffer != NULL, FALSE);
    g_return_val_if_fail(gst_buffer != NULL, FALSE);
    GstMapInfo info = GST_MAP_INFO_INIT;
    if (!gst_buffer_map(gst_buffer, &info, GST_MAP_READ))
        return FALSE;
    buffer->addr = info.data;
    buffer->length = info.size;
    gst_buffer_unmap(gst_buffer, &info);
    return TRUE;
}

GstHDICodec *gst_hdi_codec_ref(GstHDICodec *codec)
{
    g_return_val_if_fail(codec != NULL, NULL);
    gst_mini_object_ref(GST_MINI_OBJECT_CAST(codec));
    return codec;
}

void gst_hdi_codec_unref(GstHDICodec *codec)
{
    g_return_if_fail(codec != NULL);
    gst_mini_object_unref(GST_MINI_OBJECT_CAST(codec));
}

static void gst_hdi_init_params_func(GstHDIClassData *class_data)
{
    switch (class_data->codec_type) {
        case VIDEO_DECODER:
            class_data->format_to_params = gst_hdi_change_vdec_format_to_params;
            break;
        default:
            break;
    }
}

static void gst_hdi_buffer_mode_support(const CodecCapbility *hdi_cap, AllocateBufferMode mode,
    guint *support, GstHDIBufferModeSupport support_mode)
{
    g_return_if_fail(hdi_cap != NULL);
    g_return_if_fail(support != NULL);
    if (hdi_cap->allocateMask & (guint)mode) {
        (*support) |= (guint)support_mode;
    }
}

void gst_hdi_class_data_init(GstHDIClassData *class_data)
{
    g_return_if_fail(class_data != NULL);
    g_return_if_fail(class_data->codec_name != NULL);
    gst_hdi_init_params_func(class_data);
    class_data->is_soft = FALSE;
    CodecCapbility *hdi_cap = g_hash_table_lookup(caps_map, class_data->codec_name);
    if (hdi_cap == NULL) {
        GST_ERROR_OBJECT(class_data, "can not find caps");
        return;
    }
    class_data->codec_type = hdi_cap->type;
    class_data->mime = hdi_cap->mime;
    class_data->max_width = hdi_cap->maxSize.width;
    class_data->max_height = hdi_cap->maxSize.height;
    class_data->input_buffer_support = 0;
    class_data->output_buffer_support = 0;
    gst_hdi_buffer_mode_support(hdi_cap, ALLOCATE_INPUT_BUFFER_CODEC,
        &class_data->input_buffer_support, GST_HDI_BUFFER_INTERNAL_SUPPORT);
    gst_hdi_buffer_mode_support(hdi_cap, ALLOCATE_INPUT_BUFFER_USER,
        &class_data->input_buffer_support, GST_HDI_BUFFER_EXTERNAL_SUPPORT);
    gst_hdi_buffer_mode_support(hdi_cap, ALLOCATE_OUTPUT_BUFFER_CODEC,
        &class_data->output_buffer_support, GST_HDI_BUFFER_INTERNAL_SUPPORT);
    gst_hdi_buffer_mode_support(hdi_cap, ALLOCATE_OUTPUT_BUFFER_USER,
        &class_data->output_buffer_support, GST_HDI_BUFFER_EXTERNAL_SUPPORT);

    ResizableArray *format_array = &(hdi_cap->supportPixelFormats);
    for (guint index = 0; index < format_array->actualLen; ++index) {
        class_data->support_video_format =
            g_list_append(class_data->support_video_format, &format_array->element[index]);
    }
}

void gst_hdi_class_pad_caps_init(const GstHDIClassData *class_data, GstElementClass *element_class)
{
    g_return_if_fail(class_data != NULL);
    GstCaps *sink_caps = class_data->sink_caps;
    GstCaps *src_caps = class_data->src_caps;
    if (sink_caps == NULL) {
        sink_caps = gst_caps_from_string(class_data->default_sink_template_caps);
    }
    if (src_caps == NULL) {
        src_caps = gst_caps_from_string(class_data->default_src_template_caps);
    }

    if (src_caps != NULL) {
        GstPadTemplate *src_templ = gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS, src_caps);
        gst_element_class_add_pad_template(element_class, src_templ);
        gst_caps_unref(src_caps);
    }
    if (sink_caps != NULL) {
        GstPadTemplate *sink_templ = gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS, sink_caps);
        gst_element_class_add_pad_template(element_class, sink_templ);
        gst_caps_unref(sink_caps);
    }
}

const gchar *gst_hdi_error_to_string(gint err)
{
    HDI_ERRORTYPE err_u = (HDI_ERRORTYPE)err;

    switch (err_u) {
        case HDI_FAILURE:
            return "Failed";
        case HDI_SUCCESS:
            return "Success";
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

static gchar *gst_hdi_codec_mime_of_cap(const CodecCapbility *caps)
{
    gchar *codec_mime = NULL;
    gchar *codec_type = NULL;
    gchar *codec_name = NULL;
    switch (caps->mime) {
        case MEDIA_MIMETYPE_IMAGE_JPEG:
            codec_mime = "hdijpeg";
            break;
        case MEDIA_MIMETYPE_VIDEO_AVC:
            codec_mime = "hdih264";
            break;
        case MEDIA_MIMETYPE_VIDEO_HEVC:
            codec_mime = "hdih265";
            break;
        case MEDIA_MIMETYPE_AUDIO_AAC:
            codec_mime = "hdiaac";
            break;
        case MEDIA_MIMETYPE_AUDIO_G711A:
            codec_mime = "hdig711a";
            break;
        case MEDIA_MIMETYPE_AUDIO_G711U:
            codec_mime = "hdig711u";
            break;
        case MEDIA_MIMETYPE_AUDIO_G726:
            codec_mime = "hdig726";
            break;
        case MEDIA_MIMETYPE_AUDIO_PCM:
            codec_mime = "hdipcm";
            break;
        case MEDIA_MIMETYPE_AUDIO_MP3:
            codec_mime = "hdimp3";
            break;
        default:
            GST_WARNING_OBJECT(NULL, "unknown mime %d", caps->mime);
    }
    if (caps->type == VIDEO_DECODER || caps->type == AUDIO_DECODER) {
        codec_type = "dec";
    } else if (caps->type == VIDEO_ENCODER || caps->type == AUDIO_ENCODER) {
        codec_type = "enc";
    }
    if (codec_mime != NULL && codec_type != NULL) {
        codec_name = g_strdup_printf("%s%s", codec_mime, codec_type);
    }
    return codec_name;
}

GHashTable *gst_hdi_init_caps_map(void)
{
    caps_map = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    gchar *codec_name = NULL;
    int32_t index = 0;
    int32_t ret = HDI_SUCCESS;
    do {
        CodecCapbility *caps = g_malloc(sizeof(CodecCapbility));
        if (caps == NULL) {
            GST_ERROR_OBJECT(NULL, "new caps index: %d", index);
            break;
        }
#ifdef GST_HDI_PARAM_PILE
        if (index < CODEC_TYPE_NUM) {
            ret = CodecGetCapbility(TABLE_CODEC_TYPE[index][0], TABLE_CODEC_TYPE[index][1], 0, caps);
        } else {
            ret = HDI_FAILURE;
        }
#else
// we can not know soft or hardware from caps, this is a problem
        ret = CodecEnumerateCapbility(index, caps);
#endif
        if (ret == HDI_FAILURE) {
            GST_ERROR_OBJECT(NULL, "new caps index: %d", index);
            g_free(caps);
            break;
        }
        codec_name = gst_hdi_codec_mime_of_cap(caps);
        if (codec_name != NULL) {
            g_hash_table_insert(caps_map, codec_name, caps);
            codec_name = NULL;
        }
        index++;
    } while (ret == HDI_SUCCESS);

    return caps_map;
}
