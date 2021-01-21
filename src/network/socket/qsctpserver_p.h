/****************************************************************************
**
** Copyright (C) 2016 Alex Trotsenko <alex1973tr@gmail.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#ifndef QSCTPSERVER_P_H
#define QSCTPSERVER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "private/qtcpserver_p.h"

QT_BEGIN_NAMESPACE

#ifndef QT_NO_SCTP

class QSctpServerPrivate : public QTcpServerPrivate
{
    Q_DECLARE_PUBLIC(QSctpServer)
public:
    QSctpServerPrivate();
    virtual ~QSctpServerPrivate();

    int maximumChannelCount;

    void configureCreatedSocket() override;
};

#endif // QT_NO_SCTP

QT_END_NAMESPACE

#endif // QSCTPSERVER_P_H
