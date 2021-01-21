/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QMEMROTATE_P_H
#define QMEMROTATE_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include "private/qdrawhelper_p.h"

QT_BEGIN_NAMESPACE

#define QT_DECL_MEMROTATE(type)                            \
    void Q_GUI_EXPORT qt_memrotate90(const type*, int, int, int, type*, int); \
    void Q_GUI_EXPORT qt_memrotate180(const type*, int, int, int, type*, int); \
    void Q_GUI_EXPORT qt_memrotate270(const type*, int, int, int, type*, int)

QT_DECL_MEMROTATE(quint32);
QT_DECL_MEMROTATE(quint16);
QT_DECL_MEMROTATE(quint24);
QT_DECL_MEMROTATE(quint8);
QT_DECL_MEMROTATE(quint64);

#undef QT_DECL_MEMROTATE

QT_END_NAMESPACE

#endif // QMEMROTATE_P_H
