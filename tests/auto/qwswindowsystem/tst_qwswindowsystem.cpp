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

#ifdef Q_WS_QWS

//TESTED_CLASS=
//TESTED_FILES=gui/embedded/qwindowsystem_qws.h gui/embedded/qwindowsystem_qws.cpp

#include <qwindowsystem_qws.h>
#include <qpainter.h>
#include <qdesktopwidget.h>
#include <qdirectpainter_qws.h>
#include <qscreen_qws.h>
#include <private/qwindowsurface_qws_p.h>

class tst_QWSWindowSystem : public QObject
{
    Q_OBJECT

public:
    tst_QWSWindowSystem() {}
    ~tst_QWSWindowSystem() {}

private slots:
    void initTestCase();
    void showHideWindow();
    void raiseLowerWindow();
    void windowOpacity();
    void directPainter();
    void setMaxWindowRect();
    void initialGeometry();
    void WA_PaintOnScreen();
    void toplevelMove();
    void dontFlushUnitializedWindowSurfaces();
    void task188025_data();
    void task188025();

private:
    QWSWindow* getWindow(int windId);
    QColor bgColor;
};

class ColorWidget : public QWidget
{
public:
    ColorWidget(const QColor &color = QColor(Qt::red), Qt::WindowFlags f = 0)
        : QWidget(0, f | Qt::FramelessWindowHint), c(color) {}

    QColor color() { return c; }

protected:
    void paintEvent(QPaintEvent*) {
        QPainter p(this);
        p.fillRect(rect(), QBrush(c));
    }

private:
    QColor c;
};

void tst_QWSWindowSystem::initTestCase()
{
    bgColor = QColor(Qt::green);

    QWSServer *server = QWSServer::instance();
    server->setBackground(bgColor);
}

QWSWindow* tst_QWSWindowSystem::getWindow(int winId)
{
    QWSServer *server = QWSServer::instance();
    foreach (QWSWindow *w, server->clientWindows()) {
        if (w->winId() == winId)
            return w;
    }
    return 0;
}

#define VERIFY_COLOR(rect, color) {                                     \
    const QPixmap pixmap = QPixmap::grabWindow(QDesktopWidget().winId(), \
                                               rect.left(), rect.top(), \
                                               rect.width(), rect.height()); \
    QCOMPARE(pixmap.size(), rect.size());                               \
    QPixmap expectedPixmap(pixmap); /* ensure equal formats */          \
    expectedPixmap.fill(color);                                         \
    QCOMPARE(pixmap, expectedPixmap);                                   \
}

void tst_QWSWindowSystem::showHideWindow()
{
    ColorWidget w;

    const QRect rect(100, 100, 100, 100);

    w.setGeometry(rect);
    QApplication::processEvents();

    QWSWindow *win = getWindow(w.winId());
    QVERIFY(win);
    QCOMPARE(win->requestedRegion(), QRegion());
    QCOMPARE(win->allocatedRegion(), QRegion());
    VERIFY_COLOR(rect, bgColor);

    w.show();
    QApplication::processEvents();
    QApplication::sendPostedEvents(); // glib event loop workaround
    QCOMPARE(win->requestedRegion(), QRegion(rect));
    QCOMPARE(win->allocatedRegion(), QRegion(rect));
    VERIFY_COLOR(rect, w.color());

    w.hide();
    QApplication::processEvents();
    QCOMPARE(win->requestedRegion(), QRegion());
    QCOMPARE(win->allocatedRegion(), QRegion());
    VERIFY_COLOR(rect, bgColor);
}

void tst_QWSWindowSystem::raiseLowerWindow()
{
    const QRect rect(100, 100, 100, 100);

    ColorWidget w1(Qt::red);
    w1.setGeometry(rect);
    w1.show();
    QApplication::processEvents();

    ColorWidget w2(Qt::blue);
    w2.setGeometry(rect);
    w2.show();

    QWSWindow *win1 = getWindow(w1.winId());
    QWSWindow *win2 = getWindow(w2.winId());

    QApplication::processEvents();
    QApplication::sendPostedEvents(); // glib event loop workaround
    QCOMPARE(win1->requestedRegion(), QRegion(rect));
    QCOMPARE(win2->requestedRegion(), QRegion(rect));
    QCOMPARE(win1->allocatedRegion(), QRegion());
    QCOMPARE(win2->allocatedRegion(), QRegion(rect));
    VERIFY_COLOR(rect, w2.color());

    w1.raise();
    QApplication::processEvents();
    QCOMPARE(win1->requestedRegion(), QRegion(rect));
    QCOMPARE(win2->requestedRegion(), QRegion(rect));
    QCOMPARE(win1->allocatedRegion(), QRegion(rect));
    QCOMPARE(win2->allocatedRegion(), QRegion());
    VERIFY_COLOR(rect, w1.color());

    w1.lower();
    QApplication::processEvents();
    QCOMPARE(win1->requestedRegion(), QRegion(rect));
    QCOMPARE(win2->requestedRegion(), QRegion(rect));
    QCOMPARE(win1->allocatedRegion(), QRegion());
    QCOMPARE(win2->allocatedRegion(), QRegion(rect));
    VERIFY_COLOR(rect, w2.color());
}

void tst_QWSWindowSystem::windowOpacity()
{
    const QRect rect(100, 100, 100, 100);

    ColorWidget w1(Qt::red);
    w1.setGeometry(rect);
    w1.show();

    QWidget w2(0, Qt::FramelessWindowHint);
    w2.setGeometry(rect);
    w2.show();
    w2.raise();

    QWSWindow *win1 = getWindow(w1.winId());
    QWSWindow *win2 = getWindow(w2.winId());

    QApplication::processEvents();
    QApplication::sendPostedEvents(); // glib event loop workaround
    QCOMPARE(win1->allocatedRegion(), QRegion());
    QCOMPARE(win2->allocatedRegion(), QRegion(rect));
    VERIFY_COLOR(rect, w2.palette().color(w2.backgroundRole()));

    // Make w2 transparent so both widgets are shown.

    w2.setWindowOpacity(0.0);
    QApplication::processEvents();
    QCOMPARE(win1->allocatedRegion(), QRegion(rect));
    QCOMPARE(win2->allocatedRegion(), QRegion(rect));
    VERIFY_COLOR(rect, w1.color());

    w2.setWindowOpacity(1.0);
    QApplication::processEvents();
    QCOMPARE(win1->allocatedRegion(), QRegion());
    QCOMPARE(win2->allocatedRegion(), QRegion(rect));
    VERIFY_COLOR(rect, w2.palette().color(w2.backgroundRole()));

    // Use the palette to make w2 transparent
    QPalette palette = w2.palette();
    palette.setBrush(QPalette::All, QPalette::Background,
                     QColor(255, 255, 255, 0));
    w2.setPalette(palette);
    QApplication::processEvents();
    QApplication::processEvents();
    QCOMPARE(win1->allocatedRegion(), QRegion(rect));
    QCOMPARE(win2->allocatedRegion(), QRegion(rect));
    VERIFY_COLOR(rect, w1.color());

    palette.setBrush(QPalette::All, QPalette::Background,
                     QColor(255, 255, 255, 255));
    w2.setPalette(palette);
    QApplication::processEvents();
    QApplication::processEvents();
    QApplication::processEvents();
    QApplication::sendPostedEvents(); // glib event loop workaround
    QCOMPARE(win1->allocatedRegion(), QRegion());
    QCOMPARE(win2->allocatedRegion(), QRegion(rect));
    VERIFY_COLOR(rect, QColor(255, 255, 255, 255));
}

void tst_QWSWindowSystem::directPainter()
{
    const QRect rect(100, 100, 100, 100);

    ColorWidget w(Qt::red);
    w.setGeometry(rect);
    w.show();

    QWSWindow *win = getWindow(w.winId());

    QApplication::processEvents();
    QCOMPARE(win->allocatedRegion(), QRegion(rect));

    // reserve screen area using the static functions

    QDirectPainter::reserveRegion(QRegion(rect));
    QApplication::processEvents();
    QCOMPARE(win->allocatedRegion(), QRegion());
    QCOMPARE(QDirectPainter::reservedRegion(), QRegion(rect));

    QDirectPainter::reserveRegion(QRegion());
    QApplication::processEvents();
    QCOMPARE(win->allocatedRegion(), QRegion(rect));
    QCOMPARE(QDirectPainter::reservedRegion(), QRegion());

    // reserve screen area using a QDirectPainter object
    {
        QDirectPainter dp;
        dp.setRegion(QRegion(rect));
        dp.lower();

        QWSWindow *dpWin = getWindow(dp.winId());

        QApplication::processEvents();
        QCOMPARE(win->allocatedRegion(), QRegion(rect));
        QCOMPARE(dpWin->allocatedRegion(), QRegion());

        w.lower();
        QApplication::processEvents();
        QCOMPARE(win->allocatedRegion(), QRegion());
        QCOMPARE(dpWin->allocatedRegion(), QRegion(rect));
    }

    QApplication::processEvents();
    QCOMPARE(win->allocatedRegion(), QRegion(rect));
    VERIFY_COLOR(rect, w.color());
}

void tst_QWSWindowSystem::setMaxWindowRect()
{
    QDesktopWidget *desktop = QApplication::desktop();
    QSignalSpy spy(desktop, SIGNAL(workAreaResized(int)));

    const QRect screenRect = desktop->screenGeometry();

    QWidget w;
    w.showMaximized();
    QWidget w2;
    QApplication::processEvents();

    QCOMPARE(spy.count(), 0);
    QCOMPARE(w.frameGeometry(), screenRect);

    QRect available = QRect(screenRect.left(), screenRect.top(),
                            screenRect.right() + 1, screenRect.bottom() - 20 + 1);
    QWSServer::setMaxWindowRect(available);
    QApplication::processEvents();

    QCOMPARE(spy.count(), 1);
    QCOMPARE(desktop->availableGeometry(), available);
    QCOMPARE(w.frameGeometry(), desktop->availableGeometry());

    w.hide();
    QApplication::processEvents();

    QWSServer::setMaxWindowRect(screenRect);
    QCOMPARE(spy.count(), 2);
    w.show();
    QVERIFY(w.isMaximized());
    QCOMPARE(desktop->availableGeometry(), screenRect);
    QCOMPARE(w.frameGeometry(), desktop->availableGeometry());
}

void tst_QWSWindowSystem::initialGeometry()
{
    ColorWidget w(Qt::red);
    w.setGeometry(100, 0, 50, 50);
    w.show();

    const QRect rect(10, 200, 100, 100);
    w.setGeometry(rect);

    QApplication::processEvents();
    QApplication::sendPostedEvents(); // glib event loop workaround

    QCOMPARE(w.frameGeometry(), rect);
    VERIFY_COLOR(rect, QColor(Qt::red));
}

void tst_QWSWindowSystem::WA_PaintOnScreen()
{
    ColorWidget w(Qt::red);
    w.setAttribute(Qt::WA_PaintOnScreen);

    QRect rect;

    QVERIFY(w.testAttribute(Qt::WA_PaintOnScreen));
    rect = QRect(10, 0, 50, 50);
    w.setGeometry(rect);
    w.show();

    QApplication::processEvents();
    QApplication::sendPostedEvents(); // glib event loop workaround
    QWSWindowSurface *surface = static_cast<QWSWindowSurface*>(w.windowSurface());
    QCOMPARE(surface->key(), QLatin1String("OnScreen"));
    QVERIFY(w.testAttribute(Qt::WA_PaintOnScreen));
    VERIFY_COLOR(rect, QColor(Qt::red));

    // move
    rect = QRect(10, 100, 50, 50);
    w.setGeometry(rect);
    QApplication::processEvents();
    QApplication::sendPostedEvents(); // glib event loop workaround
    VERIFY_COLOR(rect, QColor(Qt::red));

    // resize
    rect = QRect(10, 100, 60, 60);
    w.setGeometry(rect);
    QApplication::processEvents();
    QApplication::sendPostedEvents(); // glib event loop workaround
    VERIFY_COLOR(rect, QColor(Qt::red));
}

class DummyMoveSurface : public QWSSharedMemSurface
{
public:
    DummyMoveSurface(QWidget *w) : QWSSharedMemSurface(w) {}
    DummyMoveSurface() : QWSSharedMemSurface() {}

    // doesn't do any move
    QRegion move(const QPoint &, const QRegion &) {
        return QRegion();
    }

    QString key() const { return QLatin1String("dummy"); }
};

class DummyScreen : public QScreen
{
private:
    QScreen *s;

public:

    DummyScreen() : QScreen(0), s(qt_screen) {
        qt_screen = this;
        w = s->width();
        h = s->height();
        dw = s->deviceWidth();
        dh = s->deviceHeight();
        d = s->depth();
        data = s->base();
        lstep = s->linestep();
        physWidth = s->physicalWidth();
        physHeight = s->physicalHeight();
        setPixelFormat(s->pixelFormat());
    }

    ~DummyScreen() {
        qt_screen = s;
    }

    bool initDevice() { return s->initDevice(); }
    bool connect(const QString &displaySpec) {
        return s->connect(displaySpec);
    }
    void disconnect() { s->disconnect(); }
    void setMode(int w, int h, int d) { s->setMode(w, h, d); }
    void exposeRegion(QRegion r, int changing) {
        s->exposeRegion(r, changing);
    }
    void blit(const QImage &img, const QPoint &topLeft, const QRegion &r) {
        s->blit(img, topLeft, r);
    }
    void solidFill(const QColor &color, const QRegion &region) {
        s->solidFill(color, region);
    }
    QWSWindowSurface* createSurface(const QString &key) const {
        if (key == QLatin1String("dummy"))
            return new DummyMoveSurface;
        return s->createSurface(key);
    }
};

void tst_QWSWindowSystem::toplevelMove()
{
    { // default move implementation, opaque window
        ColorWidget w(Qt::red);
        w.show();

        w.setGeometry(50, 50, 50, 50);
        QApplication::processEvents();
        QApplication::sendPostedEvents(); // glib event loop workaround
        VERIFY_COLOR(QRect(50, 50, 50, 50), w.color());
        VERIFY_COLOR(QRect(100, 100, 50, 50), bgColor);

        w.move(100, 100);
        QApplication::processEvents();

        VERIFY_COLOR(QRect(100, 100, 50, 50), w.color());
        VERIFY_COLOR(QRect(50, 50, 50, 50), bgColor);
    }

    { // default move implementation, non-opaque window
        ColorWidget w(Qt::red);
        w.setWindowOpacity(0.5);
        w.show();

        w.setGeometry(50, 50, 50, 50);
        QApplication::processEvents();
//        VERIFY_COLOR(QRect(50, 50, 50, 50), w.color());
        VERIFY_COLOR(QRect(100, 100, 50, 50), bgColor);

        w.move(100, 100);
        QApplication::processEvents();

//        VERIFY_COLOR(QRect(100, 100, 50, 50), w.color());
        VERIFY_COLOR(QRect(50, 50, 50, 50), bgColor);
    }

    DummyScreen *screen = new DummyScreen;
    { // dummy accelerated move

        ColorWidget w(Qt::red);
        w.setWindowSurface(new DummyMoveSurface(&w));
        w.show();

        w.setGeometry(50, 50, 50, 50);
        QApplication::processEvents();
        QApplication::sendPostedEvents(); // glib event loop workaround
        VERIFY_COLOR(QRect(50, 50, 50, 50), w.color());
        VERIFY_COLOR(QRect(100, 100, 50, 50), bgColor);

        w.move(100, 100);
        QApplication::processEvents();
        // QEXPECT_FAIL("", "Task 169976", Continue);
        //VERIFY_COLOR(QRect(50, 50, 50, 50), w.color()); // unchanged
        VERIFY_COLOR(QRect(100, 100, 50, 50), bgColor); // unchanged
    }
    delete screen;
}

static void fillWindowSurface(QWidget *w, const QColor &color)
{
    QWindowSurface *s = w->windowSurface();
    const QRect rect = s->rect(w);
    s->beginPaint(rect);
    QImage *img = s->buffer(w);
    QPainter p(img);
    p.fillRect(rect, color);
    s->endPaint(rect);
}

void tst_QWSWindowSystem::dontFlushUnitializedWindowSurfaces()
{
    QApplication::processEvents();

    const QRect r(50, 50, 50, 50);
    QDirectPainter p(0, QDirectPainter::ReservedSynchronous);
    p.setRegion(r);
    QCOMPARE(p.allocatedRegion(), QRegion(r));

    { // Opaque widget, tests the blitting path in QScreen::compose()
        ColorWidget w(Qt::red);
        w.setGeometry(r);
        w.show();
        QCOMPARE(w.visibleRegion(), QRegion());

        // At this point w has a windowsurface but it's completely covered by
        // the directpainter so nothing will be painted here and the
        // windowsurface contains uninitialized data.

        QApplication::processEvents();
        QCOMPARE(p.allocatedRegion(), QRegion(r));
        QCOMPARE(w.visibleRegion(), QRegion());
        fillWindowSurface(&w, Qt::black); // fill with "uninitialized" data

        p.setRegion(QRegion());

        QCOMPARE(w.visibleRegion(), QRegion());
        VERIFY_COLOR(r, bgColor); // don't blit uninitialized data
        QTest::qWait(100);

        QApplication::processEvents(); // get new clip region
        QCOMPARE(w.visibleRegion().translated(w.geometry().topLeft()),
                 QRegion(r));

        QApplication::processEvents(); // do paint
        VERIFY_COLOR(r, w.color());
    }

    p.setRegion(r);

    { // Semi-transparent widget, tests the blending path in QScreen::compose()
        ColorWidget w(Qt::red);
        w.setGeometry(r);
        w.setWindowOpacity(0.44);
        w.show();
        QCOMPARE(w.visibleRegion(), QRegion());

        QApplication::processEvents();
        QCOMPARE(p.allocatedRegion(), QRegion(r));
        QCOMPARE(w.visibleRegion(), QRegion());
        fillWindowSurface(&w, Qt::black); // fill with "uninitialized" data

        p.setRegion(QRegion());

        QCOMPARE(w.visibleRegion(), QRegion());
        VERIFY_COLOR(r, bgColor);
        QTest::qWait(100);

        QApplication::processEvents();
        QCOMPARE(w.visibleRegion().translated(w.geometry().topLeft()),
                 QRegion(r));

        QApplication::processEvents();

        // compose expected color
        QImage::Format screenFormat = QScreen::instance()->pixelFormat();
        if (screenFormat == QImage::Format_Invalid)
            screenFormat = QImage::Format_ARGB32_Premultiplied;

        QImage img(1, 1, screenFormat);
        {
            QPainter p(&img);
            p.fillRect(QRect(0, 0, 1, 1), bgColor);
            p.setOpacity(w.windowOpacity());
#if 1
            QImage colorImage(1,1, screenFormat);
            {
                QPainter urk(&colorImage);
                urk.fillRect(QRect(0, 0, 1, 1), w.color());
            }
            p.drawImage(0,0,colorImage);
#else
            p.fillRect(QRect(0, 0, 1, 1), w.color());
#endif
        }
        VERIFY_COLOR(r, img.pixel(0, 0));
    }
}

void tst_QWSWindowSystem::task188025_data()
{
    QTest::addColumn<int>("windowFlags");

    QTest::newRow("normal") << 0;
    QTest::newRow("paintonscreen") << int(Qt::WA_PaintOnScreen | Qt::Window);
}

void tst_QWSWindowSystem::task188025()
{
    QFETCH(int, windowFlags);
    QRect r(-25, 50, 50, 50);

    ColorWidget w(Qt::red, Qt::WindowFlags(windowFlags));
    w.setGeometry(r);
    w.show();
    QApplication::processEvents();
    QApplication::sendPostedEvents(); // glib event loop workaround

    const QRect visible(0, 50, 25, 50);
    const QPoint topLeft = w.frameGeometry().topLeft();
    QCOMPARE(w.visibleRegion(), QRegion(visible.translated(-topLeft)));
    VERIFY_COLOR(visible, Qt::red);

    w.setMask(QRect(25, 0, 25, 50));
    QApplication::processEvents();
    QCOMPARE(w.visibleRegion(), QRegion(visible.translated(-topLeft)));
    VERIFY_COLOR(visible, Qt::red);

    // extend widget to the right (mask prevents new geometry to be exposed)
    r = r.adjusted(0, 0, 25, 0);
    w.setGeometry(r);
    QApplication::processEvents();
    QCOMPARE(w.visibleRegion(), QRegion(visible.translated(-topLeft)));
    VERIFY_COLOR(visible, Qt::red);
}

QTEST_MAIN(tst_QWSWindowSystem)

#include "tst_qwswindowsystem.moc"

#else // Q_WS_QWS
QTEST_NOOP_MAIN
#endif
