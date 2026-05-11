// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSWINDOWPROXYFACTORY_H
#define QOHOSWINDOWPROXYFACTORY_H

#include <qohosdisplayinfo.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qglobal.h>
#include <QtCore/qrect.h>
#include <functional>
#include <memory>
#include <qohosinternalwindowid_p.h>
#include <qohosplugincore.h>
#include <render/qxcomponent.h>
#include <string>

QT_BEGIN_NAMESPACE

enum class WindowProxyType
{
    FloatWindow,
    MainWindow,
    SubWindow,
};

struct QOhosWindowProxyMainWindowCreateInfo
{
    QtOhos::QObjectThreadSafeRef qWindowRef;
    QtOhos::InternalWindowId windowId;
    std::string windowTitle;
    QRect frameGeometry;
    bool fullscreen = false;
    QOhosOptional<QOhosDisplayInfo::JsDisplayId> displayId;
};

struct QOhosWindowProxyExistingMainWindowCreateInfo
{
    QtOhos::QObjectThreadSafeRef qWindowRef;
    std::string qAbilityInstanceId;
    QtOhos::InternalWindowId windowId;
};

struct QOhosWindowProxySubWindowCreateInfo
{
    QtOhos::QObjectThreadSafeRef window;
    QtOhos::InternalWindowId windowId;
    std::string qAbilityInstanceId;
    std::string windowTitle;
    bool decorEnabled = false;
    bool disableWindowFocusableBeforeLoadContentHack;
    bool modal = false;
};

struct QOhosWindowProxyFloatWindowCreateInfo
{
    QtOhos::QObjectThreadSafeRef qWindowRef;
    QtOhos::InternalWindowId internalWindowId;
    QOhosOptional<QOhosDisplayInfo::JsDisplayId> displayId;
};

struct QOhosWindowProxyData
{
    std::shared_ptr<QtOhos::QAbilityPeer> qAbilityPeer;
    QNapi::Reference<QNapi::Object> jsWindow;
    WindowProxyType windowProxyType;
    std::shared_ptr<QXComponentNode> nodeXComponent;
    std::shared_ptr<void> jsKeepAliveData;
};

void makeWindowProxyDataForMainWindowInJsThread(
    QtOhos::JsState &jsState,
    const QOhosWindowProxyMainWindowCreateInfo &createInfo,
    QOhosConsumer<QtOhos::JsState &, QOhosWindowProxyData> resultConsumer);

void makeWindowProxyDataForExistingMainWindowInJsThread(
    QtOhos::JsState &jsState,
    const QOhosWindowProxyExistingMainWindowCreateInfo &createInfo,
    QOhosConsumer<QtOhos::JsState &, QOhosWindowProxyData> resultConsumer);

void makeWindowProxyDataForSubWindowInJsThread(
    QtOhos::JsState &jsState,
    const QOhosWindowProxySubWindowCreateInfo &createInfo,
    QOhosConsumer<QtOhos::JsState &, QOhosWindowProxyData> resultConsumer);

void makeWindowProxyDataForSubWindowInJsThread(
    QtOhos::JsState &jsState,
    QNapi::Object windowStageOrWindowObject,
    const QOhosWindowProxySubWindowCreateInfo &createInfo,
    QOhosConsumer<QtOhos::JsState &, QOhosWindowProxyData> resultConsumer);

void makeWindowProxyDataForFloatWindowInJsThread(
    QtOhos::JsState &jsState, const QOhosWindowProxyFloatWindowCreateInfo &createInfo,
    QOhosConsumer<QtOhos::JsState &, QOhosWindowProxyData> resultConsumer);

QT_END_NAMESPACE

#endif
