/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include <QtCore/qglobal.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

namespace QIterablePrivate {

template<typename Callback>
static QVariant retrieveElement(const QMetaType &type, Callback callback)
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
