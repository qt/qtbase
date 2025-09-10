// Copyright (C) 2016 Alex Trotsenko <alex1973tr@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
