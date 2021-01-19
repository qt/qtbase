/****************************************************************************
**
** Copyright (C) 2018 Intel Corporation.
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

#ifndef QCBORCOMMON_P_H
#define QCBORCOMMON_P_H

#include "qcborcommon.h"

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

QT_BEGIN_NAMESPACE

#ifdef QT_NO_DEBUG
#  define NDEBUG 1
#endif
#undef assert
#define assert Q_ASSERT

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wunused-function")
QT_WARNING_DISABLE_CLANG("-Wunused-function")
QT_WARNING_DISABLE_CLANG("-Wundefined-internal")

#define CBOR_NO_VALIDATION_API  1
#define CBOR_NO_PRETTY_API      1
#define CBOR_API static inline
#define CBOR_PRIVATE_API static inline
#define CBOR_INLINE_API static inline

#include <cbor.h>

QT_WARNING_POP

Q_DECLARE_TYPEINFO(CborValue, Q_PRIMITIVE_TYPE);

QT_END_NAMESPACE

#endif // QCBORCOMMON_P_H
