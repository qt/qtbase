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

#ifndef QDEBUG_P_H
#define QDEBUG_P_H

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

#include <QtCore/private/qglobal_p.h>
#include "QtCore/qdebug.h"
#include "QtCore/qmetaobject.h"
#include "QtCore/qflags.h"

QT_BEGIN_NAMESPACE

namespace QtDebugUtils {

// inline helpers for formatting basic classes.

template <class Point>
static inline void formatQPoint(QDebug &debug, const Point &point)
{
    debug << point.x() << ',' << point.y();
}

template <class Size>
static inline void formatQSize(QDebug &debug, const Size &size)
{
    debug << size.width() << ", " << size.height();
}

template <class Rect>
static inline void formatQRect(QDebug &debug, const Rect &rect)
{
    debug << rect.x() << ',' << rect.y() << ' ' << rect.width() << 'x' << rect.height();
}

template <class Margins>
static inline void formatQMargins(QDebug &debug, const Margins &margins)
{
    debug << margins.left() << ", " << margins.top() << ", " << margins.right()
        << ", " << margins.bottom();
}

#ifndef QT_NO_QOBJECT
template <class QEnum>
static inline void formatQEnum(QDebug &debug, QEnum value)
{
    const QMetaObject *metaObject = qt_getEnumMetaObject(value);
    const QMetaEnum me = metaObject->enumerator(metaObject->indexOfEnumerator(qt_getEnumName(value)));
    if (const char *key = me.valueToKey(int(value)))
        debug << key;
    else
        debug << int(value);
}

template <class QEnum>
static inline void formatNonNullQEnum(QDebug &debug, const char *prefix, QEnum value)
{
    if (value) {
         debug << prefix;
         formatQEnum(debug, value);
    }
}

template <class Enum>
static inline void formatQFlags(QDebug &debug, const QFlags<Enum> &value)
{
    const QMetaObject *metaObject = qt_getEnumMetaObject(Enum());
    const QMetaEnum me = metaObject->enumerator(metaObject->indexOfEnumerator(qt_getEnumName(Enum())));
    const QDebugStateSaver saver(debug);
    debug.noquote();
    debug << me.valueToKeys(value);
}

template <class Enum>
static inline void formatNonNullQFlags(QDebug &debug, const char *prefix, const QFlags<Enum> &value)
{
    if (value) {
        debug << prefix;
        formatQFlags(debug, value);
    }
}

#endif // !QT_NO_QOBJECT

} // namespace QtDebugUtils

QT_END_NAMESPACE

#endif // QDEBUG_P_H
