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

//TESTED_CLASS=
//TESTED_FILES=

#include <qwindowsystem_qws.h>
#include <qpainter.h>
#include <qdesktopwidget.h>
#include <qdirectpainter_qws.h>
#include <private/qwindowsurface_qws_p.h>
#include <private/qdrawhelper_p.h>

class tst_QDirectPainter : public QObject
{
    Q_OBJECT

public:
    tst_QDirectPainter() {}
    ~tst_QDirectPainter() {}

private slots:
    void initTestCase();
    void setGeometry_data();
    void setGeometry();
#ifndef QT_NO_PROCESS
    void regionSynchronization();
#endif
    void reservedSynchronous();

private:
    QWSWindow* getWindow(int windId);
    QColor bgColor;
};

class ColorPainter : public QDirectPainter
{
public:
    ColorPainter(SurfaceFlag flag = NonReserved,
                 const QColor &color = QColor(Qt::red))
        : QDirectPainter(0, flag), c(color) {}

    QColor color() { return c; }

protected:
    void regionChanged(const QRegion &region) {
        QScreen::instance()->solidFill(c, region);
    }

private:
    QColor c;
    QRegion r;
};

Q_DECLARE_METATYPE(QDirectPainter::SurfaceFlag)

void tst_QDirectPainter::initTestCase()
{
    bgColor = QColor(Qt::green);

    QWSServer *server = QWSServer::instance();
    server->setBackground(bgColor);
}

QWSWindow* tst_QDirectPainter::getWindow(int winId)
{
    QWSServer *server = QWSServer::instance();
    foreach (QWSWindow *w, server->clientWindows()) {
        if (w->winId() == winId)
            return w;
    }
    return 0;
}

class ColorWidget : public QWidget
{
public:
    ColorWidget(QWidget *parent = 0, const QColor &c = QColor(Qt::red))
        : QWidget(parent, Qt::FramelessWindowHint), color(c)
    {
        QPalette opaquePalette = palette();
        opaquePalette.setColor(backgroundRole(), color);
        setPalette(opaquePalette);
        setAutoFillBackground(true);
    }

    void paintEvent(QPaintEvent *e) {
        r += e->region();
    }

    void reset() {
        r = QRegion();
    }

    QColor color;
    QRegion r;
};

#define VERIFY_COLOR(rect, color) {                                     \
    const QPixmap pixmap = QPixmap::grabWindow(QDesktopWidget().winId(), \
                                               rect.left(), rect.top(), \
                                               rect.width(), rect.height()); \
    QCOMPARE(pixmap.size(), rect.size());                               \
    QPixmap expectedPixmap(pixmap); /* ensure equal formats */          \
    expectedPixmap.fill(color);                                         \
    QCOMPARE(pixmap, expectedPixmap);                                   \
}

void tst_QDirectPainter::setGeometry_data()
{
    QTest::addColumn<QDirectPainter::SurfaceFlag>("flag");

    QTest::newRow("NonReserved") << QDirectPainter::NonReserved;
    QTest::newRow("Reserved") << QDirectPainter::Reserved;
    QTest::newRow("ReservedSynchronous") << QDirectPainter::ReservedSynchronous;
}

void tst_QDirectPainter::setGeometry()
{
    QFETCH(QDirectPainter::SurfaceFlag, flag);

    const QRect rect(100, 100, 100, 100);
    {
        ColorPainter w(flag);

        w.setGeometry(rect);
        QApplication::processEvents();
        QCOMPARE(w.geometry(), rect);
        VERIFY_COLOR(rect, w.color());
    }
    QApplication::processEvents();
    VERIFY_COLOR(rect, bgColor);
}

#ifndef QT_NO_PROCESS
void tst_QDirectPainter::regionSynchronization()
{
    QRect dpRect(10, 10, 50, 50);

    // Start the direct painter in a different process
    QProcess proc;
    QStringList args;
    args << QString::number(dpRect.x())
         << QString::number(dpRect.y())
         << QString::number(dpRect.width())
         << QString::number(dpRect.height());

    proc.start("runDirectPainter/runDirectPainter", args);
    QVERIFY(proc.waitForStarted(5 * 1000));
    QTest::qWait(1000);
    QApplication::processEvents();
    VERIFY_COLOR(dpRect, Qt::blue); // blue hardcoded in runDirectPainter

    QTime t;
    t.start();
    static int i = 0;
    while (t.elapsed() < 10 * 1000) {
        QApplication::processEvents();

        ColorWidget w;
        w.setGeometry(10, 10, 50, 50);
        const QRect wRect = dpRect.translated(10, 0);
        w.setGeometry(wRect);
        w.show();

        QApplication::processEvents();
        QApplication::processEvents(); //glib event loop workaround
        VERIFY_COLOR(wRect, w.color);
        ++i;
    }
    QVERIFY(i > 100); // sanity check

    proc.kill();
}
#endif

class MyObject : public QObject
{
public:
    MyObject(QObject *p = 0) : QObject(p), lastEvent(0) {}

    bool event(QEvent *e) {
        lastEvent = e;
        return true;
    }

    QEvent *lastEvent;
};

void tst_QDirectPainter::reservedSynchronous()
{
    MyObject o;
    QCoreApplication::postEvent(&o, new QEvent(QEvent::None));
    QDirectPainter p(0, QDirectPainter::ReservedSynchronous);
    p.setRegion(QRect(5, 5, 50, 50));

    // The event loop should not have been executed
    QVERIFY(o.lastEvent == 0);
    QCOMPARE(p.allocatedRegion(), QRegion(QRect(5, 5, 50, 50)));
}

QTEST_MAIN(tst_QDirectPainter)

#include "tst_qdirectpainter.moc"
