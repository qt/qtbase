// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QXCOMPONENT_H
#define QXCOMPONENT_H

#include <QtCore/QtGlobal>
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <ace/xcomponent/native_interface_xcomponent.h>
#include <optional>
#include <qohosinternalwindowid_p.h>
#include <qohosplugincore.h>
#include <string>

QT_BEGIN_NAMESPACE

enum class QXComponentType {
    Render,
    Node,
};

template<QXComponentType Type>
class QXComponent
{
public:
    explicit QXComponent(::OH_NativeXComponent *xc);

    QXComponent(const QXComponent &) = default;
    QXComponent &operator=(const QXComponent &) = default;

    QXComponent(QXComponent &&) = default;
    QXComponent &operator=(QXComponent &&) = default;

    bool operator==(const QXComponent<Type> &other) const;
    bool operator<(const QXComponent<Type> &other) const;

    ::OH_NativeXComponent *handle() const;

private:
    ::OH_NativeXComponent *m_XComponent;
};

using QXComponentRender = QXComponent<QXComponentType::Render>;
using QXComponentNode = QXComponent<QXComponentType::Node>;

class QXComponentId
{
public:
    enum class RecognizedType
    {
        RenderXComponent,
        NativeNodeSubWindow,
        NativeNodeMainWindow,
        NativeNodeFloatWindow,
    };

    QXComponentId(const QXComponentId &other) = default;
    QXComponentId &operator=(const QXComponentId &other) = default;

    QXComponentId(QXComponentId &&other) = default;
    QXComponentId &operator=(QXComponentId &&other) = default;

    bool operator==(const QXComponentId &other) const;
    bool operator!=(const QXComponentId &other) const;
    bool operator<(const QXComponentId &other) const;

    static QXComponentId createForNativeNodeMainWindow(const std::string &qAbilityInstanceId);
    static QXComponentId createForNativeNodeSubWindow(QtOhos::InternalWindowId windowId);
    static QXComponentId createForRenderXComponent(QtOhos::InternalWindowId windowId);
    static QXComponentId createForNativeNodeFloatWindow(QtOhos::InternalWindowId windowId);

    static std::optional<QXComponentId> tryCreateFromXComponent(::OH_NativeXComponent *xComponent);

    std::string stringId() const;
    QNapi::Value toNapiValue(napi_env env) const;

    std::optional<RecognizedType> recognizedType() const;

private:
    QXComponentId(std::string id);

    std::string m_id;
    std::optional<RecognizedType> m_optRecognizedType;
};

template<QXComponentType Type>
QXComponent<Type>::QXComponent(::OH_NativeXComponent *xc) : m_XComponent(xc)
{
    if (xc == nullptr)
        qOhosReportFatalErrorAndAbort("::OH_NativeXComponent was null");
}

template<QXComponentType Type>
bool QXComponent<Type>::operator==(const QXComponent<Type> &other) const
{
    return m_XComponent == other.m_XComponent;
}

template<QXComponentType Type>
bool QXComponent<Type>::operator<(const QXComponent<Type> &other) const
{
    return m_XComponent < other.m_XComponent;
}

template<QXComponentType Type>
::OH_NativeXComponent *QXComponent<Type>::handle() const
{
    return m_XComponent;
}

QT_END_NAMESPACE

#endif
