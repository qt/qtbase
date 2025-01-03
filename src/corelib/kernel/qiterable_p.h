// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QITERABLE_P_H
#define QITERABLE_P_H

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

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

namespace QIterablePrivate {

template<typename Callback>
static QVariant retrieveElement(QMetaType type, Callback callback)
{
    QVariant v(type);
    void *dataPtr;
    if (type == QMetaType::fromType<QVariant>())
        dataPtr = &v;
    else
        dataPtr = v.data();
    callback(dataPtr);
    return v;
}

} // namespace QIterablePrivate

QT_END_NAMESPACE

#endif // QITERABLE_P_H
