/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qpaintengine.h>
#include <qpixmap.h>

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
