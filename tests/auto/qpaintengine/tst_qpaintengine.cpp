/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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


#include <QtTest/QtTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qpaintengine.h>
#include <qpixmap.h>

//TESTED_CLASS=
//TESTED_FILES=

class tst_QPaintEngine : public QObject
{
Q_OBJECT

public:
    tst_QPaintEngine();
    virtual ~tst_QPaintEngine();

private slots:
    void getSetCheck();
};

tst_QPaintEngine::tst_QPaintEngine()
{
}

tst_QPaintEngine::~tst_QPaintEngine()
{
}

class MyPaintEngine : public QPaintEngine
{
public:
    MyPaintEngine() : QPaintEngine() {}
    bool begin(QPaintDevice *) { return true; }
    bool end() { return true; }
    void updateState(const QPaintEngineState &) {}
    void drawPixmap(const QRectF &, const QPixmap &, const QRectF &) {}
    Type type() const { return Raster; }
};

// Testing get/set functions
void tst_QPaintEngine::getSetCheck()
{
    MyPaintEngine obj1;
    // QPaintDevice * QPaintEngine::paintDevice()
    // void QPaintEngine::setPaintDevice(QPaintDevice *)
    QPixmap *var1 = new QPixmap;
    obj1.setPaintDevice(var1);
    QCOMPARE((QPaintDevice *)var1, obj1.paintDevice());
    obj1.setPaintDevice((QPaintDevice *)0);
    QCOMPARE((QPaintDevice *)0, obj1.paintDevice());
    delete var1;
}

QTEST_MAIN(tst_QPaintEngine)
#include "tst_qpaintengine.moc"
