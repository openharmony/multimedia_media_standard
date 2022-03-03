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

#include "gst_surface_pool_src.h"
#include <gst/video/video.h>
#include <sync_fence.h>
#include "gst_consumer_surface_pool.h"
#include "gst_consumer_surface_allocator.h"
#include "media_errors.h"
#include "surface_buffer.h"
#include "buffer_type_meta.h"
#include "scope_guard.h"
#include "display_type.h"

#define gst_surface_pool_src_parent_class parent_class
using namespace OHOS;
namespace {
    constexpr guint MAX_SURFACE_QUEUE_SIZE = 12;
    constexpr guint DEFAULT_SURFACE_QUEUE_SIZE = 8;
    constexpr int32_t DEFAULT_SURFACE_SIZE = 1024 * 1024;
    constexpr int32_t DEFAULT_VIDEO_WIDTH = 1920;
    constexpr int32_t DEFAULT_VIDEO_HEIGHT = 1080;
    constexpr uint32_t STRIDE_ALIGN = 8;
}

GST_DEBUG_CATEGORY_STATIC(gst_surface_pool_src_debug_category);
#define GST_CAT_DEFAULT gst_surface_pool_src_debug_category

static GstStaticPadTemplate gst_src_template =
GST_STATIC_PAD_TEMPLATE("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS(GST_VIDEO_CAPS_MAKE(GST_VIDEO_FORMATS_ALL)));

enum {
    PROP_0,
    PROP_SURFACE,
    PROP_SURFACE_STRIDE,
    PROP_SUSPEND,
    PROP_REPEAT,
    PROP_MAX_FRAME_RATE,
};

G_DEFINE_TYPE(GstSurfacePoolSrc, gst_surface_pool_src, GST_TYPE_MEM_POOL_SRC);

static void gst_surface_pool_src_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void gst_surface_pool_src_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static GstStateChangeReturn gst_surface_pool_src_change_state(GstElement *element, GstStateChange transition);
static gboolean gst_surface_pool_src_create_surface(GstSurfacePoolSrc *src);
static gboolean gst_surface_pool_src_create_pool(GstSurfacePoolSrc *src);
static void gst_surface_pool_src_destroy_surface(GstSurfacePoolSrc *src);
static void gst_surface_pool_src_destroy_pool(GstSurfacePoolSrc *src);
static gboolean gst_surface_pool_src_decide_allocation(GstBaseSrc *basesrc, GstQuery *query);
static GstFlowReturn gst_surface_pool_src_fill(GstBaseSrc *src, guint64 offset, guint size, GstBuffer *buf);
static void gst_surface_pool_src_init_surface(GstSurfacePoolSrc *src);
static gboolean gst_surface_pool_src_send_event(GstElement *element, GstEvent *event);

static void gst_surface_pool_src_class_init(GstSurfacePoolSrcClass *klass)
{
    g_return_if_fail(klass != nullptr);
    GObjectClass *gobject_class = reinterpret_cast<GObjectClass*>(klass);
    GstElementClass *gstelement_class = reinterpret_cast<GstElementClass*>(klass);
    GstBaseSrcClass *gstbasesrc_class = reinterpret_cast<GstBaseSrcClass*>(klass);
    GST_DEBUG_CATEGORY_INIT(gst_surface_pool_src_debug_category, "surfacepoolsrc", 0, "surface pool src base class");
    gobject_class->get_property = gst_surface_pool_src_get_property;
    gobject_class->set_property = gst_surface_pool_src_set_property;

    g_object_class_install_property(gobject_class, PROP_SURFACE,
        g_param_spec_pointer("surface", "Surface", "Surface for buffer",
            (GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_SURFACE_STRIDE,
        g_param_spec_uint("surface-stride", "surface stride",
            "surface buffer stride", 0, G_MAXINT32, STRIDE_ALIGN,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_SUSPEND,
        g_param_spec_boolean("suspend", "Suspend surface", "Suspend surface",
            FALSE, (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_REPEAT,
        g_param_spec_uint("repeat", "Repeat frame", "Repeat previous frame after given milliseconds",
            0, G_MAXUINT32, 0, (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_MAX_FRAME_RATE,
        g_param_spec_uint("max-framerate", "Max frame rate", "Max frame rate",
            0, G_MAXUINT32, 0, (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    gstelement_class->change_state = gst_surface_pool_src_change_state;
    gstelement_class->send_event = gst_surface_pool_src_send_event;
    gstbasesrc_class->fill = gst_surface_pool_src_fill;
    gstbasesrc_class->decide_allocation = gst_surface_pool_src_decide_allocation;
    gst_element_class_set_static_metadata(gstelement_class,
        "surface mem source", "Source/Surface/Pool",
        "Retrieve frame from surface buffer queue with raw data", "OpenHarmony");

    gst_element_class_add_static_pad_template(gstelement_class, &gst_src_template);
}

static void gst_surface_pool_src_init(GstSurfacePoolSrc *surfacesrc)
{
    g_return_if_fail(surfacesrc != nullptr);
    surfacesrc->pool = nullptr;
    surfacesrc->stride = STRIDE_ALIGN;
    surfacesrc->need_flush = FALSE;
    surfacesrc->flushing = FALSE;
}

static void gst_surface_pool_src_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GstSurfacePoolSrc *src = GST_SURFACE_POOL_SRC(object);
    g_return_if_fail(src != nullptr);
    g_return_if_fail(value != nullptr);
    (void)pspec;
    switch (prop_id) {
        case PROP_SURFACE:
            g_value_set_pointer(value, src->producerSurface.GetRefPtr());
            break;
        case PROP_SURFACE_STRIDE:
            g_value_set_uint(value, src->stride);
            break;
        default:
            break;
    }
}

static void gst_surface_pool_src_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    GstSurfacePoolSrc *src = GST_SURFACE_POOL_SRC(object);
    g_return_if_fail(src != nullptr && value != nullptr);
    (void)pspec;
    switch (prop_id) {
        case PROP_SURFACE_STRIDE:
            src->stride = g_value_get_uint(value);
            if (src->stride > INT32_MAX) {
                src->stride = STRIDE_ALIGN;
            }
            break;
        case PROP_SUSPEND:
            g_return_if_fail(src->pool != nullptr);
            g_object_set(src->pool, "suspend", g_value_get_boolean(value), nullptr);
            break;
        case PROP_REPEAT:
            g_return_if_fail(src->pool != nullptr);
            g_object_set(src->pool, "repeat", g_value_get_uint(value), nullptr);
            break;
        case PROP_MAX_FRAME_RATE:
            g_return_if_fail(src->pool != nullptr);
            g_object_set(src->pool, "max-framerate", g_value_get_uint(value), nullptr);
            break;
        default:
            break;
    }
}

static GstFlowReturn gst_surface_pool_src_fill(GstBaseSrc *src, guint64 offset, guint size, GstBuffer *buf)
{
    (void)src;
    (void)offset;
    (void)size;
    GstBufferTypeMeta *meta = gst_buffer_get_buffer_type_meta(buf);
    if (meta != nullptr && (meta->bufferFlag & BUFFER_FLAG_EOS)) {
        return GST_FLOW_EOS;
    }
    return GST_FLOW_OK;
}

static GstStateChangeReturn gst_surface_pool_src_change_state(GstElement *element, GstStateChange transition)
{
    GstSurfacePoolSrc *surfacesrc = GST_SURFACE_POOL_SRC(element);
    g_return_val_if_fail(surfacesrc != nullptr, GST_STATE_CHANGE_FAILURE);
    switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
            g_return_val_if_fail(gst_surface_pool_src_create_surface(surfacesrc) == TRUE, GST_STATE_CHANGE_FAILURE);
            g_return_val_if_fail(gst_surface_pool_src_create_pool(surfacesrc) == TRUE, GST_STATE_CHANGE_FAILURE);
            break;
        case GST_STATE_CHANGE_READY_TO_PAUSED:
            gst_surface_pool_src_init_surface(surfacesrc);
            break;
        default:
            break;
    }
    GstStateChangeReturn ret = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);

    switch (transition) {
        case GST_STATE_CHANGE_READY_TO_NULL:
            gst_surface_pool_src_destroy_pool(surfacesrc);
            gst_surface_pool_src_destroy_surface(surfacesrc);
            GST_OBJECT_LOCK(surfacesrc);
            surfacesrc->need_flush = FALSE;
            GST_OBJECT_UNLOCK(surfacesrc);
            break;
        default:
            break;
    }

    return ret;
}

static gboolean gst_surface_pool_src_create_surface(GstSurfacePoolSrc *surfacesrc)
{
    g_return_val_if_fail(surfacesrc != nullptr, FALSE);
    sptr<Surface> consumerSurface = Surface::CreateSurfaceAsConsumer();
    g_return_val_if_fail(consumerSurface != nullptr, FALSE);
    sptr<IBufferProducer> producer = consumerSurface->GetProducer();
    g_return_val_if_fail(producer != nullptr, FALSE);
    sptr<Surface> producerSurface = Surface::CreateSurfaceAsProducer(producer);
    g_return_val_if_fail(producerSurface != nullptr, FALSE);
    surfacesrc->consumerSurface = consumerSurface;
    surfacesrc->producerSurface = producerSurface;

    GST_DEBUG_OBJECT(surfacesrc, "create surface");
    return TRUE;
}

static gboolean gst_surface_pool_src_send_event(GstElement *element, GstEvent *event)
{
    GstSurfacePoolSrc *surfacesrc = GST_SURFACE_POOL_SRC(element);
    g_return_val_if_fail(surfacesrc != nullptr, FALSE);
    g_return_val_if_fail(event != nullptr, FALSE);
    GST_DEBUG_OBJECT(surfacesrc, "New event %s", GST_EVENT_TYPE_NAME(event));

    switch (GST_EVENT_TYPE(event)) {
        case GST_EVENT_FLUSH_START:
            if (surfacesrc->need_flush == FALSE) {
                GST_DEBUG_OBJECT(surfacesrc, "No need flushing");
                surfacesrc->flushing = FALSE;
                return TRUE;
            }
            surfacesrc->flushing = TRUE;
            break;
        case GST_EVENT_FLUSH_STOP:
            if (surfacesrc->flushing == FALSE) {
                GST_DEBUG_OBJECT(surfacesrc, "No flush start");
                return TRUE;
            }
            surfacesrc->flushing = FALSE;
            break;
        default:
            break;
    }

    return GST_ELEMENT_CLASS(parent_class)->send_event(element, event);
}

static gboolean gst_surface_pool_src_create_pool(GstSurfacePoolSrc *surfacesrc)
{
    GstAllocationParams params;
    gst_allocation_params_init(&params);
    GstBufferPool *pool = gst_consumer_surface_pool_new();
    g_return_val_if_fail(pool != nullptr, FALSE);
    ON_SCOPE_EXIT(0) { gst_object_unref(pool); };
    GstAllocator *allocator = gst_consumer_surface_allocator_new();
    g_return_val_if_fail(allocator != nullptr, FALSE);
    gst_consumer_surface_pool_set_surface(pool, surfacesrc->consumerSurface);
    gst_consumer_surface_allocator_set_surface(allocator, surfacesrc->consumerSurface);
    // init pool config
    GstStructure *config = gst_buffer_pool_get_config(pool);
    gst_buffer_pool_config_set_allocator(config, allocator, &params);
    g_return_val_if_fail(gst_buffer_pool_set_config(pool, config) != TRUE, FALSE);
    surfacesrc->pool = pool;
    CANCEL_SCOPE_EXIT_GUARD(0);
    GST_DEBUG_OBJECT(surfacesrc, "create surface pool");
    return TRUE;
}

static void gst_surface_pool_src_destroy_pool(GstSurfacePoolSrc *src)
{
    gst_object_unref(src->pool);
    src->pool = nullptr;
}

static void gst_surface_pool_src_destroy_surface(GstSurfacePoolSrc *src)
{
    src->consumerSurface = nullptr;
    src->producerSurface = nullptr;
}

static void gst_surface_pool_src_init_surface(GstSurfacePoolSrc *src)
{
    // The internal function do not need judge whether it is empty
    GstMemPoolSrc *memsrc = GST_MEM_POOL_SRC(src);
    sptr<Surface> surface = src->consumerSurface;
    guint width = DEFAULT_VIDEO_WIDTH;
    guint height = DEFAULT_VIDEO_HEIGHT;
    GST_OBJECT_LOCK(memsrc);
    if (memsrc->caps != nullptr) {
        GstVideoInfo info;
        gst_video_info_init(&info);
        gst_video_info_from_caps(&info, memsrc->caps);
        width = static_cast<guint>(info.width);
        height = static_cast<guint>(info.height);
    }
    GST_OBJECT_UNLOCK(memsrc);
    SurfaceError ret = surface->SetUserData("video_width", std::to_string(width));
    if (ret != SURFACE_ERROR_OK) {
        GST_WARNING_OBJECT(src, "Set video width fail");
    }
    ret = surface->SetUserData("video_height", std::to_string(height));
    if (ret != SURFACE_ERROR_OK) {
        GST_WARNING_OBJECT(src, "Set video height fail");
    }
    ret = surface->SetUserData("surface_size", std::to_string(DEFAULT_SURFACE_SIZE));
    if (ret != SURFACE_ERROR_OK) {
        GST_WARNING_OBJECT(src, "Set surface size fail");
    }
    ret = surface->SetDefaultWidthAndHeight(width, height);
    if (ret != SURFACE_ERROR_OK) {
        GST_WARNING_OBJECT(src, "Set surface width and height fail");
    }
}

static int32_t gst_surface_pool_src_gstformat_to_surfaceformat(GstSurfacePoolSrc *surfacesrc, GstVideoInfo *format)
{
    g_return_val_if_fail(surfacesrc != nullptr, -1);
    g_return_val_if_fail(format != nullptr, -1);
    g_return_val_if_fail(format->finfo != nullptr, -1);
    switch (format->finfo->format) {
        case GST_VIDEO_FORMAT_NV21:
            return PIXEL_FMT_YCRCB_420_SP;
        case GST_VIDEO_FORMAT_NV12:
            return PIXEL_FMT_YCBCR_420_SP;
        default:
            GST_ERROR_OBJECT(surfacesrc, "Unknow format");
            break;
    }
    return -1;
}

// The buffer of the graphics is dynamically expanded.
// In order to make the number of buffers used by the graphics correspond to the number of encoders one-to-one.
// it is necessary to apply for the buffers first.
static void gst_surface_pool_src_init_surface_buffer(GstSurfacePoolSrc *surfacesrc)
{
    GstMemPoolSrc *memsrc = GST_MEM_POOL_SRC(surfacesrc);
    gint width = DEFAULT_VIDEO_WIDTH;
    gint height = DEFAULT_VIDEO_HEIGHT;
    int32_t format = -1;
    if (memsrc->caps != nullptr) {
        GstVideoInfo info;
        gst_video_info_init(&info);
        gst_video_info_from_caps(&info, memsrc->caps);
        width = info.width;
        height = info.height;
        format = gst_surface_pool_src_gstformat_to_surfaceformat(surfacesrc, &info);
    }
    std::vector<OHOS::sptr<OHOS::SurfaceBuffer>> buffers;
    OHOS::BufferRequestConfig g_requestConfig;
    g_requestConfig.width = width;
    g_requestConfig.height = height;
    g_requestConfig.strideAlignment = static_cast<gint>(surfacesrc->stride);
    g_requestConfig.format = format;
    g_requestConfig.usage = HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_DMA;
    g_requestConfig.timeout = 0;
    for (uint32_t i = 0; i < surfacesrc->producerSurface->GetQueueSize(); ++i) {
        OHOS::sptr<OHOS::SurfaceBuffer> buffer;
        int32_t releaseFence;
        (void)surfacesrc->producerSurface->RequestBuffer(buffer, releaseFence, g_requestConfig);
        sptr<OHOS::SyncFence> autoFence = new(std::nothrow) OHOS::SyncFence(releaseFence);
        if (autoFence != nullptr) {
            autoFence->Wait(100); // 100ms
        }
        if (buffer != nullptr) {
            buffers.push_back(buffer);
        }
    }
    for (uint32_t i = 0; i < buffers.size(); ++i) {
        if (buffers[i] != nullptr) {
            surfacesrc->producerSurface->CancelBuffer(buffers[i]);
        }
    }
}

static gboolean gst_surface_pool_src_get_pool(GstSurfacePoolSrc *surfacesrc, GstQuery *query, GstCaps *outcaps,
    guint min_buf, guint max_buf)
{
    g_return_val_if_fail(surfacesrc != nullptr && query != nullptr && surfacesrc->consumerSurface != nullptr, FALSE);
    if (surfacesrc->pool == nullptr) {
        return FALSE;
    }
    GstMemPoolSrc *memsrc = GST_MEM_POOL_SRC(surfacesrc);
    memsrc->buffer_num = max_buf;
    gboolean is_video = gst_query_find_allocation_meta(query, GST_VIDEO_META_API_TYPE, nullptr);
    if (is_video) {
        // when video need update size
        GstVideoInfo info;
        gst_video_info_init(&info);
        gst_video_info_from_caps(&info, outcaps);
        memsrc->buffer_size = info.size;
    }
    GST_INFO_OBJECT(surfacesrc, "update buffer num %u", memsrc->buffer_num);
    SurfaceError ret = surfacesrc->consumerSurface->SetQueueSize(memsrc->buffer_num);
    if (ret != SURFACE_ERROR_OK) {
        GST_WARNING_OBJECT(surfacesrc, "set queue size fail");
    }
    gst_surface_pool_src_init_surface_buffer(surfacesrc);
    GstStructure *config = gst_buffer_pool_get_config(surfacesrc->pool);
    if (is_video) {
        gst_buffer_pool_config_add_option(config, GST_BUFFER_POOL_OPTION_VIDEO_META);
    }
    gst_buffer_pool_config_set_params(config, outcaps, memsrc->buffer_size, min_buf, max_buf);
    if (gst_buffer_pool_set_config(surfacesrc->pool, config) != TRUE) {
        GST_WARNING_OBJECT(surfacesrc, "set config failed");
    }
    gst_query_set_nth_allocation_pool(query, 0, surfacesrc->pool, memsrc->buffer_num, min_buf, max_buf);
    GST_DEBUG_OBJECT(surfacesrc, "set surface pool success");
    return TRUE;
}

static gboolean gst_surface_pool_src_decide_allocation(GstBaseSrc *basesrc, GstQuery *query)
{
    GstSurfacePoolSrc *surfacesrc = GST_SURFACE_POOL_SRC(basesrc);
    g_return_val_if_fail(basesrc != nullptr && query != nullptr, FALSE);
    GST_OBJECT_LOCK(surfacesrc);
    surfacesrc->need_flush = TRUE;
    GST_OBJECT_UNLOCK(surfacesrc);
    GstCaps *outcaps = nullptr;
    GstBufferPool *pool = nullptr;
    guint size = 0;
    guint min_buf = DEFAULT_SURFACE_QUEUE_SIZE;
    guint max_buf = DEFAULT_SURFACE_QUEUE_SIZE;

    // get caps and save to video info
    gst_query_parse_allocation(query, &outcaps, nullptr);
    // get pool and pool info from down stream
    if (gst_query_get_n_allocation_pools(query) > 0) {
        gst_query_parse_nth_allocation_pool(query, 0, &pool, &size, &min_buf, &max_buf);
    }
    auto caps_struct = gst_caps_get_structure(outcaps, 0);
    auto mediaType = gst_structure_get_name(caps_struct);
    gboolean isVideo = g_str_has_prefix(mediaType, "video/");
    if (isVideo) {
        gst_query_add_allocation_meta(query, GST_VIDEO_META_API_TYPE, nullptr);
    }
    GstVideoInfo info;
    gst_video_info_init(&info);
    gst_video_info_from_caps(&info, outcaps);
    size = info.size;

    // we use our own pool
    if (pool != nullptr) {
        gst_query_set_nth_allocation_pool(query, 0, nullptr, 0, 0, 0);
        gst_object_unref(pool);
        pool = nullptr;
    }
    if (min_buf > MAX_SURFACE_QUEUE_SIZE || max_buf > MAX_SURFACE_QUEUE_SIZE) {
        min_buf = MAX_SURFACE_QUEUE_SIZE;
        max_buf = MAX_SURFACE_QUEUE_SIZE;
    } else if (min_buf > max_buf) {
        max_buf = min_buf;
    } else {
        max_buf = max_buf == 0 ? DEFAULT_SURFACE_QUEUE_SIZE : max_buf;
    }
    if (gst_surface_pool_src_get_pool(surfacesrc, query, outcaps, min_buf, max_buf)) {
        return TRUE;
    }
    return FALSE;
}
