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

#include <QMdiSubWindow>
#include <QMdiArea>

#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QPushButton>
#include <QStyle>
#include <QStyleOption>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QDesktopWidget>
#include <QDockWidget>
#include <QScrollBar>
#include <QTextEdit>
#ifndef QT_NO_OPENGL
#include <QtOpenGL>
#include <QOpenGLContext>
#endif
#include <QStyleHints>

static const Qt::WindowFlags DefaultWindowFlags
    = Qt::SubWindow | Qt::WindowSystemMenuHint
      | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint;

Q_DECLARE_METATYPE(QMdiArea::WindowOrder)
Q_DECLARE_METATYPE(QTabWidget::TabPosition)

static bool tabBetweenSubWindowsIn(QMdiArea *mdiArea, int tabCount = -1, bool reverse = false)
{
    if (!mdiArea) {
        qWarning("Null pointer to mdi area");
        return false;
    }

    QList<QMdiSubWindow *> subWindows = mdiArea->subWindowList();
    const bool walkThrough = tabCount == -1;

    if (walkThrough) {
        QMdiSubWindow *active = reverse ? subWindows.front() : subWindows.back();
        mdiArea->setActiveSubWindow(active);
        if (mdiArea->activeSubWindow() != active) {
            qWarning("Failed to set active sub window");
            return false;
        }
        tabCount = subWindows.size();
    }

    QWidget *focusWidget = qApp->focusWidget();
    if (!focusWidget) {
        qWarning("No focus widget");
        return false;
    }

    Qt::KeyboardModifiers modifiers = reverse ? Qt::ShiftModifier : Qt::NoModifier;
    Qt::Key key;
#ifdef Q_OS_MAC
    key = Qt::Key_Meta;
    modifiers |= Qt::MetaModifier;
#else
    key = Qt::Key_Control;
    modifiers |= Qt::ControlModifier;
#endif

    QTest::keyPress(focusWidget, key, modifiers);
    for (int i = 0; i < tabCount; ++i) {
        QTest::keyPress(focusWidget, reverse ? Qt::Key_Backtab : Qt::Key_Tab, modifiers);
        if (tabCount > 1)
            QTest::qWait(500);
        if (walkThrough) {
            QRubberBand *rubberBand = mdiArea->findChild<QRubberBand *>();
            if (!rubberBand) {
                qWarning("No rubber band");
                return false;
            }
            QMdiSubWindow *subWindow = subWindows.at(reverse ? subWindows.size() -1 - i : i);
            if (rubberBand->geometry() != subWindow->geometry()) {
                qWarning("Rubber band has different geometry");
                return false;
            }
        }
        qApp->processEvents();
    }
    QTest::keyRelease(focusWidget, key);

    return true;
}

static inline QTabBar::Shape tabBarShapeFrom(QTabWidget::TabShape shape, QTabWidget::TabPosition position)
{
    const bool rounded = (shape == QTabWidget::Rounded);
    if (position == QTabWidget::North)
        return rounded ? QTabBar::RoundedNorth : QTabBar::TriangularNorth;
    if (position == QTabWidget::South)
        return rounded ? QTabBar::RoundedSouth : QTabBar::TriangularSouth;
    if (position == QTabWidget::East)
        return rounded ? QTabBar::RoundedEast : QTabBar::TriangularEast;
    if (position == QTabWidget::West)
        return rounded ? QTabBar::RoundedWest : QTabBar::TriangularWest;
    return QTabBar::RoundedNorth;
}

static int cascadedDeltaY(const QMdiArea *area)
{
    // Calculate the delta (dx, dy) between two cascaded subwindows.
    const QWidget *subWindow = area->subWindowList().first();
    const QStyle *style = subWindow->style();
    QStyleOptionTitleBar options;
    options.initFrom(subWindow);
    int titleBarHeight = style->pixelMetric(QStyle::PM_TitleBarHeight, &options);
    // ### Remove this after the QMacStyle has been fixed
    if (style->inherits("QMacStyle"))
        titleBarHeight -= 4;
    const QFontMetrics fontMetrics = QFontMetrics(QApplication::font("QMdiSubWindowTitleBar"));
    return qMax(titleBarHeight - (titleBarHeight - fontMetrics.height()) / 2, 1)
        + style->pixelMetric(QStyle::PM_FocusFrameVMargin);
}

enum Arrangement {
    Tiled,
    Cascaded
};

static bool verifyArrangement(QMdiArea *mdiArea, Arrangement arrangement, const QList<int> &expectedIndices)
{
    if (!mdiArea || expectedIndices.isEmpty() || mdiArea->subWindowList().isEmpty())
        return false;

    const QList<QMdiSubWindow *> subWindows = mdiArea->subWindowList();

    switch (arrangement) {
    case Tiled:
    {
        // Calculate the number of rows and columns.
        const int n = subWindows.count();
        const int numColumns = qMax(qCeil(qSqrt(qreal(n))), 1);
        const int numRows = qMax((n % numColumns) ? (n / numColumns + 1) : (n / numColumns), 1);

        // Ensure that the geometry of all the subwindows are as expected by using
        // QWidget::childAt starting from the middle of the topleft cell and subsequently
        // adding rowWidth and rowHeight (going from left to right).
        const int columnWidth = mdiArea->viewport()->width() / numColumns;
        const int rowHeight = mdiArea->viewport()->height() / numRows;
        QPoint subWindowPos(columnWidth / 2, rowHeight / 2);
        for (int i = 0; i < numRows; ++i) {
            for (int j = 0; j < numColumns; ++j) {
                const int index = expectedIndices.at(i * numColumns + j);
                QWidget *actual = mdiArea->viewport()->childAt(subWindowPos);
                QMdiSubWindow *expected = subWindows.at(index);
                if (actual != expected && !expected->isAncestorOf(actual))
                    return false;
                subWindowPos.rx() += columnWidth;
            }
            subWindowPos.rx() = columnWidth / 2;
            subWindowPos.ry() += rowHeight;
        }
        break;
    }
    case Cascaded:
    {
        const int dy =  cascadedDeltaY(mdiArea);
        const int dx = 10;

        // Current activation/stacking order.
        const QList<QMdiSubWindow *> activationOrderList = mdiArea->subWindowList(QMdiArea::ActivationHistoryOrder);

        // Ensure that the geometry of all the subwindows are as expected by using
        // QWidget::childAt with the position of the first one and subsequently adding
        // dx and dy.
        QPoint subWindowPos(20, 5);
        foreach (int expectedIndex, expectedIndices) {
            QMdiSubWindow *expected = subWindows.at(expectedIndex);
            expected->raise();
            if (mdiArea->viewport()->childAt(subWindowPos) != expected)
                return false;
            expected->lower();
            subWindowPos.rx() += dx;
            subWindowPos.ry() += dy;
        }

        // Restore stacking order.
        foreach (QMdiSubWindow *subWindow, activationOrderList) {
            mdiArea->setActiveSubWindow(subWindow);
            qApp->processEvents();
        }
        break;
    }
    default:
        return false;
    }
    return true;
}

class tst_QMdiArea : public QObject
{
    Q_OBJECT
public:
    tst_QMdiArea();
public slots:
    void initTestCase();
    void cleanup();

protected slots:
    void activeChanged(QMdiSubWindow *child);

private slots:
    // Tests from QWorkspace
    void subWindowActivated_data();
    void subWindowActivated();
    void subWindowActivated2();
    void subWindowActivatedWithMinimize();
    void showWindows();
    void changeWindowTitle();
    void changeModified();
    void childSize();
    void fixedSize();
    // New tests
    void minimumSizeHint();
    void sizeHint();
    void setActiveSubWindow();
    void activeSubWindow();
    void currentSubWindow();
    void addAndRemoveWindows();
    void addAndRemoveWindowsWithReparenting();
    void removeSubWindow_2();
    void closeWindows();
    void activateNextAndPreviousWindow();
    void subWindowList_data();
    void subWindowList();
    void setBackground();
    void setViewport();
    void tileSubWindows();
    void cascadeAndTileSubWindows();
    void resizeMaximizedChildWindows_data();
    void resizeMaximizedChildWindows();
    void focusWidgetAfterAddSubWindow();
    void dontMaximizeSubWindowOnActivation();
    void delayedPlacement();
    void iconGeometryInMenuBar();
    void resizeTimer();
    void updateScrollBars();
    void setActivationOrder_data();
    void setActivationOrder();
    void tabBetweenSubWindows();
    void setViewMode();
    void setTabsClosable();
    void setTabsMovable();
    void setTabShape();
    void setTabPosition_data();
    void setTabPosition();
    void nativeSubWindows();
    void task_209615();
    void task_236750();

private:
    QMdiSubWindow *activeWindow;
};

tst_QMdiArea::tst_QMdiArea()
    : activeWindow(0)
{
    qRegisterMetaType<QMdiSubWindow *>();
}

void tst_QMdiArea::initTestCase()
{
#ifdef Q_OS_WINCE //disable magic for WindowsCE
    qApp->setAutoMaximizeThreshold(-1);
#endif
}

void tst_QMdiArea::cleanup()
{
    QVERIFY(QApplication::topLevelWidgets().isEmpty());
}

// Old QWorkspace tests
void tst_QMdiArea::activeChanged(QMdiSubWindow *child)
{
    activeWindow = child;
}

void tst_QMdiArea::subWindowActivated_data()
{
    // define the test elements we're going to use
    QTest::addColumn<int>("count");

    // create a first testdata instance and fill it with data
    QTest::newRow( "data0" ) << 0;
    QTest::newRow( "data1" ) << 1;
    QTest::newRow( "data2" ) << 2;
}

void tst_QMdiArea::subWindowActivated()
{
    QMainWindow mw(0) ;
    mw.menuBar();
    QMdiArea *workspace = new QMdiArea(&mw);
    workspace->setObjectName(QLatin1String("testWidget"));
    mw.setCentralWidget(workspace);
    QSignalSpy spy(workspace, SIGNAL(subWindowActivated(QMdiSubWindow*)));
    connect( workspace, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(activeChanged(QMdiSubWindow*)));
    mw.show();
    qApp->setActiveWindow(&mw);

    QFETCH( int, count );
    int i;

    for ( i = 0; i < count; ++i ) {
        QWidget *widget = new QWidget(workspace, 0);
        widget->setAttribute(Qt::WA_DeleteOnClose);
        workspace->addSubWindow(widget)->show();
        widget->show();
        qApp->processEvents();
        QVERIFY( activeWindow == workspace->activeSubWindow() );
        QCOMPARE(spy.count(), 1);
        spy.clear();
    }

    QList<QMdiSubWindow *> windows = workspace->subWindowList();
    QCOMPARE( (int)windows.count(), count );

    for ( i = 0; i < count; ++i ) {
        QMdiSubWindow *window = windows.at(i);
        window->showMinimized();
        qApp->processEvents();
        QVERIFY( activeWindow == workspace->activeSubWindow() );
        if ( i == 1 )
            QVERIFY( activeWindow == window );
    }

    for ( i = 0; i < count; ++i ) {
        QMdiSubWindow *window = windows.at(i);
        window->showNormal();
        qApp->processEvents();
        QVERIFY( window == activeWindow );
        QVERIFY( activeWindow == workspace->activeSubWindow() );
    }
    spy.clear();

    while (workspace->activeSubWindow() ) {
        workspace->activeSubWindow()->close();
        qApp->processEvents();
        QCOMPARE(activeWindow, workspace->activeSubWindow());
        QCOMPARE(spy.count(), 1);
        spy.clear();
    }

    QVERIFY(!activeWindow);
    QVERIFY(!workspace->activeSubWindow());
    QCOMPARE(workspace->subWindowList().count(), 0);

    {
        workspace->hide();
        QWidget *widget = new QWidget(workspace);
        widget->setAttribute(Qt::WA_DeleteOnClose);
        QMdiSubWindow *window = workspace->addSubWindow(widget);
        widget->show();
        QCOMPARE(spy.count(), 0);
        workspace->show();
        QCOMPARE(spy.count(), 1);
        spy.clear();
        QVERIFY( activeWindow == window );
        window->close();
        qApp->processEvents();
        QCOMPARE(spy.count(), 1);
        spy.clear();
        QVERIFY( activeWindow == 0 );
    }

    {
        workspace->hide();
        QWidget *widget = new QWidget(workspace);
        widget->setAttribute(Qt::WA_DeleteOnClose);
        QMdiSubWindow *window = workspace->addSubWindow(widget);
        widget->showMaximized();
        qApp->sendPostedEvents();
        QCOMPARE(spy.count(), 0);
        spy.clear();
        workspace->show();
        QCOMPARE(spy.count(), 1);
        spy.clear();
        QVERIFY( activeWindow == window );
        window->close();
        qApp->processEvents();
        QCOMPARE(spy.count(), 1);
        spy.clear();
        QVERIFY( activeWindow == 0 );
    }

    {
        QWidget *widget = new QWidget(workspace);
        widget->setAttribute(Qt::WA_DeleteOnClose);
        QMdiSubWindow *window = workspace->addSubWindow(widget);
        widget->showMinimized();
        QCOMPARE(spy.count(), 1);
        spy.clear();
        QVERIFY( activeWindow == window );
        QCOMPARE(workspace->activeSubWindow(), window);
        window->close();
        qApp->processEvents();
        QCOMPARE(spy.count(), 1);
        spy.clear();
        QVERIFY(!workspace->activeSubWindow());
        QVERIFY(!activeWindow);
    }
}

#ifdef Q_OS_MAC
#include <Security/AuthSession.h>
bool macHasAccessToWindowsServer()
{
    SecuritySessionId mySession;
    SessionAttributeBits sessionInfo;
    SessionGetInfo(callerSecuritySession, &mySession, &sessionInfo);
    return (sessionInfo & sessionHasGraphicAccess);
}
#endif


void tst_QMdiArea::subWindowActivated2()
{
    QMdiArea mdiArea;
    QSignalSpy spy(&mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)));
    for (int i = 0; i < 5; ++i)
        mdiArea.addSubWindow(new QWidget);
    QCOMPARE(spy.count(), 0);
    mdiArea.show();
    mdiArea.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&mdiArea));

    QTRY_COMPARE(spy.count(), 5);
    QCOMPARE(mdiArea.activeSubWindow(), mdiArea.subWindowList().back());
    spy.clear();

    // Just to make sure another widget is on top wrt. stacking order.
    // This will typically become the active window if things are broken.
    QMdiSubWindow *staysOnTopWindow = mdiArea.subWindowList().at(3);
    staysOnTopWindow->setWindowFlags(Qt::WindowStaysOnTopHint);
    mdiArea.setActiveSubWindow(staysOnTopWindow);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(mdiArea.activeSubWindow(), staysOnTopWindow);
    spy.clear();

    QMdiSubWindow *activeSubWindow = mdiArea.subWindowList().at(2);
    mdiArea.setActiveSubWindow(activeSubWindow);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(mdiArea.activeSubWindow(), activeSubWindow);
    spy.clear();

    // Check that we only emit _one_ signal and the active window
    // is unchanged after hide/show.
    mdiArea.hide();
#ifdef Q_DEAD_CODE_FROM_QT4_X11
    qt_x11_wait_for_window_manager(&mdiArea);
#endif
    QTest::qWait(100);
    QTRY_COMPARE(spy.count(), 1);
    QVERIFY(!mdiArea.activeSubWindow());
    QCOMPARE(mdiArea.currentSubWindow(), activeSubWindow);
    spy.clear();

    mdiArea.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mdiArea));
    QTRY_COMPARE(spy.count(), 1);
    QCOMPARE(mdiArea.activeSubWindow(), activeSubWindow);
    spy.clear();

    if (qGuiApp->styleHints()->showIsFullScreen())
        QSKIP("Platform is auto maximizing, so no showMinimized()");

    // Check that we only emit _one_ signal and the active window
    // is unchanged after showMinimized/showNormal.
    mdiArea.showMinimized();
#if defined (Q_OS_MAC)
    if (!macHasAccessToWindowsServer())
        QEXPECT_FAIL("", "showMinimized doesn't really minimize if you don't have access to the server", Abort);
#endif
#ifdef Q_OS_WINCE
    QSKIP("Not fixed yet. See Task 197453");
#endif
#ifdef Q_OS_MAC
    QSKIP("QTBUG-25298: This test is unstable on Mac.");
#endif
    if (!QGuiApplication::platformName().compare(QLatin1String("xcb"), Qt::CaseInsensitive))
        QSKIP("QTBUG-25298: Unstable on some X11 window managers");
    QTRY_COMPARE(spy.count(), 1);
    QVERIFY(!mdiArea.activeSubWindow());
    QCOMPARE(mdiArea.currentSubWindow(), activeSubWindow);
    spy.clear();

    // For this test, the QMdiArea widget must be active after minimizing and
    // showing it again. QMdiArea has no active sub window if it is inactive itself.
    mdiArea.showNormal();
    mdiArea.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&mdiArea));
    QTRY_COMPARE(spy.count(), 1);
    QCOMPARE(mdiArea.activeSubWindow(), activeSubWindow);
    spy.clear();
}

void tst_QMdiArea::subWindowActivatedWithMinimize()
{
    QMainWindow mw(0) ;
    mw.menuBar();
    QMdiArea *workspace = new QMdiArea(&mw);
    workspace->setObjectName(QLatin1String("testWidget"));
    mw.setCentralWidget(workspace);
    QSignalSpy spy(workspace, SIGNAL(subWindowActivated(QMdiSubWindow*)));
    connect( workspace, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(activeChanged(QMdiSubWindow*)) );
    mw.show();
    qApp->setActiveWindow(&mw);
    QWidget *widget = new QWidget(workspace);
    widget->setAttribute(Qt::WA_DeleteOnClose);
    QMdiSubWindow *window1 = workspace->addSubWindow(widget);
    QWidget *widget2 = new QWidget(workspace);
    widget2->setAttribute(Qt::WA_DeleteOnClose);
    QMdiSubWindow *window2 = workspace->addSubWindow(widget2);

    widget->showMinimized();
    QVERIFY( activeWindow == window1 );
    widget2->showMinimized();
    QVERIFY( activeWindow == window2 );

    window2->close();
    qApp->processEvents();
    QVERIFY( activeWindow == window1 );

    window1->close();
    qApp->processEvents();
    QVERIFY(!workspace->activeSubWindow());
    QVERIFY(!activeWindow);

    QVERIFY( workspace->subWindowList().count() == 0 );
}

void tst_QMdiArea::showWindows()
{
    QMdiArea *ws = new QMdiArea( 0 );

    QWidget *widget = 0;
    ws->show();

    widget = new QWidget(ws);
    widget->show();
    QVERIFY( widget->isVisible() );

    widget = new QWidget(ws);
    widget->showMaximized();
    QVERIFY( widget->isMaximized() );
    widget->showNormal();
    QVERIFY( !widget->isMaximized() );

    widget = new QWidget(ws);
    widget->showMinimized();
    QVERIFY( widget->isMinimized() );
    widget->showNormal();
    QVERIFY( !widget->isMinimized() );

    ws->hide();

    widget = new QWidget(ws);
    ws->show();
    QVERIFY( widget->isVisible() );

    ws->hide();

    widget = new QWidget(ws);
    widget->showMaximized();
    QVERIFY( widget->isMaximized() );
    ws->show();
    QVERIFY( widget->isVisible() );
    QVERIFY( widget->isMaximized() );
    ws->hide();

    widget = new QWidget(ws);
    widget->showMinimized();
    ws->show();
    QVERIFY( widget->isMinimized() );
    ws->hide();

    delete ws;
}


//#define USE_SHOW

void tst_QMdiArea::changeWindowTitle()
{
    const QString mwc = QString::fromLatin1("MainWindow's Caption");
    const QString mwc2 = QString::fromLatin1("MainWindow's New Caption");
    const QString wc = QString::fromLatin1("Widget's Caption");
    const QString wc2 = QString::fromLatin1("Widget's New Caption");

    QMainWindow *mw = new QMainWindow;
    mw->setWindowTitle( mwc );
    QMdiArea *ws = new QMdiArea( mw );
    mw->setCentralWidget( ws );
    mw->menuBar();
    mw->show();
    QVERIFY(QTest::qWaitForWindowExposed(mw));

    QWidget *widget = new QWidget( ws );
    widget->setWindowTitle( wc );
    ws->addSubWindow(widget);

    QCOMPARE( mw->windowTitle(), mwc );

#ifdef USE_SHOW
    widget->showMaximized();
#else
    widget->setWindowState(Qt::WindowMaximized);
#endif
#if !defined(Q_OS_MAC) && !defined(Q_OS_WINCE)
    QTRY_COMPARE( mw->windowTitle(), QString::fromLatin1("%1 - [%2]").arg(mwc).arg(wc) );
#endif

    mw->hide();
    qApp->processEvents();
    mw->show();
    QVERIFY(QTest::qWaitForWindowExposed(mw));

#if !defined(Q_OS_MAC) && !defined(Q_OS_WINCE)
    QTRY_COMPARE( mw->windowTitle(), QString::fromLatin1("%1 - [%2]").arg(mwc).arg(wc) );
#endif

#ifdef USE_SHOW
    widget->showNormal();
#else
    widget->setWindowState(Qt::WindowNoState);
#endif
    qApp->processEvents();
    QCOMPARE( mw->windowTitle(), mwc );

#ifdef USE_SHOW
    widget->showMaximized();
#else
    widget->setWindowState(Qt::WindowMaximized);
#endif
    qApp->processEvents();
#if !defined(Q_OS_MAC) && !defined(Q_OS_WINCE)
    QTRY_COMPARE( mw->windowTitle(), QString::fromLatin1("%1 - [%2]").arg(mwc).arg(wc) );
    widget->setWindowTitle( wc2 );
    QCOMPARE( mw->windowTitle(), QString::fromLatin1("%1 - [%2]").arg(mwc).arg(wc2) );
    mw->setWindowTitle( mwc2 );
    QCOMPARE( mw->windowTitle(), QString::fromLatin1("%1 - [%2]").arg(mwc2).arg(wc2) );
#endif

    mw->show();
    qApp->setActiveWindow(mw);

#ifdef USE_SHOW
    mw->showFullScreen();
#else
    mw->setWindowState(Qt::WindowFullScreen);
#endif

    qApp->processEvents();
#if !defined(Q_OS_MAC) && !defined(Q_OS_WINCE)
    QCOMPARE( mw->windowTitle(), QString::fromLatin1("%1 - [%2]").arg(mwc2).arg(wc2) );
#endif
#ifdef USE_SHOW
    widget->showNormal();
#else
    widget->setWindowState(Qt::WindowNoState);
#endif
    qApp->processEvents();
#if defined(Q_OS_MAC) || defined(Q_OS_WINCE)
    QCOMPARE(mw->windowTitle(), mwc);
#else
    QCOMPARE( mw->windowTitle(), mwc2 );
#endif

#ifdef USE_SHOW
    widget->showMaximized();
#else
    widget->setWindowState(Qt::WindowMaximized);
#endif
    qApp->processEvents();
#if !defined(Q_OS_MAC) && !defined(Q_OS_WINCE)
    QCOMPARE( mw->windowTitle(), QString::fromLatin1("%1 - [%2]").arg(mwc2).arg(wc2) );
#endif

#ifdef USE_SHOW
    mw->showNormal();
#else
    mw->setWindowState(Qt::WindowNoState);
#endif
    qApp->processEvents();
#ifdef USE_SHOW
    widget->showNormal();
#else
    widget->setWindowState(Qt::WindowNoState);
#endif

    delete mw;
}

void tst_QMdiArea::changeModified()
{
    const QString mwc = QString::fromLatin1("MainWindow's Caption");
    const QString wc = QString::fromLatin1("Widget's Caption[*]");

    QMainWindow *mw = new QMainWindow(0);
    mw->setWindowTitle( mwc );
    QMdiArea *ws = new QMdiArea( mw );
    mw->setCentralWidget( ws );
    mw->menuBar();
    mw->show();

    QWidget *widget = new QWidget( ws );
    widget->setWindowTitle( wc );
    ws->addSubWindow(widget);

    QCOMPARE( mw->isWindowModified(), false);
    QCOMPARE( widget->isWindowModified(), false);
    widget->setWindowState(Qt::WindowMaximized);
    QCOMPARE( mw->isWindowModified(), false);
    QCOMPARE( widget->isWindowModified(), false);

    widget->setWindowState(Qt::WindowNoState);
    QCOMPARE( mw->isWindowModified(), false);
    QCOMPARE( widget->isWindowModified(), false);

    widget->setWindowModified(true);
    QCOMPARE( mw->isWindowModified(), false);
    QCOMPARE( widget->isWindowModified(), true);
    widget->setWindowState(Qt::WindowMaximized);
#if !defined(Q_OS_MAC) && !defined(Q_OS_WINCE)
    QCOMPARE( mw->isWindowModified(), true);
#endif
    QCOMPARE( widget->isWindowModified(), true);

    widget->setWindowState(Qt::WindowNoState);
    QCOMPARE( mw->isWindowModified(), false);
    QCOMPARE( widget->isWindowModified(), true);

    widget->setWindowState(Qt::WindowMaximized);
#if !defined(Q_OS_MAC) && !defined(Q_OS_WINCE)
    QCOMPARE( mw->isWindowModified(), true);
#endif
    QCOMPARE( widget->isWindowModified(), true);

    widget->setWindowModified(false);
    QCOMPARE( mw->isWindowModified(), false);
    QCOMPARE( widget->isWindowModified(), false);

    widget->setWindowModified(true);
#if !defined(Q_OS_MAC) && !defined(Q_OS_WINCE)
    QCOMPARE( mw->isWindowModified(), true);
#endif
    QCOMPARE( widget->isWindowModified(), true);

    widget->setWindowState(Qt::WindowNoState);
    QCOMPARE( mw->isWindowModified(), false);
    QCOMPARE( widget->isWindowModified(), true);

    delete mw;
}

class MyChild : public QWidget
{
public:
    MyChild(QWidget *parent = 0) : QWidget(parent) {}
    QSize sizeHint() const { return QSize(234, 123); }
};

void tst_QMdiArea::childSize()
{
    QMdiArea ws;

    MyChild *child = new MyChild(&ws);
    child->show();
    QCOMPARE(child->size(), child->sizeHint());
    delete child;

    child = new MyChild(&ws);
    child->setFixedSize(200, 200);
    child->show();
    QCOMPARE(child->size(), child->minimumSize());
    delete child;

    child = new MyChild(&ws);
    child->resize(150, 150);
    child->show();
    QCOMPARE(child->size(), QSize(150,150));
    delete child;
}

void tst_QMdiArea::fixedSize()
{
    QMdiArea *ws = new QMdiArea;
    int i;

    ws->resize(500, 500);
//     ws->show();

    QSize fixed(300, 300);
    for (i = 0; i < 4; ++i) {
        QWidget *child = new QWidget(ws);
        child->setFixedSize(fixed);
        child->show();
    }

    QList<QMdiSubWindow *> windows = ws->subWindowList();
    for (i = 0; i < (int)windows.count(); ++i) {
        QMdiSubWindow *child = windows.at(i);
        QCOMPARE(child->size(), fixed);
    }

    ws->cascadeSubWindows();
    ws->resize(800, 800);
    for (i = 0; i < (int)windows.count(); ++i) {
        QMdiSubWindow *child = windows.at(i);
        QCOMPARE(child->size(), fixed);
    }
    ws->resize(500, 500);

    ws->tileSubWindows();
    ws->resize(800, 800);
    for (i = 0; i < (int)windows.count(); ++i) {
        QMdiSubWindow *child = windows.at(i);
        QCOMPARE(child->size(), fixed);
    }
    ws->resize(500, 500);

    for (i = 0; i < (int)windows.count(); ++i) {
        QMdiSubWindow *child = windows.at(i);
        delete child;
    }

    delete ws;
}

class LargeWidget : public QWidget
{
public:
    LargeWidget(QWidget *parent = 0) : QWidget(parent) {}
    QSize sizeHint() const { return QSize(1280, 1024); }
    QSize minimumSizeHint() const { return QSize(300, 300); }
};

// New tests
void tst_QMdiArea::minimumSizeHint()
{
    QMdiArea workspace;
    workspace.show();
    QSize expectedSize(workspace.style()->pixelMetric(QStyle::PM_MDIMinimizedWidth),
                       workspace.style()->pixelMetric(QStyle::PM_TitleBarHeight));
    qApp->processEvents();
    QAbstractScrollArea dummyScrollArea;
    dummyScrollArea.setFrameStyle(QFrame::NoFrame);
    expectedSize = expectedSize.expandedTo(dummyScrollArea.minimumSizeHint());
    QCOMPARE(workspace.minimumSizeHint(), expectedSize.expandedTo(qApp->globalStrut()));

    QWidget *window = workspace.addSubWindow(new QWidget);
    qApp->processEvents();
    window->show();
    QCOMPARE(workspace.minimumSizeHint(), expectedSize.expandedTo(window->minimumSizeHint()));

    QMdiSubWindow *subWindow = workspace.addSubWindow(new LargeWidget);
    subWindow->show();
    QCOMPARE(workspace.minimumSizeHint(), expectedSize.expandedTo(subWindow->minimumSizeHint()));

    workspace.setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    workspace.setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    QCOMPARE(workspace.minimumSizeHint(), expectedSize);
}

void tst_QMdiArea::sizeHint()
{
    QMdiArea workspace;
    workspace.show();
    QSize desktopSize = QApplication::desktop()->size();
    QSize expectedSize(desktopSize.width() * 2/3, desktopSize.height() * 2/3);
    QCOMPARE(workspace.sizeHint(), expectedSize.expandedTo(qApp->globalStrut()));

    QWidget *window = workspace.addSubWindow(new QWidget);
    qApp->processEvents();
    window->show();
    QCOMPARE(workspace.sizeHint(), expectedSize.expandedTo(window->sizeHint()));

    QMdiSubWindow *nested = workspace.addSubWindow(new QMdiArea);
    expectedSize = QSize(desktopSize.width() * 2/6, desktopSize.height() * 2/6);
    QCOMPARE(nested->widget()->sizeHint(), expectedSize);
}

void tst_QMdiArea::setActiveSubWindow()
{
    QMdiArea workspace;
    workspace.show();

    QSignalSpy spy(&workspace, SIGNAL(subWindowActivated(QMdiSubWindow*)));
    connect(&workspace, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(activeChanged(QMdiSubWindow*)));
    qApp->setActiveWindow(&workspace);

    // Activate hidden windows
    const int windowCount = 10;
    QMdiSubWindow *windows[windowCount];
    for (int i = 0; i < windowCount; ++i) {
        windows[i] = qobject_cast<QMdiSubWindow *>(workspace.addSubWindow(new QWidget));
        qApp->processEvents();
        QVERIFY(windows[i]->isHidden());
        workspace.setActiveSubWindow(windows[i]);
    }
    QCOMPARE(spy.count(), 0);
    QVERIFY(!activeWindow);
    spy.clear();

    // Activate visible windows
    for (int i = 0; i < windowCount; ++i) {
        windows[i]->show();
        QVERIFY(!windows[i]->isHidden());
        workspace.setActiveSubWindow(windows[i]);
        qApp->processEvents();
        QCOMPARE(spy.count(), 1);
        QCOMPARE(activeWindow, windows[i]);
        spy.clear();
    }

    // Deactivate active window
    QCOMPARE(workspace.activeSubWindow(), windows[windowCount - 1]);
    workspace.setActiveSubWindow(0);
    QCOMPARE(spy.count(), 1);
    QVERIFY(!activeWindow);
    QVERIFY(!workspace.activeSubWindow());

    // Activate widget which is not child of any window inside workspace
    QMdiSubWindow fakeWindow;
    QTest::ignoreMessage(QtWarningMsg, "QMdiArea::setActiveSubWindow: window is not inside workspace");
    workspace.setActiveSubWindow(&fakeWindow);

}

void tst_QMdiArea::activeSubWindow()
{
    QMainWindow mainWindow;

    QMdiArea *mdiArea = new QMdiArea;
    QLineEdit *subWindowLineEdit = new QLineEdit;
    QMdiSubWindow *subWindow = mdiArea->addSubWindow(subWindowLineEdit);
    mainWindow.setCentralWidget(mdiArea);

    QDockWidget *dockWidget = new QDockWidget(QLatin1String("Dock Widget"), &mainWindow);
    dockWidget->setAllowedAreas(Qt::LeftDockWidgetArea);
    QLineEdit *dockWidgetLineEdit = new QLineEdit;
    dockWidget->setWidget(dockWidgetLineEdit);
    mainWindow.addDockWidget(Qt::LeftDockWidgetArea, dockWidget);

    mainWindow.show();
    qApp->setActiveWindow(&mainWindow);
    QVERIFY(QTest::qWaitForWindowActive(&mainWindow));
    QCOMPARE(mdiArea->activeSubWindow(), subWindow);
    QCOMPARE(qApp->focusWidget(), (QWidget *)subWindowLineEdit);

    dockWidgetLineEdit->setFocus();
    QCOMPARE(qApp->focusWidget(), (QWidget *)dockWidgetLineEdit);
    QCOMPARE(mdiArea->activeSubWindow(), subWindow);

    QEvent deactivateEvent(QEvent::WindowDeactivate);
    qApp->sendEvent(subWindow, &deactivateEvent);
    QVERIFY(!mdiArea->activeSubWindow());
    QCOMPARE(qApp->focusWidget(), (QWidget *)dockWidgetLineEdit);

    QEvent activateEvent(QEvent::WindowActivate);
    qApp->sendEvent(subWindow, &activateEvent);
    QCOMPARE(mdiArea->activeSubWindow(), subWindow);
    QCOMPARE(qApp->focusWidget(), (QWidget *)subWindowLineEdit);

    QLineEdit dummyTopLevel;
    dummyTopLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dummyTopLevel));

    qApp->setActiveWindow(&dummyTopLevel);
    QCOMPARE(mdiArea->activeSubWindow(), subWindow);

    qApp->setActiveWindow(&mainWindow);
    QCOMPARE(mdiArea->activeSubWindow(), subWindow);

    //task 202657
    dockWidgetLineEdit->setFocus();
    qApp->setActiveWindow(&mainWindow);
    QVERIFY(dockWidgetLineEdit->hasFocus());
}

void tst_QMdiArea::currentSubWindow()
{
    QMdiArea mdiArea;
    mdiArea.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mdiArea));

    for (int i = 0; i < 5; ++i)
        mdiArea.addSubWindow(new QLineEdit)->show();

    qApp->setActiveWindow(&mdiArea);
    QCOMPARE(qApp->activeWindow(), (QWidget *)&mdiArea);

    // Check that the last added window is the active and the current.
    QMdiSubWindow *active = mdiArea.activeSubWindow();
    QVERIFY(active);
    QCOMPARE(mdiArea.subWindowList().back(), active);
    QCOMPARE(mdiArea.currentSubWindow(), active);

    QLineEdit dummyTopLevel;
    dummyTopLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dummyTopLevel));

    // Move focus to another top-level and check that we still
    // have an active window.
    qApp->setActiveWindow(&dummyTopLevel);
    QCOMPARE(qApp->activeWindow(), (QWidget *)&dummyTopLevel);
    QVERIFY(mdiArea.activeSubWindow());

    delete active;
    active = 0;

    // We just deleted the current sub-window -> current should then
    // be the next in list (which in this case is the first sub-window).
    QVERIFY(mdiArea.currentSubWindow());
    QCOMPARE(mdiArea.currentSubWindow(), mdiArea.subWindowList().front());

    // Activate mdi area and check that active == current.
    qApp->setActiveWindow(&mdiArea);
    active = mdiArea.activeSubWindow();
    QVERIFY(active);
    QCOMPARE(mdiArea.activeSubWindow(), mdiArea.subWindowList().front());

    active->hide();
    QCOMPARE(mdiArea.activeSubWindow(), active);
    QCOMPARE(mdiArea.currentSubWindow(), active);

    qApp->setActiveWindow(&dummyTopLevel);
    QVERIFY(mdiArea.activeSubWindow());
    QCOMPARE(mdiArea.currentSubWindow(), active);

    qApp->setActiveWindow(&mdiArea);
    active->show();
    QCOMPARE(mdiArea.activeSubWindow(), active);

    mdiArea.setActiveSubWindow(0);
    QVERIFY(!mdiArea.activeSubWindow());
    QVERIFY(!mdiArea.currentSubWindow());

    mdiArea.setActiveSubWindow(active);
    QCOMPARE(mdiArea.activeSubWindow(), active);
    QEvent windowDeactivate(QEvent::WindowDeactivate);
    qApp->sendEvent(active, &windowDeactivate);
    QVERIFY(!mdiArea.activeSubWindow());
    QVERIFY(!mdiArea.currentSubWindow());

    QEvent windowActivate(QEvent::WindowActivate);
    qApp->sendEvent(active, &windowActivate);
    QVERIFY(mdiArea.activeSubWindow());
    QVERIFY(mdiArea.currentSubWindow());
}

void tst_QMdiArea::addAndRemoveWindows()
{
    QWidget topLevel;
    QMdiArea workspace(&topLevel);
    workspace.resize(800, 600);
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));

    { // addSubWindow with large widget
    QCOMPARE(workspace.subWindowList().count(), 0);
    QWidget *window = workspace.addSubWindow(new LargeWidget);
    QVERIFY(window);
    qApp->processEvents();
    QCOMPARE(workspace.subWindowList().count(), 1);
    QCOMPARE(window->windowFlags(), DefaultWindowFlags);
    QCOMPARE(window->size(), workspace.viewport()->size());
    }

    { // addSubWindow, minimumSize set.
    QMdiSubWindow *window = new QMdiSubWindow;
    window->setMinimumSize(900, 900);
    workspace.addSubWindow(window);
    QVERIFY(window);
    qApp->processEvents();
    QCOMPARE(workspace.subWindowList().count(), 2);
    QCOMPARE(window->windowFlags(), DefaultWindowFlags);
    QCOMPARE(window->size(), window->minimumSize());
    }

    { // addSubWindow, resized
    QMdiSubWindow *window = new QMdiSubWindow;
    window->setWidget(new QWidget);
    window->resize(1500, 1500);
    workspace.addSubWindow(window);
    QVERIFY(window);
    qApp->processEvents();
    QCOMPARE(workspace.subWindowList().count(), 3);
    QCOMPARE(window->windowFlags(), DefaultWindowFlags);
    QCOMPARE(window->size(), QSize(1500, 1500));
    }

    { // addSubWindow with 0 pointer
    QTest::ignoreMessage(QtWarningMsg, "QMdiArea::addSubWindow: null pointer to widget");
    QWidget *window = workspace.addSubWindow(0);
    QVERIFY(!window);
    QCOMPARE(workspace.subWindowList().count(), 3);
    }

    { // addChildWindow
    QMdiSubWindow *window = new QMdiSubWindow;
    workspace.addSubWindow(window);
    qApp->processEvents();
    QCOMPARE(window->windowFlags(), DefaultWindowFlags);
    window->setWidget(new QWidget);
    QCOMPARE(workspace.subWindowList().count(), 4);
    QTest::ignoreMessage(QtWarningMsg, "QMdiArea::addSubWindow: window is already added");
    workspace.addSubWindow(window);
    }

    { // addChildWindow with 0 pointer
    QTest::ignoreMessage(QtWarningMsg, "QMdiArea::addSubWindow: null pointer to widget");
    workspace.addSubWindow(0);
    QCOMPARE(workspace.subWindowList().count(), 4);
    }

    // removeSubWindow
    foreach (QWidget *window, workspace.subWindowList()) {
        workspace.removeSubWindow(window);
        delete window;
    }
    QCOMPARE(workspace.subWindowList().count(), 0);

    // removeSubWindow with 0 pointer
    QTest::ignoreMessage(QtWarningMsg, "QMdiArea::removeSubWindow: null pointer to widget");
    workspace.removeSubWindow(0);

    workspace.addSubWindow(new QPushButton(QLatin1String("Dummy to make workspace non-empty")));
    qApp->processEvents();
    QCOMPARE(workspace.subWindowList().count(), 1);

    // removeSubWindow with window not inside workspace
    QTest::ignoreMessage(QtWarningMsg,"QMdiArea::removeSubWindow: window is not inside workspace");
    QMdiSubWindow *fakeWindow = new QMdiSubWindow;
    workspace.removeSubWindow(fakeWindow);
    delete fakeWindow;

    // Check that newly added windows don't occupy maximized windows'
    // restore space.
    workspace.closeAllSubWindows();
    workspace.setOption(QMdiArea::DontMaximizeSubWindowOnActivation);
    workspace.show();
    QMdiSubWindow *window1 = workspace.addSubWindow(new QWidget);
    window1->show();
    const QRect window1RestoreGeometry = window1->geometry();
    QCOMPARE(window1RestoreGeometry.topLeft(), QPoint(0, 0));

    window1->showMinimized();

    // Occupy space.
    QMdiSubWindow *window2 = workspace.addSubWindow(new QWidget);
    window2->show();
    const QRect window2RestoreGeometry = window2->geometry();
    QCOMPARE(window2RestoreGeometry.topLeft(), QPoint(0, 0));

    window2->showMaximized();

    // Don't occupy space.
    QMdiSubWindow *window3 = workspace.addSubWindow(new QWidget);
    window3->show();
    QCOMPARE(window3->geometry().topLeft(), QPoint(window2RestoreGeometry.right() + 1, 0));
}

void tst_QMdiArea::addAndRemoveWindowsWithReparenting()
{
    QMdiArea workspace;
    QMdiSubWindow window(&workspace);
    QCOMPARE(window.windowFlags(), DefaultWindowFlags);

    // 0 because the window list contains widgets and not actual
    // windows. Silly, but that's the behavior.
    QCOMPARE(workspace.subWindowList().count(), 0);
    window.setWidget(new QWidget);
    qApp->processEvents();

    QCOMPARE(workspace.subWindowList().count(), 1);
    window.setParent(0); // Will also reset window flags
    QCOMPARE(workspace.subWindowList().count(), 0);
    window.setParent(&workspace);
    QCOMPARE(workspace.subWindowList().count(), 1);
    QCOMPARE(window.windowFlags(), DefaultWindowFlags);

    QTest::ignoreMessage(QtWarningMsg, "QMdiArea::addSubWindow: window is already added");
    workspace.addSubWindow(&window);
    QCOMPARE(workspace.subWindowList().count(), 1);
}

class MySubWindow : public QMdiSubWindow
{
public:
    using QObject::receivers;
};

static int numberOfConnectedSignals(MySubWindow *subWindow)
{
    if (!subWindow)
        return 0;

    int numConnectedSignals = 0;
    for (int i = 0; i < subWindow->metaObject()->methodCount(); ++i) {
        QMetaMethod method = subWindow->metaObject()->method(i);
        if (method.methodType() == QMetaMethod::Signal) {
            QString signature(QLatin1String("2"));
            signature += QLatin1String(method.methodSignature().constData());
            numConnectedSignals += subWindow->receivers(signature.toLatin1());
        }
    }
    return numConnectedSignals;
}

void tst_QMdiArea::removeSubWindow_2()
{
    QMdiArea mdiArea;
    MySubWindow *subWindow = new MySubWindow;
    QCOMPARE(numberOfConnectedSignals(subWindow), 0);

    // Connected to aboutToActivate() and windowStateChanged().
    mdiArea.addSubWindow(subWindow);
    QVERIFY(numberOfConnectedSignals(subWindow) >= 2);

    // Ensure we disconnect from all signals.
    mdiArea.removeSubWindow(subWindow);
    QCOMPARE(numberOfConnectedSignals(subWindow), 0);

    mdiArea.addSubWindow(subWindow);
    QVERIFY(numberOfConnectedSignals(subWindow) >= 2);
    subWindow->setParent(0);
    QScopedPointer<MySubWindow> subWindowGuard(subWindow);
    QCOMPARE(numberOfConnectedSignals(subWindow), 0);
}

void tst_QMdiArea::closeWindows()
{
    QMdiArea workspace;
    workspace.show();
    qApp->setActiveWindow(&workspace);

    // Close widget
    QWidget *widget = new QWidget;
    QMdiSubWindow *subWindow = workspace.addSubWindow(widget);
    qApp->processEvents();
    QCOMPARE(workspace.subWindowList().count(), 1);
    subWindow->close();
    QCOMPARE(workspace.subWindowList().count(), 0);

    // Close window
    QWidget *window = workspace.addSubWindow(new QWidget);
    qApp->processEvents();
    QCOMPARE(workspace.subWindowList().count(), 1);
    window->close();
    qApp->processEvents();
    QCOMPARE(workspace.subWindowList().count(), 0);

    const int windowCount = 10;

    // Close active window
    for (int i = 0; i < windowCount; ++i)
        workspace.addSubWindow(new QWidget)->show();
    qApp->processEvents();
    QCOMPARE(workspace.subWindowList().count(), windowCount);
    int activeSubWindowCount = 0;
    while (workspace.activeSubWindow()) {
        workspace.activeSubWindow()->close();
        qApp->processEvents();
        ++activeSubWindowCount;
    }
    QCOMPARE(activeSubWindowCount, windowCount);
    QCOMPARE(workspace.subWindowList().count(), 0);

    // Close all windows
    for (int i = 0; i < windowCount; ++i)
        workspace.addSubWindow(new QWidget)->show();
    qApp->processEvents();
    QCOMPARE(workspace.subWindowList().count(), windowCount);
    QSignalSpy spy(&workspace, SIGNAL(subWindowActivated(QMdiSubWindow*)));
    connect(&workspace, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(activeChanged(QMdiSubWindow*)));
    workspace.closeAllSubWindows();
    qApp->processEvents();
    QCOMPARE(workspace.subWindowList().count(), 0);
    QCOMPARE(spy.count(), 1);
    QVERIFY(!activeWindow);
}

void tst_QMdiArea::activateNextAndPreviousWindow()
{
    QMdiArea workspace;
    workspace.show();
    qApp->setActiveWindow(&workspace);

    const int windowCount = 10;
    QMdiSubWindow *windows[windowCount];
    for (int i = 0; i < windowCount; ++i) {
        windows[i] = qobject_cast<QMdiSubWindow *>(workspace.addSubWindow(new QWidget));
        windows[i]->show();
        qApp->processEvents();
    }

    QSignalSpy spy(&workspace, SIGNAL(subWindowActivated(QMdiSubWindow*)));
    connect(&workspace, SIGNAL(subWindowActivated(QMdiSubWindow*)), this, SLOT(activeChanged(QMdiSubWindow*)));

    // activateNextSubWindow
    for (int i = 0; i < windowCount; ++i) {
        workspace.activateNextSubWindow();
        qApp->processEvents();
        QCOMPARE(workspace.activeSubWindow(), windows[i]);
        QCOMPARE(spy.count(), 1);
        spy.clear();
    }
    QVERIFY(activeWindow);
    QCOMPARE(workspace.activeSubWindow(), windows[windowCount - 1]);
    QCOMPARE(workspace.activeSubWindow(), activeWindow);

    // activatePreviousSubWindow
    for (int i = windowCount - 2; i >= 0; --i) {
        workspace.activatePreviousSubWindow();
        qApp->processEvents();
        QCOMPARE(workspace.activeSubWindow(), windows[i]);
        QCOMPARE(spy.count(), 1);
        spy.clear();
        if (i % 2 == 0)
            windows[i]->hide(); // 10, 8, 6, 4, 2, 0
    }
    QVERIFY(activeWindow);
    QCOMPARE(workspace.activeSubWindow(), windows[0]);
    QCOMPARE(workspace.activeSubWindow(), activeWindow);

    // activateNextSubWindow with every 2nd window hidden
    for (int i = 0; i < windowCount / 2; ++i) {
        workspace.activateNextSubWindow(); // 1, 3, 5, 7, 9
        QCOMPARE(spy.count(), 1);
        spy.clear();
    }
    QCOMPARE(workspace.activeSubWindow(), windows[windowCount - 1]);

    // activatePreviousSubWindow with every 2nd window hidden
    for (int i = 0; i < windowCount / 2; ++i) {
        workspace.activatePreviousSubWindow(); // 7, 5, 3, 1, 9
        QCOMPARE(spy.count(), 1);
        spy.clear();
    }
    QCOMPARE(workspace.activeSubWindow(), windows[windowCount - 1]);

    workspace.setActiveSubWindow(0);
    QVERIFY(!activeWindow);
}

void tst_QMdiArea::subWindowList_data()
{
    QTest::addColumn<QMdiArea::WindowOrder>("windowOrder");
    QTest::addColumn<int>("windowCount");
    QTest::addColumn<int>("activeSubWindow");
    QTest::addColumn<int>("staysOnTop1");
    QTest::addColumn<int>("staysOnTop2");

    QTest::newRow("CreationOrder") << QMdiArea::CreationOrder << 10 << 4 << 8 << 5;
    QTest::newRow("StackingOrder") << QMdiArea::StackingOrder << 10 << 6 << 3 << 9;
    QTest::newRow("ActivationHistoryOrder") << QMdiArea::ActivationHistoryOrder << 10 << 7 << 2 << 1;
}
void tst_QMdiArea::subWindowList()
{
    QFETCH(QMdiArea::WindowOrder, windowOrder);
    QFETCH(int, windowCount);
    QFETCH(int, activeSubWindow);
    QFETCH(int, staysOnTop1);
    QFETCH(int, staysOnTop2);

    QMdiArea workspace;
    workspace.show();
    qApp->setActiveWindow(&workspace);

    QList<QMdiSubWindow *> activationOrder;
    QVector<QMdiSubWindow *> windows;
    for (int i = 0; i < windowCount; ++i) {
        windows.append(qobject_cast<QMdiSubWindow *>(workspace.addSubWindow(new QWidget)));
        windows[i]->show();
        activationOrder.append(windows[i]);
    }

    {
    QList<QMdiSubWindow *> widgets = workspace.subWindowList(windowOrder);
    QCOMPARE(widgets.count(), windowCount);
    for (int i = 0; i < widgets.count(); ++i)
        QCOMPARE(widgets.at(i), windows[i]);
    }

    windows[staysOnTop1]->setWindowFlags(windows[staysOnTop1]->windowFlags() | Qt::WindowStaysOnTopHint);
    workspace.setActiveSubWindow(windows[activeSubWindow]);
    qApp->processEvents();
    QCOMPARE(workspace.activeSubWindow(), windows[activeSubWindow]);
    activationOrder.move(activationOrder.indexOf(windows[activeSubWindow]), windowCount - 1);

    QList<QMdiSubWindow *> subWindows = workspace.subWindowList(windowOrder);
    if (windowOrder == QMdiArea::CreationOrder) {
        QCOMPARE(subWindows.at(activeSubWindow), windows[activeSubWindow]);
        QCOMPARE(subWindows.at(staysOnTop1), windows[staysOnTop1]);
        for (int i = 0; i < windowCount; ++i)
            QCOMPARE(subWindows.at(i), windows[i]);
        return;
    }

    if (windowOrder == QMdiArea::StackingOrder) {
        QCOMPARE(subWindows.at(subWindows.count() - 1), windows[staysOnTop1]);
        QCOMPARE(subWindows.at(subWindows.count() - 2), windows[activeSubWindow]);
        QCOMPARE(subWindows.count(), windowCount);
    } else { // ActivationHistoryOrder
        QCOMPARE(subWindows, activationOrder);
    }

    windows[staysOnTop2]->setWindowFlags(windows[staysOnTop2]->windowFlags() | Qt::WindowStaysOnTopHint);
    workspace.setActiveSubWindow(windows[staysOnTop2]);
    qApp->processEvents();
    QCOMPARE(workspace.activeSubWindow(), windows[staysOnTop2]);
    activationOrder.move(activationOrder.indexOf(windows[staysOnTop2]), windowCount - 1);

    workspace.setActiveSubWindow(windows[activeSubWindow]);
    qApp->processEvents();
    QCOMPARE(workspace.activeSubWindow(), windows[activeSubWindow]);
    activationOrder.move(activationOrder.indexOf(windows[activeSubWindow]), windowCount - 1);

    QList<QMdiSubWindow *> widgets = workspace.subWindowList(windowOrder);
    QCOMPARE(widgets.count(), windowCount);
    if (windowOrder == QMdiArea::StackingOrder) {
        QCOMPARE(widgets.at(widgets.count() - 1), windows[staysOnTop2]);
        QCOMPARE(widgets.at(widgets.count() - 2), windows[staysOnTop1]);
        QCOMPARE(widgets.at(widgets.count() - 3), windows[activeSubWindow]);
    } else { // ActivationHistory
        QCOMPARE(widgets, activationOrder);
    }

    windows[activeSubWindow]->raise();
    windows[staysOnTop2]->lower();

    widgets = workspace.subWindowList(windowOrder);
    if (windowOrder == QMdiArea::StackingOrder) {
        QCOMPARE(widgets.at(widgets.count() - 1), windows[activeSubWindow]);
        QCOMPARE(widgets.at(widgets.count() - 2), windows[staysOnTop1]);
        QCOMPARE(widgets.at(0), windows[staysOnTop2]);
    } else { // ActivationHistoryOrder
        QCOMPARE(widgets, activationOrder);
    }

    windows[activeSubWindow]->stackUnder(windows[staysOnTop1]);
    windows[staysOnTop2]->raise();

    widgets = workspace.subWindowList(windowOrder);
    if (windowOrder == QMdiArea::StackingOrder) {
        QCOMPARE(widgets.at(widgets.count() - 1), windows[staysOnTop2]);
        QCOMPARE(widgets.at(widgets.count() - 2), windows[staysOnTop1]);
        QCOMPARE(widgets.at(widgets.count() - 3), windows[activeSubWindow]);
    } else { // ActivationHistoryOrder
        QCOMPARE(widgets, activationOrder);
    }

    workspace.setActiveSubWindow(windows[staysOnTop1]);
    activationOrder.move(activationOrder.indexOf(windows[staysOnTop1]), windowCount - 1);

    widgets = workspace.subWindowList(windowOrder);
    if (windowOrder == QMdiArea::StackingOrder) {
        QCOMPARE(widgets.at(widgets.count() - 1), windows[staysOnTop1]);
        QCOMPARE(widgets.at(widgets.count() - 2), windows[staysOnTop2]);
        QCOMPARE(widgets.at(widgets.count() - 3), windows[activeSubWindow]);
    } else { // ActivationHistoryOrder
        QCOMPARE(widgets, activationOrder);
    }
}

void tst_QMdiArea::setBackground()
{
    QMdiArea workspace;
    QCOMPARE(workspace.background(), workspace.palette().brush(QPalette::Dark));
    workspace.setBackground(QBrush(Qt::green));
    QCOMPARE(workspace.background(), QBrush(Qt::green));
}

void tst_QMdiArea::setViewport()
{
    QMdiArea workspace;
    workspace.show();

    QWidget *firstViewport = workspace.viewport();
    QVERIFY(firstViewport);

    const int windowCount = 10;
    for (int i = 0; i < windowCount; ++i) {
        QMdiSubWindow *window = workspace.addSubWindow(new QWidget);
        window->show();
        if (i % 2 == 0) {
            window->showMinimized();
            QVERIFY(window->isMinimized());
        } else {
            window->showMaximized();
            QVERIFY(window->isMaximized());
        }
    }

    qApp->processEvents();
    QList<QMdiSubWindow *> windowsBeforeViewportChange = workspace.subWindowList();
    QCOMPARE(windowsBeforeViewportChange.count(), windowCount);

    workspace.setViewport(new QWidget);
    qApp->processEvents();
    QVERIFY(workspace.viewport() != firstViewport);

    QList<QMdiSubWindow *> windowsAfterViewportChange = workspace.subWindowList();
    QCOMPARE(windowsAfterViewportChange.count(), windowCount);
    QCOMPARE(windowsAfterViewportChange, windowsBeforeViewportChange);

    //    for (int i = 0; i < windowCount; ++i) {
    //        QMdiSubWindow *window = windowsAfterViewportChange.at(i);
    //        if (i % 2 == 0)
    //            QVERIFY(!window->isMinimized());
    //else
    //    QVERIFY(!window->isMaximized());
    //    }

    QTest::ignoreMessage(QtWarningMsg, "QMdiArea: Deleting the view port is undefined, "
                                       "use setViewport instead.");
    delete workspace.viewport();
    qApp->processEvents();

    QCOMPARE(workspace.subWindowList().count(), 0);
    QVERIFY(!workspace.activeSubWindow());
}

void tst_QMdiArea::tileSubWindows()
{
    QMdiArea workspace;
    workspace.resize(600,480);
    workspace.show();
    QVERIFY(QTest::qWaitForWindowExposed(&workspace));

    const int windowCount = 10;
    for (int i = 0; i < windowCount; ++i) {
        QMdiSubWindow *subWindow = workspace.addSubWindow(new QWidget);
        subWindow->setMinimumSize(50, 30);
        subWindow->show();
    }
    workspace.tileSubWindows();
    workspace.setActiveSubWindow(0);
    QCOMPARE(workspace.viewport()->childrenRect(), workspace.viewport()->rect());

    QList<QMdiSubWindow *> windows = workspace.subWindowList();
    for (int i = 0; i < windowCount; ++i) {
        QMdiSubWindow *window = windows.at(i);
        for (int j = 0; j < windowCount; ++j) {
            if (i == j)
                continue;
            QVERIFY(!window->geometry().intersects(windows.at(j)->geometry()));
        }
    }

    // Keep the views tiled through any subsequent resize events.
    for (int i = 0; i < 5; ++i) {
        workspace.resize(workspace.size() - QSize(10, 10));
        qApp->processEvents();
    }
    workspace.setActiveSubWindow(0);
#ifndef Q_OS_WINCE //See Task 197453 ToDo
    QCOMPARE(workspace.viewport()->childrenRect(), workspace.viewport()->rect());
#endif

    QMdiSubWindow *window = windows.at(0);

    // Change the geometry of one of the children and verify
    // that the views are not tiled anymore.
    window->move(window->x() + 1, window->y());
    workspace.resize(workspace.size() - QSize(10, 10));
    workspace.setActiveSubWindow(0);
    QVERIFY(workspace.viewport()->childrenRect() != workspace.viewport()->rect());
    qApp->processEvents();

    // Re-tile.
    workspace.tileSubWindows();
    workspace.setActiveSubWindow(0);
    QCOMPARE(workspace.viewport()->childrenRect(), workspace.viewport()->rect());

    // Close one of the children and verify that the views
    // are not tiled anymore.
    window->close();
    workspace.resize(workspace.size() - QSize(10, 10));
    workspace.setActiveSubWindow(0);
    QVERIFY(workspace.viewport()->childrenRect() != workspace.viewport()->rect());
    qApp->processEvents();

    // Re-tile.
    workspace.tileSubWindows();
    workspace.setActiveSubWindow(0);
    QCOMPARE(workspace.viewport()->childrenRect(), workspace.viewport()->rect());

    window = windows.at(1);

    // Maximize one of the children and verify that the views
    // are not tiled anymore.
    workspace.tileSubWindows();
    window->showMaximized();
    workspace.resize(workspace.size() - QSize(10, 10));
    workspace.setActiveSubWindow(0);
    QVERIFY(workspace.viewport()->childrenRect() != workspace.viewport()->rect());
    qApp->processEvents();

    // Re-tile.
    workspace.tileSubWindows();
    workspace.setActiveSubWindow(0);
    QCOMPARE(workspace.viewport()->childrenRect(), workspace.viewport()->rect());

    // Minimize one of the children and verify that the views
    // are not tiled anymore.
    workspace.tileSubWindows();
    window->showMinimized();
    workspace.resize(workspace.size() - QSize(10, 10));
    workspace.setActiveSubWindow(0);
    QVERIFY(workspace.viewport()->childrenRect() != workspace.viewport()->rect());
    qApp->processEvents();

    // Re-tile.
    workspace.tileSubWindows();
    workspace.setActiveSubWindow(0);
    QCOMPARE(workspace.viewport()->childrenRect(), workspace.viewport()->rect());

    // Active/deactivate windows and verify that the views are tiled.
    workspace.setActiveSubWindow(windows.at(5));
    workspace.resize(workspace.size() - QSize(10, 10));
    workspace.setActiveSubWindow(0);
    QTest::qWait(250); // delayed re-arrange of minimized windows
    QTRY_COMPARE(workspace.viewport()->childrenRect(), workspace.viewport()->rect());

    // Add another window and verify that the views are not tiled anymore.
    workspace.addSubWindow(new QPushButton(QLatin1String("I'd like to mess up tiled views")))->show();
    workspace.resize(workspace.size() - QSize(10, 10));
    workspace.setActiveSubWindow(0);
    QVERIFY(workspace.viewport()->childrenRect() != workspace.viewport()->rect());

    // Re-tile.
    workspace.tileSubWindows();
    workspace.setActiveSubWindow(0);
    QCOMPARE(workspace.viewport()->childrenRect(), workspace.viewport()->rect());

    // Cascade and verify that the views are not tiled anymore.
    workspace.cascadeSubWindows();
    workspace.resize(workspace.size() - QSize(10, 10));
    workspace.setActiveSubWindow(0);
    QVERIFY(workspace.viewport()->childrenRect() != workspace.viewport()->rect());

    // Make sure the active window is placed in top left corner regardless
    // of whether we have any windows with staysOnTopHint or not.
    windows.at(3)->setWindowFlags(windows.at(3)->windowFlags() | Qt::WindowStaysOnTopHint);
    QMdiSubWindow *activeSubWindow = windows.at(6);
    workspace.setActiveSubWindow(activeSubWindow);
    QCOMPARE(workspace.activeSubWindow(), activeSubWindow);
    workspace.tileSubWindows();
    QCOMPARE(activeSubWindow->geometry().topLeft(), QPoint(0, 0));

    // Verify that we try to resize the area such that all sub-windows are visible.
    // It's important that tiled windows are NOT overlapping.
    workspace.resize(350, 150);
    qApp->processEvents();
    QTRY_COMPARE(workspace.size(), QSize(350, 150));

    const QSize minSize(600, 130);
    foreach (QMdiSubWindow *subWindow, workspace.subWindowList())
        subWindow->setMinimumSize(minSize);

    QCOMPARE(workspace.size(), QSize(350, 150));

    // Prevent scrollbars from messing up the expected viewport calculation below
    workspace.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    workspace.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    workspace.tileSubWindows();
    // The sub-windows are now tiled like this:
    // | win 1 || win 2 || win 3 |
    // +-------++-------++-------+
    // +-------++-------++-------+
    // | win 4 || win 5 || win 6 |
    // +-------++-------++-------+
    // +-------++-------++-------+
    // | win 7 || win 8 || win 9 |
    workspace.setActiveSubWindow(0);
    int frameWidth = 0;
    if (workspace.style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents, 0, &workspace))
        frameWidth = workspace.style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    const int spacing = 2 * frameWidth + 2;
    const QSize expectedViewportSize(3 * minSize.width() + spacing, 3 * minSize.height() + spacing);
#ifdef Q_OS_WINCE
    QSKIP("Not fixed yet! See task 197453");
#endif
    QTRY_COMPARE(workspace.viewport()->rect().size(), expectedViewportSize);

    // Restore original scrollbar behavior for test below
    workspace.setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    workspace.setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Not enough space for all sub-windows to be visible -> provide scroll bars.
    workspace.resize(160, 150);
    qApp->processEvents();
    QTRY_COMPARE(workspace.size(), QSize(160, 150));

    // Horizontal scroll bar.
    QScrollBar *hBar = workspace.horizontalScrollBar();
    QCOMPARE(workspace.horizontalScrollBarPolicy(), Qt::ScrollBarAsNeeded);
    QTRY_VERIFY(hBar->isVisible());
    QCOMPARE(hBar->value(), 0);
    QCOMPARE(hBar->minimum(), 0);

    // Vertical scroll bar.
    QScrollBar *vBar = workspace.verticalScrollBar();
    QCOMPARE(workspace.verticalScrollBarPolicy(), Qt::ScrollBarAsNeeded);
    QVERIFY(vBar->isVisible());
    QCOMPARE(vBar->value(), 0);
    QCOMPARE(vBar->minimum(), 0);

    workspace.tileSubWindows();
    QVERIFY(QTest::qWaitForWindowExposed(&workspace));
    qApp->processEvents();

    QTRY_VERIFY(workspace.size() != QSize(150, 150));
    QTRY_VERIFY(!vBar->isVisible());
    QTRY_VERIFY(!hBar->isVisible());
}

void tst_QMdiArea::cascadeAndTileSubWindows()
{
    QMdiArea workspace;
    workspace.resize(400, 400);
    workspace.show();
    QVERIFY(QTest::qWaitForWindowExposed(&workspace));

    const int windowCount = 10;
    QList<QMdiSubWindow *> windows;
    for (int i = 0; i < windowCount; ++i) {
        QMdiSubWindow *window = workspace.addSubWindow(new MyChild);
        if (i % 3 == 0) {
            window->showMinimized();
            QVERIFY(window->isMinimized());
        } else {
            window->showMaximized();
            QVERIFY(window->isMaximized());
        }
        windows.append(window);
    }

    // cascadeSubWindows
    qApp->processEvents();
    workspace.cascadeSubWindows();
    qApp->processEvents();

    // Check dy between two cascaded windows
    const int dy = cascadedDeltaY(&workspace);
#ifdef Q_OS_MAC
    QEXPECT_FAIL("", "QTBUG-25298", Abort);
#endif
    QCOMPARE(windows.at(2)->geometry().top() - windows.at(1)->geometry().top(), dy);

    for (int i = 0; i < windows.count(); ++i) {
        QMdiSubWindow *window = windows.at(i);
        if (i % 3 == 0) {
            QVERIFY(window->isMinimized());
        } else {
            QVERIFY(!window->isMaximized());
            QCOMPARE(window->size(), window->sizeHint());
            window->showMaximized();
            QVERIFY(window->isMaximized());
        }
    }
}

void tst_QMdiArea::resizeMaximizedChildWindows_data()
{
    QTest::addColumn<int>("startSize");
    QTest::addColumn<int>("increment");
    QTest::addColumn<int>("windowCount");

    QTest::newRow("multiple children") << 400 << 20 << 10;
}

void tst_QMdiArea::resizeMaximizedChildWindows()
{
    QFETCH(int, startSize);
    QFETCH(int, increment);
    QFETCH(int, windowCount);

    QWidget topLevel;
    QMdiArea workspace(&topLevel);
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    workspace.resize(startSize, startSize);
    workspace.setOption(QMdiArea::DontMaximizeSubWindowOnActivation);
    QSize workspaceSize = workspace.size();
    QVERIFY(workspaceSize.isValid());
    QCOMPARE(workspaceSize, QSize(startSize, startSize));

    QList<QMdiSubWindow *> windows;
    for (int i = 0; i < windowCount; ++i) {
        QMdiSubWindow *window = workspace.addSubWindow(new QWidget);
        windows.append(window);
        qApp->processEvents();
        window->showMaximized();
        QTest::qWait(100);
        QVERIFY(window->isMaximized());
        QSize windowSize = window->size();
        QVERIFY(windowSize.isValid());
        QCOMPARE(window->rect(), workspace.contentsRect());

        workspace.resize(workspaceSize + QSize(increment, increment));
        QTest::qWait(100);
        qApp->processEvents();
        QTRY_COMPARE(workspace.size(), workspaceSize + QSize(increment, increment));
        QTRY_COMPARE(window->size(), windowSize + QSize(increment, increment));
        workspaceSize = workspace.size();
    }

    int newSize = startSize + increment * windowCount;
    QCOMPARE(workspaceSize, QSize(newSize, newSize));
    foreach (QWidget *window, windows)
        QCOMPARE(window->rect(), workspace.contentsRect());
}

// QWidget::setParent clears focusWidget so make sure
// we restore it after QMdiArea::addSubWindow.
void tst_QMdiArea::focusWidgetAfterAddSubWindow()
{
    QWidget *view = new QWidget;
    view->setLayout(new QVBoxLayout);

    QLineEdit *lineEdit1 = new QLineEdit;
    QLineEdit *lineEdit2 = new QLineEdit;
    view->layout()->addWidget(lineEdit1);
    view->layout()->addWidget(lineEdit2);

    lineEdit2->setFocus();
    QCOMPARE(view->focusWidget(), static_cast<QWidget *>(lineEdit2));

    QMdiArea mdiArea;
    mdiArea.addSubWindow(view);
    QCOMPARE(view->focusWidget(), static_cast<QWidget *>(lineEdit2));

    mdiArea.show();
    view->show();
    qApp->setActiveWindow(&mdiArea);
    QCOMPARE(qApp->focusWidget(), static_cast<QWidget *>(lineEdit2));
}

void tst_QMdiArea::dontMaximizeSubWindowOnActivation()
{
    QMdiArea mdiArea;
    mdiArea.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mdiArea));
    qApp->setActiveWindow(&mdiArea);

    // Add one maximized window.
    mdiArea.addSubWindow(new QWidget)->showMaximized();
    QVERIFY(mdiArea.activeSubWindow());
    QVERIFY(mdiArea.activeSubWindow()->isMaximized());

    // Add few more windows and verify that they are maximized.
    for (int i = 0; i < 5; ++i) {
        QMdiSubWindow *window = mdiArea.addSubWindow(new QWidget);
        window->show();
#if defined Q_OS_QNX
        QEXPECT_FAIL("", "QTBUG-38231", Abort);
#endif
        QVERIFY(window->isMaximized());
        qApp->processEvents();
    }

    // Verify that activated windows still are maximized on activation.
    QList<QMdiSubWindow *> subWindows = mdiArea.subWindowList();
    for (int i = 0; i < subWindows.count(); ++i) {
        mdiArea.activateNextSubWindow();
        QMdiSubWindow *window = subWindows.at(i);
        QCOMPARE(mdiArea.activeSubWindow(), window);
        QVERIFY(window->isMaximized());
        qApp->processEvents();
    }

    // Restore active window and verify that other windows aren't
    // maximized on activation.
    mdiArea.activeSubWindow()->showNormal();
    for (int i = 0; i < subWindows.count(); ++i) {
        mdiArea.activateNextSubWindow();
        QMdiSubWindow *window = subWindows.at(i);
        QCOMPARE(mdiArea.activeSubWindow(), window);
        QVERIFY(!window->isMaximized());
        qApp->processEvents();
    }

    // Enable 'DontMaximizedSubWindowOnActivation' and maximize the active window.
    mdiArea.setOption(QMdiArea::DontMaximizeSubWindowOnActivation);
    mdiArea.activeSubWindow()->showMaximized();
    int indexOfMaximized = subWindows.indexOf(mdiArea.activeSubWindow());

    // Verify that windows are not maximized on activation.
    for (int i = 0; i < subWindows.count(); ++i) {
        mdiArea.activateNextSubWindow();
        QMdiSubWindow *window = subWindows.at(i);
        QCOMPARE(mdiArea.activeSubWindow(), window);
        if (indexOfMaximized != i)
            QVERIFY(!window->isMaximized());
        qApp->processEvents();
    }
    QVERIFY(mdiArea.activeSubWindow()->isMaximized());

    // Minimize all windows.
    foreach (QMdiSubWindow *window, subWindows) {
        window->showMinimized();
        QVERIFY(window->isMinimized());
        qApp->processEvents();
    }

    // Disable 'DontMaximizedSubWindowOnActivation' and maximize the active window.
    mdiArea.setOption(QMdiArea::DontMaximizeSubWindowOnActivation, false);
    mdiArea.activeSubWindow()->showMaximized();

    // Verify that minimized windows are maximized on activation.
    for (int i = 0; i < subWindows.count(); ++i) {
        mdiArea.activateNextSubWindow();
        QMdiSubWindow *window = subWindows.at(i);
        QCOMPARE(mdiArea.activeSubWindow(), window);
        QVERIFY(window->isMaximized());
        qApp->processEvents();
    }

    // Verify that activated windows are maximized after closing
    // the active window
    for (int i = 0; i < subWindows.count(); ++i) {
        QVERIFY(mdiArea.activeSubWindow());
        QVERIFY(mdiArea.activeSubWindow()->isMaximized());
        mdiArea.activeSubWindow()->close();
        qApp->processEvents();
    }

    QVERIFY(!mdiArea.activeSubWindow());
    QCOMPARE(mdiArea.subWindowList().size(), 0);

    // Verify that new windows are not maximized.
    mdiArea.addSubWindow(new QWidget)->show();
    QVERIFY(mdiArea.activeSubWindow());
    QVERIFY(!mdiArea.activeSubWindow()->isMaximized());
}

void tst_QMdiArea::delayedPlacement()
{
    QMdiArea mdiArea;

    QMdiSubWindow *window1 = mdiArea.addSubWindow(new QWidget);
    QCOMPARE(window1->geometry().topLeft(), QPoint(0, 0));

    QMdiSubWindow *window2 = mdiArea.addSubWindow(new QWidget);
    QCOMPARE(window2->geometry().topLeft(), QPoint(0, 0));

    QMdiSubWindow *window3 = mdiArea.addSubWindow(new QWidget);
    QCOMPARE(window3->geometry().topLeft(), QPoint(0, 0));

    mdiArea.resize(window3->minimumSizeHint().width() * 3, 400);
    mdiArea.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mdiArea));

    QCOMPARE(window1->geometry().topLeft(), QPoint(0, 0));
    QCOMPARE(window2->geometry().topLeft(), window1->geometry().topRight() + QPoint(1, 0));
    QCOMPARE(window3->geometry().topLeft(), window2->geometry().topRight() + QPoint(1, 0));
}

void tst_QMdiArea::iconGeometryInMenuBar()
{
#if !defined (Q_OS_MAC) && !defined(Q_OS_WINCE)
    QMainWindow mainWindow;
    QMenuBar *menuBar = mainWindow.menuBar();
    QMdiArea *mdiArea = new QMdiArea;
    QMdiSubWindow *subWindow = mdiArea->addSubWindow(new QWidget);
    mainWindow.setCentralWidget(mdiArea);
    mainWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mainWindow));

    subWindow->showMaximized();
    QVERIFY(subWindow->isMaximized());

    QWidget *leftCornerWidget = menuBar->cornerWidget(Qt::TopLeftCorner);
    QVERIFY(leftCornerWidget);
    int topMargin = (menuBar->height() - leftCornerWidget->height()) / 2;
    int leftMargin = qApp->style()->pixelMetric(QStyle::PM_MenuBarHMargin)
                   + qApp->style()->pixelMetric(QStyle::PM_MenuBarPanelWidth);
    QPoint pos(leftMargin, topMargin);
    QRect geometry = QStyle::visualRect(qApp->layoutDirection(), menuBar->rect(),
                                        QRect(pos, leftCornerWidget->size()));
    QCOMPARE(leftCornerWidget->geometry(), geometry);
#endif
}

class EventSpy : public QObject
{
public:
    EventSpy(QObject *object, QEvent::Type event)
        : eventToSpy(event), _count(0)
    {
        if (object)
            object->installEventFilter(this);
    }

    int count() const { return _count; }
    void clear() { _count = 0; }

protected:
    bool eventFilter(QObject *object, QEvent *event)
    {
        if (event->type() == eventToSpy)
            ++_count;
        return  QObject::eventFilter(object, event);
    }

private:
    QEvent::Type eventToSpy;
    int _count;
};

void tst_QMdiArea::resizeTimer()
{
    QMdiArea mdiArea;
    QMdiSubWindow *subWindow = mdiArea.addSubWindow(new QWidget);
    mdiArea.show();
    QVERIFY(QTest::qWaitForWindowActive(&mdiArea));

#ifndef Q_OS_WINCE
    int time = 250;
#else
    int time = 1000;
#endif

    EventSpy timerEventSpy(subWindow, QEvent::Timer);
    QCOMPARE(timerEventSpy.count(), 0);

    mdiArea.tileSubWindows();
    QTest::qWait(time); // Wait for timer events to occur.
    QCOMPARE(timerEventSpy.count(), 1);
    timerEventSpy.clear();

    mdiArea.resize(mdiArea.size() + QSize(2, 2));
    QTest::qWait(time); // Wait for timer events to occur.
    QCOMPARE(timerEventSpy.count(), 1);
    timerEventSpy.clear();

    // Check that timers are killed.
    QTest::qWait(time); // Wait for timer events to occur.
    QCOMPARE(timerEventSpy.count(), 0);
}

void tst_QMdiArea::updateScrollBars()
{
    QMdiArea mdiArea;
    mdiArea.setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mdiArea.setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QMdiSubWindow *subWindow1 = mdiArea.addSubWindow(new QWidget);
    QMdiSubWindow *subWindow2 = mdiArea.addSubWindow(new QWidget);

    mdiArea.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mdiArea));
    qApp->processEvents();

    QScrollBar *hbar = mdiArea.horizontalScrollBar();
    QVERIFY(hbar);
    QVERIFY(hbar->style()->styleHint(QStyle::SH_ScrollBar_Transient) || !hbar->isVisible());

    QScrollBar *vbar = mdiArea.verticalScrollBar();
    QVERIFY(vbar);
    QVERIFY(vbar->style()->styleHint(QStyle::SH_ScrollBar_Transient) || !vbar->isVisible());

    // Move sub-window 2 away.
    subWindow2->move(10000, 10000);
    qApp->processEvents();
    QVERIFY(hbar->style()->styleHint(QStyle::SH_ScrollBar_Transient) || hbar->isVisible());
    QVERIFY(vbar->style()->styleHint(QStyle::SH_ScrollBar_Transient) || vbar->isVisible());

    for (int i = 0; i < 2; ++i) {
    // Maximize sub-window 1 and make sure we don't have any scroll bars.
    subWindow1->showMaximized();
    qApp->processEvents();
    QVERIFY(subWindow1->isMaximized());
    QVERIFY(hbar->style()->styleHint(QStyle::SH_ScrollBar_Transient) || !hbar->isVisible());
    QVERIFY(vbar->style()->styleHint(QStyle::SH_ScrollBar_Transient) || !vbar->isVisible());

    // We still shouldn't get any scroll bars.
    mdiArea.resize(mdiArea.size() - QSize(20, 20));
    QVERIFY(QTest::qWaitForWindowExposed(&mdiArea));
    qApp->processEvents();
    QVERIFY(subWindow1->isMaximized());
    QVERIFY(hbar->style()->styleHint(QStyle::SH_ScrollBar_Transient) || !hbar->isVisible());
    QVERIFY(vbar->style()->styleHint(QStyle::SH_ScrollBar_Transient) || !vbar->isVisible());

    // Restore sub-window 1 and make sure we have scroll bars again.
    subWindow1->showNormal();
    qApp->processEvents();
    QVERIFY(!subWindow1->isMaximized());
    QVERIFY(hbar->style()->styleHint(QStyle::SH_ScrollBar_Transient) || hbar->isVisible());
    QVERIFY(vbar->style()->styleHint(QStyle::SH_ScrollBar_Transient) || vbar->isVisible());
        if (i == 0) {
            // Now, do the same when the viewport is scrolled.
            hbar->setValue(1000);
            vbar->setValue(1000);
        }
    }
}

void tst_QMdiArea::setActivationOrder_data()
{
    QTest::addColumn<QMdiArea::WindowOrder>("activationOrder");
    QTest::addColumn<int>("subWindowCount");
    QTest::addColumn<int>("staysOnTopIndex");
    QTest::addColumn<int>("firstActiveIndex");
    QTest::addColumn<QList<int> >("expectedActivationIndices");
    // The order of expectedCascadeIndices:
    // window 1 -> (index 0)
    //   window 2 -> (index 1)
    //     window 3 -> (index 2)
    // ....
    QTest::addColumn<QList<int> >("expectedCascadeIndices");

    // The order of expectedTileIndices (the same as reading a book LTR).
    // +--------------------+--------------------+--------------------+
    // | window 1 (index 0) | window 2 (index 1) | window 3 (index 2) |
    // |                    +--------------------+--------------------+
    // |          (index 3) | window 4 (index 4) | window 5 (index 5) |
    // +--------------------------------------------------------------+
    QTest::addColumn<QList<int> >("expectedTileIndices");

    QList<int> list;
    QList<int> list2;
    QList<int> list3;

    list << 2 << 1 << 0 << 1 << 2 << 3 << 4;
    list2 << 0 << 1 << 2 << 3 << 4;
    list3 << 1 << 4 << 3 << 1 << 2 << 0;
    QTest::newRow("CreationOrder") << QMdiArea::CreationOrder << 5 << 3 << 1 << list << list2 << list3;

    list = QList<int>();
    list << 3 << 1 << 4 << 3 << 1 << 2 << 0;
    list2 = QList<int>();
    list2 << 0 << 2 << 4 << 1 << 3;
    list3 = QList<int>();
    list3 << 1 << 3 << 4 << 1 << 2 << 0;
    QTest::newRow("StackingOrder") << QMdiArea::StackingOrder << 5 << 3 << 1 << list << list2 << list3;

    list = QList<int>();
    list << 0 << 1 << 0 << 1 << 4 << 3 << 2;
    list2 = QList<int>();
    list2 << 0 << 2 << 3 << 4 << 1;
    list3 = QList<int>();
    list3 << 1 << 4 << 3 << 1 << 2 << 0;
    QTest::newRow("ActivationHistoryOrder") << QMdiArea::ActivationHistoryOrder << 5 << 3 << 1 << list << list2 << list3;
}

void tst_QMdiArea::setActivationOrder()
{
    QFETCH(QMdiArea::WindowOrder, activationOrder);
    QFETCH(int, subWindowCount);
    QFETCH(int, staysOnTopIndex);
    QFETCH(int, firstActiveIndex);
    QFETCH(QList<int>, expectedActivationIndices);
    QFETCH(QList<int>, expectedCascadeIndices);
    QFETCH(QList<int>, expectedTileIndices);

    // Default order.
    QMdiArea mdiArea;
    QCOMPARE(mdiArea.activationOrder(), QMdiArea::CreationOrder);

    // New order.
    mdiArea.setActivationOrder(activationOrder);
    QCOMPARE(mdiArea.activationOrder(), activationOrder);

    QList<QMdiSubWindow *> subWindows;
    for (int i = 0; i < subWindowCount; ++i)
        subWindows << mdiArea.addSubWindow(new QPushButton(tr("%1").arg(i)));
    QCOMPARE(mdiArea.subWindowList(activationOrder), subWindows);

    mdiArea.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mdiArea));

    for (int i = 0; i < subWindows.count(); ++i) {
        mdiArea.activateNextSubWindow();
        QCOMPARE(mdiArea.activeSubWindow(), subWindows.at(i));
        qApp->processEvents();
    }

    QMdiSubWindow *staysOnTop = subWindows.at(staysOnTopIndex);
    staysOnTop->setWindowFlags(staysOnTop->windowFlags() | Qt::WindowStaysOnTopHint);
    staysOnTop->raise();

    mdiArea.setActiveSubWindow(subWindows.at(firstActiveIndex));
    QCOMPARE(mdiArea.activeSubWindow(), subWindows.at(firstActiveIndex));

    // Verify the actual arrangement/geometry.
    mdiArea.tileSubWindows();
    QTest::qWait(100);
    QVERIFY(verifyArrangement(&mdiArea, Tiled, expectedTileIndices));

    mdiArea.cascadeSubWindows();
#ifdef Q_OS_MAC
    QEXPECT_FAIL("", "QTBUG-25298", Abort);
#endif
    QVERIFY(verifyArrangement(&mdiArea, Cascaded, expectedCascadeIndices));
    QTest::qWait(100);

    mdiArea.activateNextSubWindow();
    QCOMPARE(mdiArea.activeSubWindow(), subWindows.at(expectedActivationIndices.takeFirst()));

    mdiArea.activatePreviousSubWindow();
    QCOMPARE(mdiArea.activeSubWindow(), subWindows.at(expectedActivationIndices.takeFirst()));

    mdiArea.activatePreviousSubWindow();
    QCOMPARE(mdiArea.activeSubWindow(), subWindows.at(expectedActivationIndices.takeFirst()));

    for (int i = 0; i < subWindowCount; ++i) {
        mdiArea.closeActiveSubWindow();
        qApp->processEvents();
        if (i == subWindowCount - 1) { // Last window closed.
            QVERIFY(!mdiArea.activeSubWindow());
            break;
        }
        QVERIFY(mdiArea.activeSubWindow());
        QCOMPARE(mdiArea.activeSubWindow(), subWindows.at(expectedActivationIndices.takeFirst()));
    }

    QVERIFY(mdiArea.subWindowList(activationOrder).isEmpty());
    QVERIFY(expectedActivationIndices.isEmpty());
}

void tst_QMdiArea::tabBetweenSubWindows()
{
    QMdiArea mdiArea;
    QList<QMdiSubWindow *> subWindows;
    for (int i = 0; i < 5; ++i)
        subWindows << mdiArea.addSubWindow(new QLineEdit);

    mdiArea.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mdiArea));

    qApp->setActiveWindow(&mdiArea);
    QWidget *focusWidget = subWindows.back()->widget();
    QCOMPARE(qApp->focusWidget(), focusWidget);

    QSignalSpy spy(&mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)));
    QCOMPARE(spy.count(), 0);

    // Walk through the entire list of sub windows.
#ifdef Q_OS_MAC
    QEXPECT_FAIL("", "QTBUG-25298", Abort);
#endif
    QVERIFY(tabBetweenSubWindowsIn(&mdiArea));
    QCOMPARE(mdiArea.activeSubWindow(), subWindows.back());
    QCOMPARE(spy.count(), 0);

    mdiArea.setActiveSubWindow(subWindows.front());
    QCOMPARE(mdiArea.activeSubWindow(), subWindows.front());
    spy.clear();

    // Walk through the entire list of sub windows in the opposite direction (Ctrl-Shift-Tab).
    QVERIFY(tabBetweenSubWindowsIn(&mdiArea, -1, true));
    QCOMPARE(mdiArea.activeSubWindow(), subWindows.front());
    QCOMPARE(spy.count(), 0);

    // Ctrl-Tab-Tab-Tab
    QVERIFY(tabBetweenSubWindowsIn(&mdiArea, 3));
    QCOMPARE(mdiArea.activeSubWindow(), subWindows.at(3));
    QCOMPARE(spy.count(), 1);

    mdiArea.setActiveSubWindow(subWindows.at(1));
    QCOMPARE(mdiArea.activeSubWindow(), subWindows.at(1));
    spy.clear();

    // Quick switch (Ctrl-Tab once) -> switch back to the previously active sub-window.
    QVERIFY(tabBetweenSubWindowsIn(&mdiArea, 1));
    QCOMPARE(mdiArea.activeSubWindow(), subWindows.at(3));
    QCOMPARE(spy.count(), 1);
}

void tst_QMdiArea::setViewMode()
{
    QMdiArea mdiArea;

    QPixmap iconPixmap(16, 16);
    iconPixmap.fill(Qt::red);
    for (int i = 0; i < 5; ++i) {
        QMdiSubWindow *subWindow = mdiArea.addSubWindow(new QWidget);
        subWindow->setWindowTitle(QString(QLatin1String("Title %1")).arg(i));
        subWindow->setWindowIcon(iconPixmap);
    }

    mdiArea.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mdiArea));

    QMdiSubWindow *activeSubWindow = mdiArea.activeSubWindow();
    QList<QMdiSubWindow *> subWindows = mdiArea.subWindowList();

    // Default.
    QVERIFY(!activeSubWindow->isMaximized());
    QTabBar *tabBar = mdiArea.findChild<QTabBar *>();
    QVERIFY(!tabBar);
    QCOMPARE(mdiArea.viewMode(), QMdiArea::SubWindowView);

    // Tabbed view.
    mdiArea.setViewMode(QMdiArea::TabbedView);
    QCOMPARE(mdiArea.viewMode(), QMdiArea::TabbedView);
    tabBar = mdiArea.findChild<QTabBar *>();
    QVERIFY(tabBar);
    QVERIFY(tabBar->isVisible());

    QCOMPARE(tabBar->count(), subWindows.count());
    QVERIFY(activeSubWindow->isMaximized());
    QCOMPARE(tabBar->currentIndex(), subWindows.indexOf(activeSubWindow));

    // Check that tabIcon and tabText are set properly.
    for (int i = 0; i < subWindows.size(); ++i) {
        QMdiSubWindow *subWindow = subWindows.at(i);
        QCOMPARE(tabBar->tabText(i), subWindow->windowTitle());
        QCOMPARE(tabBar->tabIcon(i), subWindow->windowIcon());
    }

    // Check that tabText and tabIcon are updated.
    activeSubWindow->setWindowTitle(QLatin1String("Dude, I want another window title"));
    QCOMPARE(tabBar->tabText(tabBar->currentIndex()), activeSubWindow->windowTitle());
    iconPixmap.fill(Qt::green);
    activeSubWindow->setWindowIcon(iconPixmap);
    QCOMPARE(tabBar->tabIcon(tabBar->currentIndex()), activeSubWindow->windowIcon());

    // If there's an empty window title, tabText should return "(Untitled)" (as in firefox).
    activeSubWindow->setWindowTitle(QString());
    QCOMPARE(tabBar->tabText(tabBar->currentIndex()), QLatin1String("(Untitled)"));

    // If there's no window icon, tabIcon should return ... an empty icon :)
    activeSubWindow->setWindowIcon(QIcon());
    QCOMPARE(tabBar->tabIcon(tabBar->currentIndex()), QIcon());

    // Check that the current tab changes when activating another sub-window.
    for (int i = 0; i < subWindows.size(); ++i) {
        mdiArea.activateNextSubWindow();
        activeSubWindow = mdiArea.activeSubWindow();
        QCOMPARE(tabBar->currentIndex(), subWindows.indexOf(activeSubWindow));
    }

    activeSubWindow = mdiArea.activeSubWindow();
    const int tabIndex = tabBar->currentIndex();

    // The current tab should not change when the sub-window is hidden.
    activeSubWindow->hide();
    QCOMPARE(tabBar->currentIndex(), tabIndex);
    activeSubWindow->show();
    QCOMPARE(tabBar->currentIndex(), tabIndex);

    // Disable the tab when the sub-window is hidden and another sub-window is activated.
    activeSubWindow->hide();
    mdiArea.activateNextSubWindow();
    QVERIFY(tabBar->currentIndex() != tabIndex);
    QVERIFY(!tabBar->isTabEnabled(tabIndex));

    // Enable it again.
    activeSubWindow->show();
    QCOMPARE(tabBar->currentIndex(), tabIndex);
    QVERIFY(tabBar->isTabEnabled(tabIndex));

    // Remove sub-windows and make sure the tab is removed.
    foreach (QMdiSubWindow *subWindow, subWindows) {
        if (subWindow != activeSubWindow) {
            mdiArea.removeSubWindow(subWindow);
            delete subWindow;
        }
    }
    subWindows.clear();
    QCOMPARE(tabBar->count(), 1);

    // Go back to default (QMdiArea::SubWindowView).
    mdiArea.setViewMode(QMdiArea::SubWindowView);
    QVERIFY(!activeSubWindow->isMaximized());
    tabBar = mdiArea.findChild<QTabBar *>();
    QVERIFY(!tabBar);
    QCOMPARE(mdiArea.viewMode(), QMdiArea::SubWindowView);
}

void tst_QMdiArea::setTabsClosable()
{
    QMdiArea mdiArea;
    mdiArea.addSubWindow(new QWidget);

    // test default
    QCOMPARE(mdiArea.tabsClosable(), false);

    // change value before tab bar exists
    QTabBar *tabBar = mdiArea.findChild<QTabBar *>();
    QVERIFY(!tabBar);
    mdiArea.setTabsClosable(true);
    QCOMPARE(mdiArea.tabsClosable(), true);

    // force tab bar creation
    mdiArea.setViewMode(QMdiArea::TabbedView);
    tabBar = mdiArea.findChild<QTabBar *>();
    QVERIFY(tabBar);

    // value must've been propagated
    QCOMPARE(tabBar->tabsClosable(), true);

    // change value when tab bar exists
    mdiArea.setTabsClosable(false);
    QCOMPARE(mdiArea.tabsClosable(), false);
    QCOMPARE(tabBar->tabsClosable(), false);
}

void tst_QMdiArea::setTabsMovable()
{
    QMdiArea mdiArea;
    QMdiSubWindow *subWindow1 = mdiArea.addSubWindow(new QWidget);
    QMdiSubWindow *subWindow2 = mdiArea.addSubWindow(new QWidget);
    QMdiSubWindow *subWindow3 = mdiArea.addSubWindow(new QWidget);

    // test default
    QCOMPARE(mdiArea.tabsMovable(), false);

    // change value before tab bar exists
    QTabBar *tabBar = mdiArea.findChild<QTabBar *>();
    QVERIFY(!tabBar);
    mdiArea.setTabsMovable(true);
    QCOMPARE(mdiArea.tabsMovable(), true);

    // force tab bar creation
    mdiArea.setViewMode(QMdiArea::TabbedView);
    tabBar = mdiArea.findChild<QTabBar *>();
    QVERIFY(tabBar);

    // value must've been propagated
    QCOMPARE(tabBar->isMovable(), true);

    // test tab moving
    QList<QMdiSubWindow *> subWindows;
    subWindows << subWindow1 << subWindow2 << subWindow3;
    QCOMPARE(mdiArea.subWindowList(QMdiArea::CreationOrder), subWindows);
    tabBar->moveTab(1, 2); // 1,3,2
    subWindows.clear();
    subWindows << subWindow1 << subWindow3 << subWindow2;
    QCOMPARE(mdiArea.subWindowList(QMdiArea::CreationOrder), subWindows);
    tabBar->moveTab(0, 2); // 3,2,1
    subWindows.clear();
    subWindows << subWindow3 << subWindow2 << subWindow1;
    QCOMPARE(mdiArea.subWindowList(QMdiArea::CreationOrder), subWindows);

    // change value when tab bar exists
    mdiArea.setTabsMovable(false);
    QCOMPARE(mdiArea.tabsMovable(), false);
    QCOMPARE(tabBar->isMovable(), false);
}

void tst_QMdiArea::setTabShape()
{
    QMdiArea mdiArea;
    mdiArea.addSubWindow(new QWidget);
    mdiArea.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mdiArea));

    // Default.
    QCOMPARE(mdiArea.tabShape(), QTabWidget::Rounded);

    // Triangular.
    mdiArea.setTabShape(QTabWidget::Triangular);
    QCOMPARE(mdiArea.tabShape(), QTabWidget::Triangular);

    mdiArea.setViewMode(QMdiArea::TabbedView);

    QTabBar *tabBar = mdiArea.findChild<QTabBar *>();
    QVERIFY(tabBar);
    QCOMPARE(tabBar->shape(), QTabBar::TriangularNorth);

    // Back to default (Rounded).
    mdiArea.setTabShape(QTabWidget::Rounded);
    QCOMPARE(mdiArea.tabShape(), QTabWidget::Rounded);
    QCOMPARE(tabBar->shape(), QTabBar::RoundedNorth);
}

void tst_QMdiArea::setTabPosition_data()
{
    QTest::addColumn<QTabWidget::TabPosition>("tabPosition");
    QTest::addColumn<bool>("hasLeftMargin");
    QTest::addColumn<bool>("hasTopMargin");
    QTest::addColumn<bool>("hasRightMargin");
    QTest::addColumn<bool>("hasBottomMargin");

    QTest::newRow("North") << QTabWidget::North << false << true << false << false;
    QTest::newRow("South") << QTabWidget::South << false << false << false << true;
    QTest::newRow("East") << QTabWidget::East << false << false << true << false;
    QTest::newRow("West") << QTabWidget::West << true << false << false << false;
}

void tst_QMdiArea::setTabPosition()
{
    QFETCH(QTabWidget::TabPosition, tabPosition);
    QFETCH(bool, hasLeftMargin);
    QFETCH(bool, hasTopMargin);
    QFETCH(bool, hasRightMargin);
    QFETCH(bool, hasBottomMargin);

    QMdiArea mdiArea;
    mdiArea.addSubWindow(new QWidget);
    mdiArea.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mdiArea));

    // Make sure there are no margins.
    mdiArea.setContentsMargins(0, 0, 0, 0);

    // Default.
    QCOMPARE(mdiArea.tabPosition(), QTabWidget::North);
    mdiArea.setViewMode(QMdiArea::TabbedView);
    QTabBar *tabBar = mdiArea.findChild<QTabBar *>();
    QVERIFY(tabBar);
    QCOMPARE(tabBar->shape(), QTabBar::RoundedNorth);

    // New position.
    mdiArea.setTabPosition(tabPosition);
    QCOMPARE(mdiArea.tabPosition(), tabPosition);
    QCOMPARE(tabBar->shape(), tabBarShapeFrom(QTabWidget::Rounded, tabPosition));

    const Qt::LayoutDirection originalLayoutDirection = qApp->layoutDirection();

    // Check that we have correct geometry in both RightToLeft and LeftToRight.
    for (int i = 0; i < 2; ++i) {
        // Check viewportMargins.
        const QRect viewportGeometry = mdiArea.viewport()->geometry();
        const int left = viewportGeometry.left();
        const int top = viewportGeometry.y();
        const int right = mdiArea.width() - viewportGeometry.width();
        const int bottom = mdiArea.height() - viewportGeometry.height();

        const QSize sizeHint = tabBar->sizeHint();

        if (hasLeftMargin)
            QCOMPARE(qApp->isLeftToRight() ? left : right, sizeHint.width());
        if (hasRightMargin)
            QCOMPARE(qApp->isLeftToRight() ? right : left, sizeHint.width());
        if (hasTopMargin || hasBottomMargin)
            QCOMPARE(hasTopMargin ? top : bottom, sizeHint.height());

        // Check actual tab bar geometry.
        const QRegion expectedTabBarGeometry = QRegion(mdiArea.rect()).subtracted(viewportGeometry);
        QVERIFY(!expectedTabBarGeometry.isEmpty());
        QCOMPARE(QRegion(tabBar->geometry()), expectedTabBarGeometry);

        if (i == 0)
            qApp->setLayoutDirection(originalLayoutDirection == Qt::LeftToRight ? Qt::RightToLeft : Qt::LeftToRight);
        qApp->processEvents();
    }

    qApp->setLayoutDirection(originalLayoutDirection);
}

void tst_QMdiArea::nativeSubWindows()
{
    const QString platformName = QGuiApplication::platformName();
    if (platformName != QLatin1String("xcb") && platformName != QLatin1String("windows"))
        QSKIP(qPrintable(QString::fromLatin1("nativeSubWindows() does not work on this platform (%1).").arg(platformName)));
#if defined(Q_OS_WIN) && !defined(QT_NO_OPENGL)
    if (QOpenGLContext::openGLModuleType() != QOpenGLContext::LibGL)
        QSKIP("nativeSubWindows() does not work with ANGLE on Windows, QTBUG-28545.");
#endif
    { // Add native widgets after show.
    QMdiArea mdiArea;
    mdiArea.addSubWindow(new QWidget);
    mdiArea.addSubWindow(new QWidget);
    mdiArea.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mdiArea));

    // No native widgets.
    QVERIFY(!mdiArea.viewport()->internalWinId());
    foreach (QMdiSubWindow *subWindow, mdiArea.subWindowList())
        QVERIFY(!subWindow->internalWinId());

    QWidget *nativeWidget = new QWidget;
    QVERIFY(nativeWidget->winId()); // enforce native window.
    QMdiSubWindow *subWin = mdiArea.addSubWindow(nativeWidget);
    QVERIFY(subWin->internalWinId());

    // The viewport and all the sub-windows must be native.
    QVERIFY(mdiArea.viewport()->internalWinId());
    foreach (QMdiSubWindow *subWindow, mdiArea.subWindowList())
        QVERIFY(subWindow->internalWinId());

    // Add a non-native widget. This should become native.
    QMdiSubWindow *subWindow = new QMdiSubWindow;
    subWindow->setWidget(new QWidget);
    QVERIFY(!subWindow->internalWinId());
    mdiArea.addSubWindow(subWindow);
    QVERIFY(subWindow->internalWinId());
    }

    { // Add native widgets before show.
    QMdiArea mdiArea;
    mdiArea.addSubWindow(new QWidget);
    QWidget *nativeWidget = new QWidget;
    (void)nativeWidget->winId();
    mdiArea.addSubWindow(nativeWidget);
    mdiArea.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mdiArea));

    // The viewport and all the sub-windows must be native.
    QVERIFY(mdiArea.viewport()->internalWinId());
    foreach (QMdiSubWindow *subWindow, mdiArea.subWindowList())
        QVERIFY(subWindow->internalWinId());
    }

    { // Make a sub-window native *after* it's added to the area.
    QMdiArea mdiArea;
    mdiArea.addSubWindow(new QWidget);
    mdiArea.addSubWindow(new QWidget);
    mdiArea.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mdiArea));

    QMdiSubWindow *nativeSubWindow = mdiArea.subWindowList().last();
    QVERIFY(!nativeSubWindow->internalWinId());
    (void)nativeSubWindow->winId();

    // All the sub-windows should be native at this point
    QVERIFY(mdiArea.viewport()->internalWinId());
    foreach (QMdiSubWindow *subWindow, mdiArea.subWindowList())
            QVERIFY(subWindow->internalWinId());
    }

#ifndef QT_NO_OPENGL
    {
    if (!QGLFormat::hasOpenGL())
        QSKIP("QGL not supported on this platform");

    QMdiArea mdiArea;
    QGLWidget *glViewport = new QGLWidget;
    mdiArea.setViewport(glViewport);
    mdiArea.addSubWindow(new QWidget);
    mdiArea.addSubWindow(new QWidget);
    mdiArea.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mdiArea));

    const QGLContext *context = glViewport->context();
    if (!context || !context->isValid())
        QSKIP("QGL is broken, cannot continue test");

    // The viewport and all the sub-windows must be native.
    QVERIFY(mdiArea.viewport()->internalWinId());
    foreach (QMdiSubWindow *subWindow, mdiArea.subWindowList())
        QVERIFY(subWindow->internalWinId());
    }
#endif
}

void tst_QMdiArea::task_209615()
{
    QTabWidget tabWidget;
    QMdiArea *mdiArea1 = new QMdiArea;
    QMdiArea *mdiArea2 = new QMdiArea;
    QMdiSubWindow *subWindow = mdiArea1->addSubWindow(new QLineEdit);

    tabWidget.addTab(mdiArea1, QLatin1String("1"));
    tabWidget.addTab(mdiArea2, QLatin1String("2"));
    tabWidget.show();

    mdiArea1->removeSubWindow(subWindow);
    mdiArea2->addSubWindow(subWindow);

    // Please do not assert/crash.
    tabWidget.setCurrentIndex(1);
}

void tst_QMdiArea::task_236750()
{
    QMdiArea mdiArea;
    QMdiSubWindow *subWindow = mdiArea.addSubWindow(new QTextEdit);
    mdiArea.show();

    subWindow->setWindowFlags(subWindow->windowFlags() | Qt::FramelessWindowHint);
    // Please do not crash (floating point exception).
    subWindow->showMinimized();
}

QTEST_MAIN(tst_QMdiArea)
#include "tst_qmdiarea.moc"

