/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <qmargins.h>

Q_DECLARE_METATYPE(QMargins)

//TESTED_CLASS=
//TESTED_FILES=

class tst_QMargins : public QObject
{
    Q_OBJECT

public:
    tst_QMargins();
    virtual ~tst_QMargins();


public slots:
    void init();
    void cleanup();
private slots:
    void getSetCheck();
    void dataStreamCheck();
};

// Testing get/set functions
void tst_QMargins::getSetCheck()
{
    QMargins margins;
    // int QMargins::width()
    // void QMargins::setWidth(int)
    margins.setLeft(0);
    QCOMPARE(0, margins.left());
    margins.setTop(INT_MIN);
    QCOMPARE(INT_MIN, margins.top());
    margins.setBottom(INT_MAX);
    QCOMPARE(INT_MAX, margins.bottom());
    margins.setRight(INT_MAX);
    QCOMPARE(INT_MAX, margins.right());

    margins = QMargins();
    QVERIFY(margins.isNull());
    margins.setLeft(5);
    margins.setRight(5);
    QVERIFY(!margins.isNull());
    QCOMPARE(margins, QMargins(5, 0, 5, 0));
} 

// Testing QDataStream operators
void tst_QMargins::dataStreamCheck()
{
    QByteArray buffer;

    // stream out
    {
        QMargins marginsOut(0,INT_MIN,INT_MAX,6852);
        QDataStream streamOut(&buffer, QIODevice::WriteOnly);
        streamOut << marginsOut;
    }

    // stream in & compare
    {
        QMargins marginsIn;
        QDataStream streamIn(&buffer, QIODevice::ReadOnly);
        streamIn >> marginsIn;

        QCOMPARE(marginsIn.left(), 0);
        QCOMPARE(marginsIn.top(), INT_MIN);
        QCOMPARE(marginsIn.right(), INT_MAX);
        QCOMPARE(marginsIn.bottom(), 6852);
    }
}

tst_QMargins::tst_QMargins()
{
}

tst_QMargins::~tst_QMargins()
{
}

void tst_QMargins::init()
{
}

void tst_QMargins::cleanup()
{
}



QTEST_APPLESS_MAIN(tst_QMargins)
#include "tst_qmargins.moc"
