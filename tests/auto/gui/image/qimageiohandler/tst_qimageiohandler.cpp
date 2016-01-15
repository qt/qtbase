/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qimageiohandler.h>
#include <qfile.h>

class tst_QImageIOHandler : public QObject
{
Q_OBJECT

public:
    tst_QImageIOHandler();
    virtual ~tst_QImageIOHandler();

private slots:
    void getSetCheck();
};

class MyImageIOHandler : public QImageIOHandler
{
public:
    MyImageIOHandler() : QImageIOHandler() { }
    bool canRead() const { return true; }
    bool read(QImage *) { return true; }
};

tst_QImageIOHandler::tst_QImageIOHandler()
{
}

tst_QImageIOHandler::~tst_QImageIOHandler()
{
}

// Testing get/set functions
void tst_QImageIOHandler::getSetCheck()
{
    MyImageIOHandler obj1;
    // QIODevice * QImageIOHandler::device()
    // void QImageIOHandler::setDevice(QIODevice *)
    QFile *var1 = new QFile;
    obj1.setDevice(var1);
    QCOMPARE(obj1.device(), (QIODevice *)var1);
    obj1.setDevice((QIODevice *)0);
    QCOMPARE(obj1.device(), (QIODevice *)0);
    delete var1;
}

QTEST_MAIN(tst_QImageIOHandler)
#include "tst_qimageiohandler.moc"
