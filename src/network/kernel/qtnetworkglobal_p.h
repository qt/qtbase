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

#ifndef QTNETWORKGLOBAL_P_H
#define QTNETWORKGLOBAL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/qtnetworkglobal.h>
#include <QtCore/private/qglobal_p.h>
#include <QtNetwork/private/qtnetwork-config_p.h>

// ### Qt6: Remove
#ifdef QT_DEPRECATED_BEARER_MANAGEMENT
#undef QT_DEPRECATED_BEARER_MANAGEMENT
#endif
#define QT_DEPRECATED_BEARER_MANAGEMENT

// ### Qt6: Remove
#ifdef QT_DEPRECATED_NETWORK_API_5_15
#undef QT_DEPRECATED_NETWORK_API_5_15
#endif
#define QT_DEPRECATED_NETWORK_API_5_15

#ifdef QT_DEPRECATED_NETWORK_API_5_15_X
#undef QT_DEPRECATED_NETWORK_API_5_15_X
#endif
#define QT_DEPRECATED_NETWORK_API_5_15_X(x)

#endif // QTNETWORKGLOBAL_P_H
