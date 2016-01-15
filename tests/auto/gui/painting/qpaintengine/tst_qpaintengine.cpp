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
