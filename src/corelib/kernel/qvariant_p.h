// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
