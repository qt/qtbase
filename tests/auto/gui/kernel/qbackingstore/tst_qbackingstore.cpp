// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qwindow.h>
#include <qbackingstore.h>
#include <qpa/qplatformbackingstore.h>
#include <qpa/qplatformintegration.h>
#include <private/qguiapplication_p.h>
#include <qpainter.h>
#include <qscreen.h>

#include <QTest>

#include <QEvent>

// For QSignalSpy slot connections.
Q_DECLARE_METATYPE(Qt::ScreenOrientation)

class tst_QBackingStore : public QObject
{
    Q_OBJECT

private slots:

    void initTestCase_data();
    void init();

    void resize();
    void paint();

    void scrollRectInImage_data();
    void scrollRectInImage();

    void scroll();
    void flush_data();
    void flush();

    void staticContents();

    void childWindowFlush();
};

void tst_QBackingStore::initTestCase_data()
{
    QTest::addColumn<QSurfaceFormat::SwapBehavior>("swapBehavior");

    QTest::newRow("single-buffer") << QSurfaceFormat::SingleBuffer;
    QTest::newRow("double-buffer") << QSurfaceFormat::DoubleBuffer;
}

void tst_QBackingStore::init()
{
    QFETCH_GLOBAL(QSurfaceFormat::SwapBehavior, swapBehavior);

    QSurfaceFormat defaultFormat = QSurfaceFormat::defaultFormat();
    defaultFormat.setSwapBehavior(swapBehavior);
    QSurfaceFormat::setDefaultFormat(defaultFormat);
}

void tst_QBackingStore::resize()
{
    QWindow window;
    window.create();

    QBackingStore backingStore(&window);

    QRect rect(0, 0, 100, 100);
    backingStore.resize(rect.size());
    QCOMPARE(backingStore.size(), rect.size());

    // The paint device should reflect the requested
    // size, taking the window's DPR into account.
    backingStore.beginPaint(rect);
    auto paintDevice = backingStore.paintDevice();
    QCOMPARE(paintDevice->devicePixelRatio(), window.devicePixelRatio());
    QCOMPARE(QSize(paintDevice->width(), paintDevice->height()),
        rect.size() * window.devicePixelRatio());
    backingStore.endPaint();

    // So should the platform backingstore when accessed as an QImage
    QImage image = backingStore.handle()->toImage();
    if (!image.isNull()) // toImage might not be implemented
        QCOMPARE(image.size(), rect.size() * window.devicePixelRatio());
}

void tst_QBackingStore::paint()
{
    QWindow window;
    window.create();

    // The resize() test verifies that the backingstore image
    // has a size that takes the window's DPR into account.
    auto dpr = window.devicePixelRatio();

    QBackingStore backingStore(&window);

    QRect rect(0, 0, 100, 100);
    backingStore.resize(rect.size());

    // Partial fill of a fresh backingstore should not crash
    backingStore.beginPaint(QRect(0, 0, 50, 50));
    backingStore.endPaint();
    backingStore.flush(rect);

    // Two rounds, with flush in between
    for (int i = 0; i < 2; ++i) {
        backingStore.beginPaint(rect);
        QPainter p(backingStore.paintDevice());
        QColor bgColor = i ? Qt::red : Qt::blue;
        QColor fgColor = i ? Qt::green : Qt::yellow;
        p.fillRect(rect, bgColor);
        p.fillRect(QRect(50, 50, 10, 10), fgColor);
        p.end();
        backingStore.endPaint();

        QImage image = backingStore.handle()->toImage();
        if (image.isNull())
            QSKIP("Platform backingstore does not implement toImage");

        // qRound() to the nearest device pixel: matches the rasterizer's edges at fractional dpr
        QCOMPARE(image.pixelColor(qRound(50 * dpr), qRound(50 * dpr)), fgColor);
        QCOMPARE(image.pixelColor(qRound(49 * dpr), qRound(50 * dpr)), bgColor);
        QCOMPARE(image.pixelColor(qRound(50 * dpr), qRound(49 * dpr)), bgColor);
        QCOMPARE(image.pixelColor(qRound(59 * dpr), qRound(59 * dpr)), fgColor);
        QCOMPARE(image.pixelColor(qRound(60 * dpr), qRound(59 * dpr)), bgColor);
        QCOMPARE(image.pixelColor(qRound(59 * dpr), qRound(60 * dpr)), bgColor);

        backingStore.flush(rect);
    }
}

void tst_QBackingStore::scrollRectInImage_data()
{
    QTest::addColumn<QRect>("rect");
    QTest::addColumn<QPoint>("offset");

    QTest::newRow("empty rect") << QRect() << QPoint();
    QTest::newRow("rect outside image") << QRect(-100, -100, 1000, 1000) << QPoint(10, 10);
    QTest::newRow("scroll outside positive") << QRect(10, 10, 10, 10) << QPoint(1000, 1000);
    QTest::newRow("scroll outside negative") << QRect(10, 10, 10, 10) << QPoint(-1000, -1000);

    QTest::newRow("sub-rect positive scroll") << QRect(100, 100, 50, 50) << QPoint(10, 10);
    QTest::newRow("sub-rect negative scroll") << QRect(100, 100, 50, 50) << QPoint(-10, -10);

    QTest::newRow("positive vertical only") << QRect(100, 100, 50, 50) << QPoint(0, 10);
    QTest::newRow("negative vertical only") << QRect(100, 100, 50, 50) << QPoint(0, -10);
    QTest::newRow("positive horizontal only") << QRect(100, 100, 50, 50) << QPoint(10, 0);
    QTest::newRow("negative horizontal only") << QRect(100, 100, 50, 50) << QPoint(-10, 0);

    QTest::newRow("whole rect positive") << QRect(0, 0, 250, 250) << QPoint(10, 10);
    QTest::newRow("whole rect negative") << QRect(0, 0, 250, 250) << QPoint(-10, -10);
}

QT_BEGIN_NAMESPACE
Q_GUI_EXPORT void qt_scrollRectInImage(QImage &, const QRect &, const QPoint &);
QT_END_NAMESPACE

void tst_QBackingStore::scrollRectInImage()
{
    QImage test(250, 250, QImage::Format_ARGB32_Premultiplied);

    QFETCH(QRect, rect);
    QFETCH(QPoint, offset);

    qt_scrollRectInImage(test, rect, offset);
}

void tst_QBackingStore::scroll()
{
    QWindow window;
    window.create();

    // The resize() test verifies that the backingstore image
    // has a size that takes the window's DPR into account.
    auto dpr = window.devicePixelRatio();

    QBackingStore backingStore(&window);
    QRect rect(0, 0, 100, 100);

    // Scrolling a backingstore without a size shouldn't crash
    backingStore.scroll(rect, 10, 10);
    backingStore.scroll(rect, -10, -10);

    backingStore.resize(rect.size());

    // Scrolling a backingstore without painting to it shouldn't crash
    backingStore.scroll(rect, 10, 10);
    backingStore.scroll(rect, -10, -10);

    // Two rounds, with flush in between
    for (int i = 0; i < 2; ++i) {

        backingStore.beginPaint(rect);
        QPainter p(backingStore.paintDevice());
        QColor bgColor = i ? Qt::red : Qt::blue;
        QColor fgColor = i ? Qt::green : Qt::yellow;
        p.fillRect(rect, bgColor);
        p.fillRect(QRect(50, 50, 10, 10), fgColor);
        p.end();
        backingStore.endPaint();

        QImage image = backingStore.handle()->toImage();
        if (image.isNull())
            QSKIP("Platform backingstore does not implement toImage");

        // qRound() to the nearest device pixel: matches the rasterizer's edges at fractional dpr
        QCOMPARE(image.pixelColor(qRound(50 * dpr), qRound(50 * dpr)), fgColor);
        QCOMPARE(image.pixelColor(qRound(49 * dpr), qRound(50 * dpr)), bgColor);
        QCOMPARE(image.pixelColor(qRound(50 * dpr), qRound(49 * dpr)), bgColor);
        QCOMPARE(image.pixelColor(qRound(59 * dpr), qRound(59 * dpr)), fgColor);
        QCOMPARE(image.pixelColor(qRound(60 * dpr), qRound(59 * dpr)), bgColor);
        QCOMPARE(image.pixelColor(qRound(59 * dpr), qRound(60 * dpr)), bgColor);
        image = {};

        bool supportsScroll = backingStore.scroll(QRect(52, 52, 6, 6), -12, -12);
        if (!supportsScroll)
            QSKIP("Platform backingstore does not support scrolling");

        image = backingStore.handle()->toImage();
        QCOMPARE(image.pixelColor(qRound(40 * dpr), qRound(40 * dpr)), fgColor);
        QCOMPARE(image.pixelColor(qRound(39 * dpr), qRound(40 * dpr)), bgColor);
        QCOMPARE(image.pixelColor(qRound(40 * dpr), qRound(39 * dpr)), bgColor);
        QCOMPARE(image.pixelColor(qRound(45 * dpr), qRound(45 * dpr)), fgColor);
        QCOMPARE(image.pixelColor(qRound(46 * dpr), qRound(45 * dpr)), bgColor);
        QCOMPARE(image.pixelColor(qRound(45 * dpr), qRound(46 * dpr)), bgColor);
        image = {};

        backingStore.flush(rect);

        // Scroll again after flush, but before new round of painting
        backingStore.scroll(QRect(52, 52, 6, 6), 12, 12);

        image = backingStore.handle()->toImage();
        QCOMPARE(image.pixelColor(qRound(64 * dpr), qRound(64 * dpr)), fgColor);
        QCOMPARE(image.pixelColor(qRound(63 * dpr), qRound(64 * dpr)), bgColor);
        QCOMPARE(image.pixelColor(qRound(64 * dpr), qRound(63 * dpr)), bgColor);
        QCOMPARE(image.pixelColor(qRound(69 * dpr), qRound(69 * dpr)), fgColor);
        QCOMPARE(image.pixelColor(qRound(70 * dpr), qRound(69 * dpr)), bgColor);
        QCOMPARE(image.pixelColor(qRound(69 * dpr), qRound(70 * dpr)), bgColor);
        image = {};
    }
}

class Window : public QWindow
{
public:
    Window()
        : backingStore(this)
    {
    }

    void resizeEvent(QResizeEvent *) override
    {
        backingStore.resize(size());
    }

    void paintEvent(QPaintEvent *event) override
    {
        QRect rect(QPoint(), size());

        backingStore.beginPaint(rect);

        QPainter p(backingStore.paintDevice());
        p.fillRect(rect, Qt::white);
        p.end();

        backingStore.endPaint();

        backingStore.flush(event->region().boundingRect());
    }

private:
    QBackingStore backingStore;
};

void tst_QBackingStore::flush_data()
{
    QTest::addColumn<Qt::WindowState>("windowState");

    QTest::newRow("normal") << Qt::WindowNoState;
    QTest::newRow("maximized") << Qt::WindowMaximized;
    QTest::newRow("fullscreen") << Qt::WindowFullScreen;
}

void tst_QBackingStore::flush()
{
    QFETCH(Qt::WindowState, windowState);
    Window window;
    window.setGeometry(20, 20, 200, 200);
    window.setWindowState(windowState);
    window.setVisible(true);

    QTRY_VERIFY(window.isExposed());
}

void tst_QBackingStore::staticContents()
{
    const auto *integration = QGuiApplicationPrivate::platformIntegration();
    if (!integration->hasCapability(QPlatformIntegration::BackingStoreStaticContents))
        QSKIP("Platform does not support static backingstore content");

    QWindow window;
    window.create();

    const auto dpr = window.devicePixelRatio();

    QBackingStore backingStore(&window);

    QRect initialRect(0, 0, 100, 100);

    // Static contents without paint first should not crash
    backingStore.setStaticContents(initialRect);
    backingStore.resize(initialRect.size());
    QCOMPARE(backingStore.size(), initialRect.size());
    backingStore.beginPaint(QRect(0, 0, 50, 50));
    backingStore.endPaint();
    backingStore.handle()->toImage();

    {
        backingStore.setStaticContents(QRect());
        backingStore.beginPaint(initialRect);
        QPainter p(backingStore.paintDevice());
        p.fillRect(initialRect, Qt::green);
        p.end();
        backingStore.endPaint();

        QImage image = backingStore.handle()->toImage();
        if (image.isNull())
            QSKIP("Platform backingstore does not implement toImage");

        QCOMPARE(image.pixelColor(initialRect.topLeft() * dpr), Qt::green);
        QCOMPARE(image.pixelColor(initialRect.bottomLeft() * dpr), Qt::green);
        QCOMPARE(image.pixelColor(initialRect.topRight() * dpr), Qt::green);
        QCOMPARE(image.pixelColor(initialRect.bottomRight() * dpr), Qt::green);
    }

    {
        backingStore.setStaticContents(initialRect);

        QRect resizedRect(0, 0, 200, 200);
        backingStore.resize(resizedRect.size());

        QRegion repaintRegion = QRegion(resizedRect) - QRegion(initialRect);

        backingStore.beginPaint(repaintRegion);
        QPainter p(backingStore.paintDevice());
        for (auto repaintRect : repaintRegion)
            p.fillRect(repaintRect, Qt::red);
        p.end();
        backingStore.endPaint();

        QImage image = backingStore.handle()->toImage();
        if (image.isNull())
            QSKIP("Platform backingstore does not implement toImage");

        QCOMPARE(image.pixelColor(initialRect.topLeft() * dpr), Qt::green);
        QCOMPARE(image.pixelColor(initialRect.bottomLeft() * dpr), Qt::green);
        QCOMPARE(image.pixelColor(initialRect.topRight() * dpr), Qt::green);
        QCOMPARE(image.pixelColor(initialRect.bottomRight() * dpr), Qt::green);

        for (auto repaintRect : repaintRegion) {
            QCOMPARE(image.pixelColor(repaintRect.topLeft() * dpr), Qt::red);
            QCOMPARE(image.pixelColor(repaintRect.bottomLeft() * dpr), Qt::red);
            QCOMPARE(image.pixelColor(repaintRect.topRight() * dpr), Qt::red);
            QCOMPARE(image.pixelColor(repaintRect.bottomRight() * dpr), Qt::red);
        }
    }
}

void tst_QBackingStore::childWindowFlush()
{
    const auto *integration = QGuiApplicationPrivate::platformIntegration();
    if (!integration->hasCapability(QPlatformIntegration::ScreenWindowGrabbing))
        QSKIP("Platform does not support grabbing screen");
    if (!integration->hasCapability(QPlatformIntegration::ChildWindows))
        QSKIP("Platform does not support child windows");

    QWindow parent;
    parent.setGeometry(100, 100, 400, 400);
    QWindow child(&parent);
    QRect childRect(0, 0, 200, 200);
    child.setGeometry(childRect);
    child.show();
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));
    QVERIFY(QTest::qWaitForWindowExposed(&child));

    QBackingStore backingStore(&parent);

    // Prime double buffering at the initial small size.
    QRect small(0, 0, 100, 100);
    backingStore.resize(small.size());
    backingStore.beginPaint(small);
    {
        QPainter p(backingStore.paintDevice());
        p.fillRect(small, Qt::blue);
    }
    backingStore.endPaint();
    backingStore.flush(small);
    backingStore.flush(small, &child);

    static const QColor greenColor = Qt::green;

    // Grow to the full parent size, paint red with the child's
    // area green, flush the top-level (swaps buffers) and then
    // the native child.
    QRect big(QPoint(), parent.size());
    backingStore.resize(big.size());
    backingStore.beginPaint(big);
    {
        QPainter p(backingStore.paintDevice());
        p.fillRect(big, Qt::red);
        p.fillRect(childRect, greenColor);
    }
    backingStore.endPaint();
    backingStore.flush(big);
    backingStore.flush(big, &child);

    // Move the child over the red area, so that any part of the
    // child that was not flushed shows red instead of green.
    child.setPosition(200, 200);

    QImage grab;
    auto pixelAt = [&](const QPoint &point) {
        return grab.pixelColor(point * grab.devicePixelRatio());
    };

    auto grabPixelAt = [&](const QPoint &point) {
        grab = parent.screen()->grabWindow(parent.winId()).toImage();
        return pixelAt(point);
    };

    QTRY_COMPARE(grabPixelAt(QPoint(250, 250)), greenColor);

    QCOMPARE(pixelAt(QPoint(350, 250)), greenColor);
    QCOMPARE(pixelAt(QPoint(250, 350)), greenColor);
    QCOMPARE(pixelAt(QPoint(350, 350)), greenColor);
}

#include <tst_qbackingstore.moc>
QTEST_MAIN(tst_QBackingStore);
