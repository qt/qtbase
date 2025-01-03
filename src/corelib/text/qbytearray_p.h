// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
constexpr qsizetype MaxByteArraySize = MaxAllocSize - sizeof(std::remove_pointer<QByteArray::DataPointer>::type) - 1;
constexpr qsizetype MaxStringSize = (MaxAllocSize - sizeof(std::remove_pointer<QByteArray::DataPointer>::type)) / 2 - 1;

QT_END_NAMESPACE

#endif // QBYTEARRAY_P_H
