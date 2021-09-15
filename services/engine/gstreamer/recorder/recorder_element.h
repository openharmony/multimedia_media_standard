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

#ifndef RECORDER_ELEMENT_H
#define RECORDER_ELEMENT_H

#include <memory>
#include <unordered_map>
#include <type_traits>
#include <mutex>
#include <set>
#include <gst/gstelement.h>
#include "nocopyable.h"
#include "recorder_param.h"
#include "recorder_inner_defines.h"
#include "recorder_message_handler.h"

namespace OHOS {
namespace Media {
class RecorderElement : public RecorderMsgHandler {
public:
    struct CreateParam {
        // The source element's description that this element uniquely dependents to.
        // For multiplexer, the description is invalid.
        RecorderSourceDesc srcDesc;
        std::string name;
    };

    explicit RecorderElement(const CreateParam &createParam);
    virtual ~RecorderElement();

    DISALLOW_COPY_AND_MOVE(RecorderElement);

    /**
     * @brief Get this element's sourceId
     * @return sourceId.
     */
    inline int32_t GetSourceId() const
    {
        return desc_.handle_;
    }

    /**
     * @brief Get this element's name
     * @return sourceId.
     */
    inline std::string GetName() const
    {
        return name_;
    }

    /**
     * @brief Initialize the element after it created.
     * @return MSERR_OK if success, or failed.
     */
    virtual int32_t Init() = 0;

    /**
     * @brief Configure the element. This will be called before CheckConfigReady.
     * @param recParam: RecorderParam, this is the base type of specific type's recorder parameter.
     * @return MSERR_OK if success, or failed.
     */
    virtual int32_t Configure(const RecorderParam &recParam)
    {
        (void)recParam;
        return MSERR_OK;
    }

    /**
     * @brief This interface will be called before Prepare to ensure that all static recorder parameters required
     * by this element are configured completely.
     * @return MSERR_OK if success, or failed.
     */
    virtual int32_t CheckConfigReady()
    {
        return MSERR_OK;
    }

    /**
     * @brief This interface is invoked before the corresponding gstreamer element's state changed from NULL to
     * PAUSED, and during the process when the RecorderPipeline's Prepare invoked.
     * @return MSERR_OK if success, or failed.
     */
    virtual int32_t Prepare()
    {
        return MSERR_OK;
    }

    /**
     * @brief This interface is invoked before the corresponding gstreamer element's state changed from PAUSED to
     * PLAYING. and during the process when the RecorderPipeline's Start invoked.
     * @return MSERR_OK if success, or failed.
     */
    virtual int32_t Start()
    {
        return MSERR_OK;
    }

    /**
     * @brief This interface is invoked before the corresponding gstreamer element's state changed from PLAYING to
     * PAUSED. and during the process when the RecorderPipeline's Pause invoked.
     * @return MSERR_OK if success, or failed.
     */
    virtual int32_t Pause()
    {
        return MSERR_OK;
    }

    /**
     * @brief This interface is invoked before the corresponding gstreamer element's state changed from PAUSED to
     * PLAYING. and during the process when the RecorderPipeline's Resume invoked.
     * @return MSERR_OK if success, or failed.
     */
    virtual int32_t Resume()
    {
        return MSERR_OK;
    }

    /**
     * @brief This interface is invoked before the Stop interface called. If it is required to drain out all cached
     * buffer of the whole pipeline, this interface will not be called. The components could send EOS event to
     * relevant gstreamer element through this interface to only drain out all itself cached buffer before changing
     * state to NULL. It is designed to speed up the whole pipeline's stopping process. Only the muxer component need
     * to implement this interface, others will be ignored. The upper layer will decide to whether need to wait the
     * GST_MESSAGE_EOS according to this interface's return value.
     * @return true if it's necessarily to wait for GST_MESSAGE_EOS, or unnecessarily.
     */
    virtual bool DrainAll()
    {
        return true;
    }

    /**
     * @brief This interface is invoked before the corresponding gstreamer element's state changed from PLAYING or
     * PAUSED to NULL. and during the process when the RecorderPipeline's Stop invoked.
     * @return MSERR_OK if success, or failed.
     */
    virtual int32_t Stop()
    {
        return MSERR_OK;
    }

    /**
     * @brief This interface is invoked during the process when the RecorderPipeline's Reset invoked.
     * @return MSERR_OK if success, or failed.
     */
    virtual int32_t Reset()
    {
        return MSERR_OK;
    }

    /**
     * @brief Set dynamic parameter to this element, this will be called after Prepare and before Stop
     * @param recParam: RecorderParam, this is the base type of specific type's recorder parameter.
     * @return MSERR_OK if success, or failed.
     */
    virtual int32_t SetParameter(const RecorderParam &recParam)
    {
        (void)recParam;
        return MSERR_OK;
    }

    /**
     * @brief Get Parameter of this elements
     * @param recParam: RecorderParam, this is the base type of specific type's recorder parameter.
     * @return MSERR_OK if success, or failed.
     */
    virtual int32_t GetParameter(RecorderParam &recParam)
    {
        (void)recParam;
        return MSERR_OK;
    }

    /**
     * @brief Process raw message from gstreamer's element to inner message format.
     * @param msg: GstMessage, this is the raw message from gstreamer
     * @param prettyMsg: RecorderMessage, this is the processed result
     * @return see @RecorderMsgProcResult
     */
    RecorderMsgProcResult OnMessageReceived(GstMessage &rawMsg, RecorderMessage &prettyMsg) final;

    /**
     * @brief Dump all dynamic or static configuration info.
     */
    virtual void Dump() {}

protected:
    /**
     * @brief Mark one parameter successfully configured to this element.
     * @param paramType: the enum value of RecorderParamType
     * @return None.
     */
    void MarkParameter(int32_t paramType)
    {
        (void)configedParams_.insert(paramType);
    }

    /**
     * @brief Check whether the specified type's parameter is configured.
     * @param paramType: the enum value of RecorderParamType
     * @return true if configured, false if not configured.
     */
    bool CheckParameter(int32_t paramType) const
    {
        return configedParams_.find(paramType) != configedParams_.end();
    }

    /**
     * @brief Check whether the all specified type's parameters is configured.
     * @param expectedParams: the enum value set of RecorderParamType
     * @return true if all specified type's parameters configured, false if not all.
     */
    bool CheckAllParamsConfiged(const std::set<int32_t> &expectedParams) const;

    /**
     * @brief Check whether the any one specified type's parameters is configured.
     * @param expectedParams: the enum value set of RecorderParamType
     * @return true if any one specified type's parameters configured, false if no one.
     */
    bool CheckAnyParamConfiged(const std::set<int32_t> &expectedParams) const;

    /**
     * @brief Subclass implement to process the raw message from gstreamer's element
     * @param msg: GstMessage, this is the raw message from gstreamer
     * @param prettyMsg: RecorderMessage, this is the processed result
     * @return see @RecorderMsgProcResult
     */
    virtual RecorderMsgProcResult DoProcessMessage(GstMessage &msg, RecorderMessage &prettyMsg)
    {
        return RecorderMsgProcResult::REC_MSG_PROC_IGNORE;
    }

    friend class RecorderPipelineLinkHelper;

    RecorderSourceDesc desc_;
    std::string name_;
    GstElement *gstElem_ = nullptr;

private:
    std::set<int32_t> configedParams_;
};

/**
 * @brief Provides the factory to register the derived class iinstantiation method and
 * create the RecorderElement instance.
 */
class RecorderElementFactory  {
public:
    static RecorderElementFactory &GetInstance()
    {
        static RecorderElementFactory inst;
        return inst;
    }

    using ElementCreator = std::function<std::shared_ptr<RecorderElement>(const RecorderElement::CreateParam&)>;

    int32_t RegisterElement(const std::string &key, const ElementCreator creator);
    std::shared_ptr<RecorderElement> CreateElement(const std::string key, const RecorderElement::CreateParam &param);

private:
    RecorderElementFactory() = default;
    ~RecorderElementFactory() = default;

    std::unordered_map<std::string, ElementCreator> creatorTbl_;
    std::mutex tblMutex_;
};

template<typename T,
         typename = std::enable_if_t<std::is_base_of_v<RecorderElement, std::decay_t<T>>>>
class RecorderElementRegister {
public:
    RecorderElementRegister(const std::string &key)
    {
        (void)RecorderElementFactory::GetInstance().RegisterElement(
            key, [](const RecorderElement::CreateParam &param) {
                return std::make_shared<std::decay_t<T>>(param);
            }
        );
    }

    ~RecorderElementRegister() = default;
};

/**
 * @brief Provides the RecorderElement register macro to simplify the registeration.
 */
#define REGISTER_RECORDER_ELEMENT(__classname__) \
    static std::shared_ptr<RecorderElementRegister<__classname__>> g_##__classname__##register = \
        std::make_shared<RecorderElementRegister<__classname__>>(#__classname__)
}
}
#endif
