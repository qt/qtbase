/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QTNETWORKGLOBAL_H
#define QTNETWORKGLOBAL_H

#include <QtCore/qglobal.h>
#include <QtNetwork/qtnetwork-config.h>

QT_BEGIN_NAMESPACE

#ifndef QT_STATIC
#  if defined(QT_BUILD_NETWORK_LIB)
#    define Q_NETWORK_EXPORT Q_DECL_EXPORT
#  else
#    define Q_NETWORK_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define Q_NETWORK_EXPORT
#endif

// ### Qt6: Remove
// We work around an issue in ICC where it errors out during compilation of Qt by not marking it
// deprecated if ICC is used. We also drop the notice if MSVC 2015 is used because it generated
// warnings in the header in ways that cannot be suppressed.
#if defined(Q_CC_INTEL) || (defined(Q_CC_MSVC) && _MSC_VER < 1910)
#define QT_DEPRECATED_BEARER_MANAGEMENT
#else
#define QT_DEPRECATED_BEARER_MANAGEMENT QT_DEPRECATED_VERSION_5_15
#endif

// ### Qt6: Remove
#define QT_DEPRECATED_NETWORK_API_5_15 QT_DEPRECATED_VERSION_5_15
#define QT_DEPRECATED_NETWORK_API_5_15_X QT_DEPRECATED_VERSION_X_5_15

QT_END_NAMESPACE

#endif

