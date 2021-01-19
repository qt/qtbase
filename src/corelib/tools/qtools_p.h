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

#ifndef QTOOLS_P_H
#define QTOOLS_P_H

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

#include "QtCore/private/qglobal_p.h"
#include <limits.h>

QT_BEGIN_NAMESPACE

namespace QtMiscUtils {
Q_DECL_CONSTEXPR inline char toHexUpper(uint value) noexcept
{
    return "0123456789ABCDEF"[value & 0xF];
}

Q_DECL_CONSTEXPR inline char toHexLower(uint value) noexcept
{
    return "0123456789abcdef"[value & 0xF];
}

Q_DECL_CONSTEXPR inline int fromHex(uint c) noexcept
{
    return ((c >= '0') && (c <= '9')) ? int(c - '0') :
           ((c >= 'A') && (c <= 'F')) ? int(c - 'A' + 10) :
           ((c >= 'a') && (c <= 'f')) ? int(c - 'a' + 10) :
           /* otherwise */              -1;
}

Q_DECL_CONSTEXPR inline char toOct(uint value) noexcept
{
    return '0' + char(value & 0x7);
}

Q_DECL_CONSTEXPR inline int fromOct(uint c) noexcept
{
    return ((c >= '0') && (c <= '7')) ? int(c - '0') : -1;
}
}

// We typically need an extra bit for qNextPowerOfTwo when determining the next allocation size.
enum {
    MaxAllocSize = INT_MAX
};

struct CalculateGrowingBlockSizeResult {
    size_t size;
    size_t elementCount;
};

// Implemented in qarraydata.cpp:
size_t Q_CORE_EXPORT Q_DECL_CONST_FUNCTION
qCalculateBlockSize(size_t elementCount, size_t elementSize, size_t headerSize = 0) noexcept;
CalculateGrowingBlockSizeResult Q_CORE_EXPORT Q_DECL_CONST_FUNCTION
qCalculateGrowingBlockSize(size_t elementCount, size_t elementSize, size_t headerSize = 0) noexcept ;

QT_END_NAMESPACE

#endif // QTOOLS_P_H
