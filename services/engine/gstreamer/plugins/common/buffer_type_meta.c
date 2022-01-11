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

#include "buffer_type_meta.h"

static gboolean gst_buffer_type_meta_init(GstMeta *meta, gpointer params, GstBuffer *buffer)
{
    (void)params;
    (void)buffer;
    g_return_val_if_fail(meta != NULL, FALSE);
    GstBufferTypeMeta *buffer_meta = (GstBufferTypeMeta *)meta;
    buffer_meta->type = BUFFER_TYPE_VIR;
    buffer_meta->buf = (intptr_t)0;
    buffer_meta->offset = 0;
    buffer_meta->length = 0;
    buffer_meta->totalSize = 0;
    buffer_meta->fenceFd = -1;
    buffer_meta->memFlag = 0;
    buffer_meta->bufferFlag = 0;

    return TRUE;
}

GType gst_buffer_type_meta_api_get_type(void)
{
    static volatile GType type = 0;
    static const gchar *tags[] ={ GST_META_TAG_MEMORY_STR, NULL};
    if (g_once_init_enter (&type)) {
        GType _type = gst_meta_api_type_register ("GstBufferTypeMetaAPI", tags);
        g_once_init_leave (&type, _type);
    }
    return type;
}

static gboolean gst_buffer_type_meta_transform(GstBuffer *dest, GstMeta *meta,
    GstBuffer *buffer, GQuark type, gpointer data)
{
    g_return_val_if_fail(dest != NULL, FALSE);
    g_return_val_if_fail(meta != NULL, FALSE);
    g_return_val_if_fail(data != NULL, FALSE);
    g_return_val_if_fail(buffer != NULL, FALSE);
    GstBufferTypeMeta *dMeta, *sMeta;
    sMeta = (GstBufferTypeMeta *)meta;

    if (GST_META_TRANSFORM_IS_COPY (type)) {
        GstMetaTransformCopy *copy = data;

        if (!copy->region) {
            dMeta = (GstBufferTypeMeta *)gst_buffer_add_meta(dest, GST_BUFFER_TYPE_META_INFO, NULL);
            if (!dMeta) {
                return FALSE;
            }
            dMeta->type = sMeta->type;
            dMeta->buf = sMeta->buf;
            dMeta->offset = sMeta->offset;
            dMeta->length = sMeta->length;
            dMeta->totalSize = sMeta->totalSize;
            dMeta->fenceFd = sMeta->fenceFd;
            dMeta->memFlag = sMeta->memFlag;
            dMeta->bufferFlag = sMeta->bufferFlag;
        }
    } else {
        return FALSE;
    }
    return TRUE;
}

const GstMetaInfo *gst_buffer_type_meta_get_info(void)
{
    static const GstMetaInfo *buffer_type_meta_info = NULL;

    if (g_once_init_enter ((GstMetaInfo **)&buffer_type_meta_info)) {
        const GstMetaInfo *meta = gst_meta_register(GST_BUFFER_TYPE_META_API_TYPE, "GstBufferTypeMeta",
            sizeof(GstBufferTypeMeta), (GstMetaInitFunction)gst_buffer_type_meta_init,
            (GstMetaFreeFunction)NULL, gst_buffer_type_meta_transform);
        g_once_init_leave ((GstMetaInfo **)&buffer_type_meta_info, (GstMetaInfo *)meta);
    }
    return buffer_type_meta_info;
}

GstBufferTypeMeta *gst_buffer_get_buffer_type_meta(GstBuffer *buffer)
{
    g_return_val_if_fail(buffer != NULL, FALSE);
    gpointer state = NULL;
    GstBufferTypeMeta *buffer_meta = NULL;
    GstMeta *meta;
    const GstMetaInfo *info = GST_BUFFER_TYPE_META_INFO;

    while ((meta = gst_buffer_iterate_meta(buffer, &state))) {
        if (meta->info->api == info->api) {
            buffer_meta = (GstBufferTypeMeta *)meta;
            return buffer_meta;
        }
    }
    return buffer_meta;
}

GstBufferTypeMeta *gst_buffer_add_buffer_vir_meta(GstBuffer *buffer, intptr_t buf, uint32_t bufferFlag)
{
    g_return_val_if_fail(buffer != NULL, FALSE);
    GstBufferTypeMeta *buffer_meta = NULL;

    buffer_meta = (GstBufferTypeMeta *)gst_buffer_add_meta(buffer, GST_BUFFER_TYPE_META_INFO, NULL);
    g_return_val_if_fail(buffer_meta != NULL, buffer_meta);

    buffer_meta->type = BUFFER_TYPE_VIR;
    buffer_meta->buf = buf;
    buffer_meta->bufferFlag = bufferFlag;
    return buffer_meta;
}

GstBufferTypeMeta *gst_buffer_add_buffer_handle_meta(GstBuffer *buffer, intptr_t buf,
        int32_t fenceFd, uint32_t bufferFlag)
{
    g_return_val_if_fail(buffer != NULL, FALSE);
    GstBufferTypeMeta *buffer_meta = NULL;

    buffer_meta = (GstBufferTypeMeta *)gst_buffer_add_meta(buffer, GST_BUFFER_TYPE_META_INFO, NULL);
    g_return_val_if_fail(buffer_meta != NULL, buffer_meta);

    buffer_meta->type = BUFFER_TYPE_HANDLE;
    buffer_meta->buf = buf;
    buffer_meta->fenceFd = fenceFd;
    buffer_meta->bufferFlag = bufferFlag;
    return buffer_meta;
}

GstBufferTypeMeta *gst_buffer_add_buffer_fd_meta(GstBuffer *buffer, intptr_t buf, uint32_t offset,
        uint32_t length, uint32_t totalSize, uint32_t memFlag, uint32_t bufferFlag)
{
    g_return_val_if_fail(buffer != NULL, FALSE);
    GstBufferTypeMeta *buffer_meta = NULL;

    buffer_meta = (GstBufferTypeMeta *)gst_buffer_add_meta(buffer, GST_BUFFER_TYPE_META_INFO, NULL);
    g_return_val_if_fail(buffer_meta != NULL, buffer_meta);

    buffer_meta->type = BUFFER_TYPE_AVSHMEM;
    buffer_meta->buf = buf;
    buffer_meta->offset = offset;
    buffer_meta->length = length;
    buffer_meta->totalSize = totalSize;
    buffer_meta->memFlag = memFlag;
    buffer_meta->bufferFlag = bufferFlag;
    return buffer_meta;
}