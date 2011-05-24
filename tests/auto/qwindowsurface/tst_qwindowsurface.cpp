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
#include <QPainter>
#include <QPalette>
#include <QWindowsStyle>

#ifndef Q_WS_MAC

#include <private/qwindowsurface_p.h>
#include <QDesktopWidget>
#include <QX11Info>


#include "../../shared/util.h"

class tst_QWindowSurface : public QObject
{
    Q_OBJECT

public:
    tst_QWindowSurface() {}
    ~tst_QWindowSurface() {}

private slots:
    void getSetWindowSurface();
    void flushOutsidePaintEvent();
    void grabWidget();
};

class MyWindowSurface : public QWindowSurface
{
public:
    MyWindowSurface(QWidget *w) : QWindowSurface(w) {}

    QPaintDevice *paintDevice() {
        return &image;
    }

    void flush(QWidget*, const QRegion&, const QPoint&) {
        /* nothing */
    }

private:
    QImage image;
};

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

//from tst_qwidget.cpp
static void VERIFY_COLOR(const QRegion &region, const QColor &color)
{
    const QRegion r = QRegion(region);
    for (int i = 0; i < r.rects().size(); ++i) {
        const QRect rect = r.rects().at(i);
        for (int t = 0; t < 5; t++) {
            const QPixmap pixmap = QPixmap::grabWindow(QDesktopWidget().winId(),
                                                   rect.left(), rect.top(),
                                                   rect.width(), rect.height());
            QCOMPARE(pixmap.size(), rect.size());
            QPixmap expectedPixmap(pixmap); /* ensure equal formats */
            expectedPixmap.fill(color);
            if (pixmap.toImage().pixel(0,0) != QColor(color).rgb() && t < 4 )
            { QTest::qWait(200); continue; }
            QCOMPARE(pixmap.toImage().pixel(0,0), QColor(color).rgb());
            QCOMPARE(pixmap, expectedPixmap);
            break;
        }
    }
}

void tst_QWindowSurface::getSetWindowSurface()
{
    QWidget w;
    QVERIFY(!w.windowSurface());

    w.show();
    QApplication::processEvents();
    QVERIFY(w.windowSurface());

    for (int i = 0; i < 2; ++i) {
        MyWindowSurface *surface = new MyWindowSurface(&w);
        QCOMPARE(w.windowSurface(), (QWindowSurface *)surface);

        w.setWindowSurface(surface);
        QCOMPARE(w.windowSurface(), (QWindowSurface *)surface);
    }
}

void tst_QWindowSurface::flushOutsidePaintEvent()
{
#ifdef Q_WS_X11
    if (QX11Info::isCompositingManagerRunning())
        QSKIP("Test is unreliable with composition manager", SkipAll);
#endif

#ifdef Q_WS_WIN
    if (QSysInfo::WindowsVersion & QSysInfo::WV_VISTA) {
        QTest::qWait(1000);
    }
#endif
    ColorWidget w(0, Qt::red);
    w.setGeometry(10, 100, 50, 50);
    // prevent custom styles from messing up the background
    w.setStyle(new QWindowsStyle);
    w.show();
    QTest::qWaitForWindowShown(&w);

    QApplication::processEvents();
#if defined(Q_WS_QWS)
    QApplication::sendPostedEvents(); //for the glib event loop
#elif defined(Q_WS_S60)
    QTest::qWait(5000);
#endif
    VERIFY_COLOR(w.geometry(), w.color);
    w.reset();

    // trigger a paintEvent() the next time the event loop is entered
    w.update();

    // draw a black rectangle inside the widget
    QWindowSurface *surface = w.windowSurface();
    QVERIFY(surface);
    const QRect rect = surface->rect(&w);
    surface->beginPaint(rect);
    QImage *img = surface->buffer(&w);
    if (img) {
        QPainter p(img);
        p.fillRect(QRect(QPoint(),img->size()), Qt::black);
    }
    surface->endPaint(rect);
    surface->flush(&w, rect, QPoint());

#ifdef Q_WS_QWS
    VERIFY_COLOR(w.geometry(), Qt::black);
#endif

    // the paintEvent() should overwrite the painted rectangle
    QApplication::processEvents();

#if defined(Q_WS_QWS)
    QSKIP("task 176755", SkipAll);
#endif
    VERIFY_COLOR(w.geometry(), w.color);
    QCOMPARE(QRegion(w.rect()), w.r);
    w.reset();
}


void tst_QWindowSurface::grabWidget()
{
    QWidget parentWidget;
    QWidget childWidget(&parentWidget);
    QWidget babyWidget(&childWidget);

    parentWidget.resize(300, 300);
    childWidget.setGeometry(50, 50, 200, 200);
    childWidget.setAutoFillBackground(true);
    babyWidget.setGeometry(50, 50, 100, 100);
    babyWidget.setAutoFillBackground(true);

    QPalette pal = parentWidget.palette();
    pal.setColor(QPalette::Window, QColor(Qt::blue));
    parentWidget.setPalette(pal);

    pal = childWidget.palette();
    pal.setColor(QPalette::Window, QColor(Qt::white));
    childWidget.setPalette(pal);

    pal = babyWidget.palette();
    pal.setColor(QPalette::Window, QColor(Qt::red));
    babyWidget.setPalette(pal);

    // prevent custom styles from messing up the background
    parentWidget.setStyle(new QWindowsStyle);

    babyWidget.show();
    childWidget.show();
    parentWidget.show();
    QTest::qWaitForWindowShown(&parentWidget);

    QPixmap parentPixmap;
    QTRY_COMPARE((parentPixmap = parentWidget.windowSurface()->grabWidget(&parentWidget)).size(),
                  QSize(300,300));
    QPixmap childPixmap = childWidget.windowSurface()->grabWidget(&childWidget);
    QPixmap babyPixmap = babyWidget.windowSurface()->grabWidget(&babyWidget);
    QPixmap parentSubPixmap = parentWidget.windowSurface()->grabWidget(&parentWidget, QRect(25, 25, 100, 100));
    QPixmap childSubPixmap = childWidget.windowSurface()->grabWidget(&childWidget, QRect(55, 55, 50, 50));
    QPixmap childInvalidSubPixmap = childWidget.windowSurface()->grabWidget(&childWidget, QRect(-50, -50, 150, 150));

    QCOMPARE(parentPixmap.size(), QSize(300, 300));
    QCOMPARE(childPixmap.size(), QSize(200, 200));
    QCOMPARE(babyPixmap.size(), QSize(100, 100));
    QCOMPARE(parentSubPixmap.size(), QSize(100, 100));
    QCOMPARE(childSubPixmap.size(), QSize(50, 50));
    QCOMPARE(childInvalidSubPixmap.size(), QSize(100, 100));

    QImage parentImage = parentPixmap.toImage();
    QImage childImage = childPixmap.toImage();
    QImage babyImage = babyPixmap.toImage();
    QImage parentSubImage = parentSubPixmap.toImage();
    QImage childSubImage = childSubPixmap.toImage();
    QImage childInvalidSubImage = childInvalidSubPixmap.toImage();

    QVERIFY(QColor(parentImage.pixel(0, 0)) == QColor(Qt::blue));
    QVERIFY(QColor(parentImage.pixel(75, 75)) == QColor(Qt::white));
    QVERIFY(QColor(parentImage.pixel(149, 149)) == QColor(Qt::red));

    QVERIFY(QColor(childImage.pixel(0, 0)) == QColor(Qt::white));
    QVERIFY(QColor(childImage.pixel(99, 99)) == QColor(Qt::red));

    QVERIFY(QColor(parentSubImage.pixel(0, 0)) == QColor(Qt::blue));
    QVERIFY(QColor(parentSubImage.pixel(30, 30)) == QColor(Qt::white));
    QVERIFY(QColor(parentSubImage.pixel(80, 80)) == QColor(Qt::red));

    QVERIFY(QColor(childSubImage.pixel(0, 0)) == QColor(Qt::red));

    QVERIFY(QColor(childInvalidSubImage.pixel(0, 0)) == QColor(Qt::white));
}

QTEST_MAIN(tst_QWindowSurface)

#else // Q_WS_MAC

QTEST_NOOP_MAIN

#endif

#include "tst_qwindowsurface.moc"
