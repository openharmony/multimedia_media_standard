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

#ifndef GST_VENC_MPEG4_H
#define GST_VENC_MPEG4_H

#ifndef GST_API_EXPORT
#define GST_API_EXPORT __attribute__((visibility("default")))
#endif

#include "gst_venc_base.h"

G_BEGIN_DECLS

#define GST_TYPE_VENC_Mpeg4 \
    (gst_venc_mpeg4_get_type())
#define GST_VENC_Mpeg4(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_VENC_Mpeg4, GstVencMpeg4))
#define GST_VENC_Mpeg4_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_VENC_Mpeg4, GstVencMpeg4Class))
#define GST_VENC_Mpeg4_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), GST_TYPE_VENC_Mpeg4, GstVencMpeg4Class))
#define GST_IS_VENC_Mpeg4(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_VENC_Mpeg4))
#define GST_IS_VENC_Mpeg4_CLASS(obj) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_VENC_Mpeg4))

typedef struct _GstVencMpeg4 GstVencMpeg4;
typedef struct _GstVencMpeg4Class GstVencMpeg4Class;

struct _GstVencMpeg4 {
    GstVencBase parent;
};

struct _GstVencMpeg4Class {
    GstVencBaseClass parent_class;
};

GST_API_EXPORT GType gst_venc_mpeg4_get_type(void);

G_END_DECLS

#endif /* GST_VENC_MPEG4_H */
