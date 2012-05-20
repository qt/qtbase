/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPAGESETUPDIALOG_H
#define QPAGESETUPDIALOG_H

#include <QtWidgets/qdialog.h>
#include <QtPrintSupport/qtprintsupportglobal.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


#ifndef QT_NO_PRINTDIALOG

class QPrinter;
class QPageSetupDialogPrivate;

class Q_PRINTSUPPORT_EXPORT QPageSetupDialog : public QDialog
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QPageSetupDialog)

public:
    explicit QPageSetupDialog(QPrinter *printer, QWidget *parent = 0);
    explicit QPageSetupDialog(QWidget *parent = 0);
    virtual ~QPageSetupDialog();

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    virtual void setVisible(bool visible);
#endif
    virtual int exec();

#ifdef Q_NO_USING_KEYWORD
#ifndef Q_QDOC
    void open() { QDialog::open(); }
#endif
#else
    using QDialog::open;
#endif
    void open(QObject *receiver, const char *member);

    void done(int result);

    QPrinter *printer();
};

#endif // QT_NO_PRINTDIALOG

QT_END_NAMESPACE

QT_END_HEADER

#endif // QPAGESETUPDIALOG_H
