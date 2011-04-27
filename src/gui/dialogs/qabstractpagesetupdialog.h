/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QABSTRACTPAGESETUPDIALOG_H
#define QABSTRACTPAGESETUPDIALOG_H

#include <QtGui/qdialog.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_PRINTDIALOG

class QAbstractPageSetupDialogPrivate;
class QPrinter;

// ### Qt 5: Remove this class
class Q_GUI_EXPORT QAbstractPageSetupDialog : public QDialog
{
    Q_DECLARE_PRIVATE(QAbstractPageSetupDialog)
    Q_OBJECT

public:
    explicit QAbstractPageSetupDialog(QPrinter *printer, QWidget *parent = 0);
    QAbstractPageSetupDialog(QAbstractPageSetupDialogPrivate &ptr,
                             QPrinter *printer, QWidget *parent = 0);
    ~QAbstractPageSetupDialog();

    virtual int exec() = 0;
    void done(int result);

    QPrinter *printer();
};

#endif // QT_NO_PRINTDIALOG

QT_END_NAMESPACE

QT_END_HEADER

#endif // QABSTRACTPAGESETUPDIALOG_H
