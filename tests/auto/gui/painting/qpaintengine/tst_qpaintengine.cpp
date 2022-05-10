// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>

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
    bool begin(QPaintDevice *) override { return true; }
    bool end() override { return true; }
    void updateState(const QPaintEngineState &) override {}
    void drawPixmap(const QRectF &, const QPixmap &, const QRectF &) override {}
    Type type() const override { return Raster; }
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
