/****************************************************************************
**
** Copyright (C) 2013 Samuel Gaist <samuel.gaist@edeltech.ch>
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

#ifndef QSESSIONMANAGER_P_H
#define QSESSIONMANAGER_P_H

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
#include <private/qobject_p.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>

#ifndef QT_NO_SESSIONMANAGER

QT_BEGIN_NAMESPACE

class QPlatformSessionManager;

class QSessionManagerPrivate : public QObjectPrivate
{
public:
    QSessionManagerPrivate(const QString &id,
                           const QString &key);

    ~QSessionManagerPrivate();

    QPlatformSessionManager *platformSessionManager;
};

QT_END_NAMESPACE

#endif // QT_NO_SESSIONMANAGER

#endif // QSESSIONMANAGER_P_H
