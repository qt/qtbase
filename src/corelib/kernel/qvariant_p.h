/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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
******************************************************************************/

#ifndef QVARIANT_P_H
#define QVARIANT_P_H

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

#include <QtCore/qglobal.h>
#include <QtCore/qvariant.h>
#include <QtCore/private/qmetatype_p.h>

QT_BEGIN_NAMESPACE

template <class T>
inline void v_construct(QVariant::Private *x, const T &t)
{
    if constexpr (QVariant::Private::CanUseInternalSpace<T>) {
        new (&x->data) T(t);
        x->is_shared = false;
    } else {
        x->data.shared = QVariant::PrivateShared::create(QtPrivate::qMetaTypeInterfaceForType<T>());
        new (x->data.shared->data()) T(t);
        x->is_shared = true;
    }
}

QT_END_NAMESPACE

#endif // QVARIANT_P_H
