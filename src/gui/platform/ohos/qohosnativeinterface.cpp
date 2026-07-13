// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtGui/qwindow.h>
#include <QtGui/qpa/qplatformwindow_p.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <QtGui/private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

using namespace QNativeInterface::Private;

/*!
    \class QNativeInterface::Private::QOhosWindow
    \since 6.12
    \internal
    \brief Native interface to a window on OpenHarmony.
    \inmodule QtGui
    \ingroup native-interfaces
*/
QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QOhosWindow);
QT_DEFINE_PRIVATE_NATIVE_INTERFACE(QOhosIntegration);

QWindow *QOhosWindow::fromNative(ArkUI_NodeHandle content)
{
    if (!content)
        return nullptr;

    auto *integration = QGuiApplicationPrivate::platformIntegration();
    const WId id = integration->call<&QOhosIntegration::windowHandle>(content);
    return id ? QWindow::fromWinId(id) : nullptr;
}

QT_END_NAMESPACE
