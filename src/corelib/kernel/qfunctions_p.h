/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience
// of qfunctions_*. This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#ifndef QFUNCTIONS_P_H
#define QFUNCTIONS_P_H

#include <QtCore/private/qglobal_p.h>

#if defined(Q_OS_VXWORKS)
#  include "QtCore/qfunctions_vxworks.h"
#elif defined(Q_OS_NACL)
#  include "QtCore/qfunctions_nacl.h"
#elif defined(Q_OS_WINRT)
#  include "QtCore/qfunctions_winrt.h"
#endif

#endif

