/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
