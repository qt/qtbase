// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <render/qxcomponent.h>

#include <array>
#include <cstdint>
#include <qohosutils.h>
#include <utility>

QT_BEGIN_NAMESPACE

namespace
{

const std::string xComponentPrefixForMainWindow = "__nnMainWindow_";
const std::string xComponentPrefixForSubWindow = "__nnSubWindow_";
const std::string xComponentPrefixForFloatWindow = "__nnFloatWindow_";
const std::string xComponentPrefixForRenderXComponent = "RenderXComponent_";

bool startsWith(const std::string &str, const std::string &prefix)
{
    return str.compare(0, prefix.size(), prefix) == 0;
}

std::optional<QXComponentId::RecognizedType> tryMapXComponentIdValueToRecognizedType(const std::string &idValue)
{
    static const std::pair<std::string, QXComponentId::RecognizedType> prefixToTypeMapping[] = {
        {xComponentPrefixForMainWindow, QXComponentId::RecognizedType::NativeNodeMainWindow},
        {xComponentPrefixForSubWindow, QXComponentId::RecognizedType::NativeNodeSubWindow},
        {xComponentPrefixForFloatWindow, QXComponentId::RecognizedType::NativeNodeFloatWindow},
        {xComponentPrefixForRenderXComponent, QXComponentId::RecognizedType::RenderXComponent},
    };

    for (const auto &prefixTypePair: prefixToTypeMapping) {
        if (startsWith(idValue, prefixTypePair.first))
            return prefixTypePair.second;
    }

    return {};
}

}

bool QXComponentId::operator==(const QXComponentId &other) const
{
    return m_id == other.m_id;
}

bool QXComponentId::operator!=(const QXComponentId &other) const
{
    return m_id != other.m_id;
}

bool QXComponentId::operator<(const QXComponentId &other) const
{
    return m_id < other.m_id;
}

QXComponentId QXComponentId::createForNativeNodeMainWindow(const std::string &qAbilityInstanceId)
{
    return QXComponentId(xComponentPrefixForMainWindow + qAbilityInstanceId);
}

QXComponentId QXComponentId::createForNativeNodeSubWindow(QtOhos::InternalWindowId windowId)
{
    return QXComponentId(xComponentPrefixForSubWindow + windowId.toStdString());
}

QXComponentId QXComponentId::createForRenderXComponent(QtOhos::InternalWindowId windowId)
{
    return QXComponentId(xComponentPrefixForRenderXComponent + windowId.toStdString());
}

QXComponentId QXComponentId::createForNativeNodeFloatWindow(QtOhos::InternalWindowId windowId)
{
    return QXComponentId(xComponentPrefixForFloatWindow + windowId.toStdString());
}

QXComponentId::QXComponentId(std::string id)
    : m_id(std::move(id))
    , m_optRecognizedType(tryMapXComponentIdValueToRecognizedType(m_id))
{
}

std::string QXComponentId::stringId() const
{
    return m_id;
}

std::optional<QXComponentId> QXComponentId::tryCreateFromXComponent(::OH_NativeXComponent *xComponent)
{
    constexpr auto requiredBufferSize = OH_XCOMPONENT_ID_LEN_MAX + 1;

    if (xComponent == nullptr)
        qOhosReportFatalErrorAndAbort("OH_NativeXComponent was null");

    std::uint64_t xComponentIdLength = requiredBufferSize;
    std::array<char, requiredBufferSize> xComponentIdData = {};
    auto errorCode = ::OH_NativeXComponent_GetXComponentId(
        xComponent,
        xComponentIdData.data(),
        &xComponentIdLength);
    if (errorCode != ::OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        qOhosReportFatalErrorAndAbort(
            "OH_NativeXComponent_GetXComponentId failed with error: %d", errorCode);
    }

    return QXComponentId(
        std::string(
            xComponentIdData.data(),
            xComponentIdLength));
}

QNapi::Value QXComponentId::toNapiValue(napi_env env) const
{
    return QNapi::String::New(env, m_id);
}

std::optional<QXComponentId::RecognizedType> QXComponentId::recognizedType() const
{
    return m_optRecognizedType;
}

QT_END_NAMESPACE
