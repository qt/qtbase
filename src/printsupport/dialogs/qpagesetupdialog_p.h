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

#ifndef QPAGESETUPDIALOG_P_H
#define QPAGESETUPDIALOG_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// to version without notice, or even be removed.
//
// We mean it.
//
//

#include <QtPrintSupport/private/qtprintsupportglobal_p.h>

#include "private/qdialog_p.h"

#include "qbytearray.h"
#include "qpagesetupdialog.h"
#include "qpointer.h"

QT_REQUIRE_CONFIG(printdialog);

QT_BEGIN_NAMESPACE

class QPrinter;

class QPageSetupDialogPrivate : public QDialogPrivate
{
    Q_DECLARE_PUBLIC(QPageSetupDialog)

public:
    explicit QPageSetupDialogPrivate(QPrinter *printer);

    void setPrinter(QPrinter *newPrinter);

    QPrinter *printer;
    bool ownsPrinter;
    QPointer<QObject> receiverToDisconnectOnClose;
    QByteArray memberToDisconnectOnClose;
};

QT_END_NAMESPACE

#endif // QPAGESETUPDIALOG_P_H
