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

#ifndef QBYTEARRAY_P_H
#define QBYTEARRAY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qbytearray.h>
#include "private/qtools_p.h"

QT_BEGIN_NAMESPACE

// -1 because of the terminating NUL
constexpr qsizetype MaxByteArraySize = MaxAllocSize - sizeof(std::remove_pointer<QByteArray::DataPtr>::type) - 1;
constexpr qsizetype MaxStringSize = (MaxAllocSize - sizeof(std::remove_pointer<QByteArray::DataPtr>::type)) / 2 - 1;

QT_END_NAMESPACE

#endif // QBYTEARRAY_P_H
