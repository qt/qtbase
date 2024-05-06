// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

using namespace QNativeInterface::Private;

#if defined(Q_OS_VISIONOS)

/*!
    \class QNativeInterface::QVisionOSApplication
    \since 6.8
    \internal
    \preliminary
    \brief Native interface to QGuiApplication, to be retrieved from QPlatformIntegration.
    \inmodule QtGui
    \ingroup native-interfaces
*/

QT_DEFINE_NATIVE_INTERFACE(QVisionOSApplication);

#endif // Q_OS_VISIONOS

QT_END_NAMESPACE
