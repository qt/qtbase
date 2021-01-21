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

#ifndef QKEYSEQUENCE_P_H
#define QKEYSEQUENCE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qapplication_*.cpp, qwidget*.cpp and qfiledialog.cpp.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include "qkeysequence.h"

#include <algorithm>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_SHORTCUT
struct QKeyBinding
{
    QKeySequence::StandardKey standardKey;
    uchar priority;
    uint shortcut;
    uint platform;
};

class QKeySequencePrivate
{
public:
    enum { MaxKeyCount = 4 }; // also used in QKeySequenceEdit
    inline QKeySequencePrivate() : ref(1)
    {
        std::fill_n(key, uint(MaxKeyCount), 0);
    }
    inline QKeySequencePrivate(const QKeySequencePrivate &copy) : ref(1)
    {
        std::copy(copy.key, copy.key + MaxKeyCount,
                  QT_MAKE_CHECKED_ARRAY_ITERATOR(key, MaxKeyCount));
    }
    QAtomicInt ref;
    int key[MaxKeyCount];
    static QString encodeString(int key, QKeySequence::SequenceFormat format);
    // used in dbusmenu
    Q_GUI_EXPORT static QString keyName(int key, QKeySequence::SequenceFormat format);
    static int decodeString(QString accel, QKeySequence::SequenceFormat format);
};
#endif // QT_NO_SHORTCUT

QT_END_NAMESPACE

#endif //QKEYSEQUENCE_P_H
