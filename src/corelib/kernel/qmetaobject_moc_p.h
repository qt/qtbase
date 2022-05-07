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

#if !defined(QMETAOBJECT_P_H) && !defined(UTILS_H)
# error "Include qmetaobject_p.h (or moc's utils.h) before including this file."
#endif

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

#include <QtCore/qbytearray.h>

QT_BEGIN_NAMESPACE

// This function is shared with moc.cpp. This file should be included where needed.
static QByteArray normalizeTypeInternal(const char *t, const char *e)
{
    int len = QtPrivate::qNormalizeType(t, e, nullptr);
    if (len == 0)
        return QByteArray();
    QByteArray result(len, Qt::Uninitialized);
    len = QtPrivate::qNormalizeType(t, e, result.data());
    Q_ASSERT(len == result.size());
    return result;
}

QT_END_NAMESPACE
