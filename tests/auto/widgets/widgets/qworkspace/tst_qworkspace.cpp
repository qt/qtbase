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
#include <qapplication.h>
#include <qmainwindow.h>
#include <qmenubar.h>
#include <qworkspace.h>

//TESTED_CLASS=
//TESTED_FILES=

class tst_QWorkspace : public QObject
{
    Q_OBJECT

public:
    tst_QWorkspace();
    virtual ~tst_QWorkspace();


protected slots:
    void activeChanged( QWidget *w );
    void accelActivated();

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
private slots:
    void getSetCheck();
    void windowActivated_data();
    void windowActivated();
    void windowActivatedWithMinimize();
    void showWindows();
    void changeWindowTitle();
    void changeModified();
    void childSize();
    void fixedSize();
#if defined(Q_WS_WIN) || defined(Q_WS_X11)
    void nativeSubWindows();
#endif
    void task206368();

private:
    QWidget *activeWidget;
    bool accelPressed;
};

// Testing get/set functions
void tst_QWorkspace::getSetCheck()
{
    QWorkspace obj1;
    // bool QWorkspace::scrollBarsEnabled()
    // void QWorkspace::setScrollBarsEnabled(bool)
    obj1.setScrollBarsEnabled(false);
    QCOMPARE(false, obj1.scrollBarsEnabled());
    obj1.setScrollBarsEnabled(true);
    QCOMPARE(true, obj1.scrollBarsEnabled());
}

tst_QWorkspace::tst_QWorkspace()
 : activeWidget( 0 )
{
}

tst_QWorkspace::~tst_QWorkspace()
{

}

// initTestCase will be executed once before the first testfunction is executed.
void tst_QWorkspace::initTestCase()
{

}

// cleanupTestCase will be executed once after the last testfunction is executed.
void tst_QWorkspace::cleanupTestCase()
{
}

// init() will be executed immediately before each testfunction is run.
void tst_QWorkspace::init()
{
// TODO: Add testfunction specific initialization code here.
}

// cleanup() will be executed immediately after each testfunction is run.
void tst_QWorkspace::cleanup()
{
// TODO: Add testfunction specific cleanup code here.
}

void tst_QWorkspace::activeChanged( QWidget *w )
{
    activeWidget = w;
}

void tst_QWorkspace::windowActivated_data()
{
    // define the test elements we're going to use
    QTest::addColumn<int>("count");

    // create a first testdata instance and fill it with data
    QTest::newRow( "data0" ) << 0;
    QTest::newRow( "data1" ) << 1;
    QTest::newRow( "data2" ) << 2;
}

void tst_QWorkspace::windowActivated()
{
    QMainWindow mw(0, Qt::X11BypassWindowManagerHint);
    mw.menuBar();
    QWorkspace *workspace = new QWorkspace(&mw);
    workspace->setObjectName("testWidget");
    mw.setCentralWidget(workspace);
    QSignalSpy spy(workspace, SIGNAL(windowActivated(QWidget*)));
    connect( workspace, SIGNAL(windowActivated(QWidget*)), this, SLOT(activeChanged(QWidget*)) );
    mw.show();
    qApp->setActiveWindow(&mw);

    QFETCH( int, count );
    int i;

    for ( i = 0; i < count; ++i ) {
        QWidget *widget = new QWidget(workspace, 0);
        widget->setAttribute(Qt::WA_DeleteOnClose);
        workspace->addWindow(widget);
        widget->show();
        qApp->processEvents();
        QVERIFY( activeWidget == workspace->activeWindow() );
        QCOMPARE(spy.count(), 1);
        spy.clear();
    }

    QWidgetList windows = workspace->windowList();
    QCOMPARE( (int)windows.count(), count );

    for ( i = 0; i < count; ++i ) {
        QWidget *window = windows.at(i);
        window->showMinimized();
        qApp->processEvents();
        QVERIFY( activeWidget == workspace->activeWindow() );
        if ( i == 1 )
            QVERIFY( activeWidget == window );
    }

    for ( i = 0; i < count; ++i ) {
        QWidget *window = windows.at(i);
        window->showNormal();
        qApp->processEvents();
        QVERIFY( window == activeWidget );
        QVERIFY( activeWidget == workspace->activeWindow() );
    }
    spy.clear();

    while ( workspace->activeWindow() ) {
        workspace->activeWindow()->close();
        qApp->processEvents();
        QVERIFY( activeWidget == workspace->activeWindow() );
        QCOMPARE(spy.count(), 1);
        spy.clear();
    }
    QVERIFY(activeWidget == 0);
    QVERIFY(workspace->activeWindow() == 0);
    QVERIFY(workspace->windowList().count() == 0);

    {
        workspace->hide();
        QWidget *widget = new QWidget(workspace);
        widget->setObjectName("normal");
        widget->setAttribute(Qt::WA_DeleteOnClose);
        workspace->addWindow(widget);
        widget->show();
        QCOMPARE(spy.count(), 0);
        workspace->show();
        QCOMPARE(spy.count(), 1);
        spy.clear();
        QVERIFY( activeWidget == widget );
        widget->close();
        qApp->processEvents();
        QCOMPARE(spy.count(), 1);
        spy.clear();
        QVERIFY( activeWidget == 0 );
    }

    {
        workspace->hide();
        QWidget *widget = new QWidget(workspace);
        widget->setObjectName("maximized");
        widget->setAttribute(Qt::WA_DeleteOnClose);
        workspace->addWindow(widget);
        widget->showMaximized();
        qApp->sendPostedEvents();
#ifdef Q_WS_MAC
        QEXPECT_FAIL("", "This test has never passed on Mac. QWorkspace is obsoleted -> won't fix", Abort);
#endif
        QCOMPARE(spy.count(), 0);
        spy.clear();
        workspace->show();
        QCOMPARE(spy.count(), 1);
        spy.clear();
        QVERIFY( activeWidget == widget );
        widget->close();
        qApp->processEvents();
        QCOMPARE(spy.count(), 1);
        spy.clear();
        QVERIFY( activeWidget == 0 );
    }

    {
        QWidget *widget = new QWidget(workspace);
        widget->setObjectName("minimized");
        widget->setAttribute(Qt::WA_DeleteOnClose);
        workspace->addWindow(widget);
        widget->showMinimized();
        QCOMPARE(spy.count(), 1);
        spy.clear();
        QVERIFY( activeWidget == widget );
        QVERIFY(workspace->activeWindow() == widget);
        widget->close();
        qApp->processEvents();
        QCOMPARE(spy.count(), 1);
        spy.clear();
        QVERIFY(workspace->activeWindow() == 0);
        QVERIFY( activeWidget == 0 );
    }
}
void tst_QWorkspace::windowActivatedWithMinimize()
{
    QMainWindow mw(0, Qt::X11BypassWindowManagerHint) ;
    mw.menuBar();
    QWorkspace *workspace = new QWorkspace(&mw);
    workspace->setObjectName("testWidget");
    mw.setCentralWidget(workspace);
    QSignalSpy spy(workspace, SIGNAL(windowActivated(QWidget*)));
    connect( workspace, SIGNAL(windowActivated(QWidget*)), this, SLOT(activeChanged(QWidget*)) );
    mw.show();
    qApp->setActiveWindow(&mw);
    QWidget *widget = new QWidget(workspace);
    widget->setObjectName("minimized1");
    widget->setAttribute(Qt::WA_DeleteOnClose);
    workspace->addWindow(widget);
    QWidget *widget2 = new QWidget(workspace);
    widget2->setObjectName("minimized2");
    widget2->setAttribute(Qt::WA_DeleteOnClose);
    workspace->addWindow(widget2);

    widget->showMinimized();
    QVERIFY( activeWidget == widget );
    widget2->showMinimized();
    QVERIFY( activeWidget == widget2 );

    widget2->close();
    qApp->processEvents();
    QVERIFY( activeWidget == widget );

    widget->close();
    qApp->processEvents();
    QVERIFY(workspace->activeWindow() == 0);
    QVERIFY( activeWidget == 0 );

    QVERIFY( workspace->windowList().count() == 0 );
}

void tst_QWorkspace::accelActivated()
{
    accelPressed = TRUE;
}

void tst_QWorkspace::showWindows()
{
    QWorkspace *ws = new QWorkspace( 0 );

    QWidget *widget = 0;
    ws->show();

    widget = new QWidget(ws);
    widget->setObjectName("plain1");
    widget->show();
    QVERIFY( widget->isVisible() );

    widget = new QWidget(ws);
    widget->setObjectName("maximized1");
    widget->showMaximized();
    QVERIFY( widget->isMaximized() );
    widget->showNormal();
    QVERIFY( !widget->isMaximized() );

    widget = new QWidget(ws);
    widget->setObjectName("minimized1");
    widget->showMinimized();
    QVERIFY( widget->isMinimized() );
    widget->showNormal();
    QVERIFY( !widget->isMinimized() );

    ws->hide();

    widget = new QWidget(ws);
    widget->setObjectName("plain2");
    ws->show();
    QVERIFY( widget->isVisible() );

    ws->hide();

    widget = new QWidget(ws);
    widget->setObjectName("maximized2");
    widget->showMaximized();
    QVERIFY( widget->isMaximized() );
    ws->show();
    QVERIFY( widget->isVisible() );
    QVERIFY( widget->isMaximized() );
    ws->hide();

    widget = new QWidget(ws);
    widget->setObjectName("minimized2");
    widget->showMinimized();
    ws->show();
    QVERIFY( widget->isMinimized() );
    ws->hide();

    delete ws;
}


//#define USE_SHOW

void tst_QWorkspace::changeWindowTitle()
{
#ifdef Q_OS_WINCE
    QSKIP( "Test fails on Windows CE due to QWorkspace state handling");
#endif
    const QString mwc( "MainWindow's Caption" );
    const QString mwc2( "MainWindow's New Caption" );
    const QString wc( "Widget's Caption" );
    const QString wc2( "Widget's New Caption" );

    QMainWindow *mw = new QMainWindow(0, Qt::X11BypassWindowManagerHint);
    mw->setWindowTitle( mwc );
    QWorkspace *ws = new QWorkspace( mw );
    mw->setCentralWidget( ws );


    QWidget *widget = new QWidget( ws );
    widget->setWindowTitle( wc );
    ws->addWindow(widget);

    QCOMPARE( mw->windowTitle(), mwc );


#ifdef USE_SHOW
    widget->showMaximized();
#else
    widget->setWindowState(Qt::WindowMaximized);
#endif
    QCOMPARE( mw->windowTitle(), QString("%1 - [%2]").arg(mwc).arg(wc) );

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
    QCOMPARE( mw->windowTitle(), QString("%1 - [%2]").arg(mwc).arg(wc) );
    widget->setWindowTitle( wc2 );
    QCOMPARE( mw->windowTitle(), QString("%1 - [%2]").arg(mwc).arg(wc2) );
    mw->setWindowTitle( mwc2 );
    QCOMPARE( mw->windowTitle(), QString("%1 - [%2]").arg(mwc2).arg(wc2) );

    mw->show();
    qApp->setActiveWindow(mw);

#ifdef USE_SHOW
    mw->showFullScreen();
#else
    mw->setWindowState(Qt::WindowFullScreen);
#endif

    qApp->processEvents();
    QCOMPARE( mw->windowTitle(), QString("%1 - [%2]").arg(mwc2).arg(wc2) );
#ifdef USE_SHOW
    widget->showNormal();
#else
    widget->setWindowState(Qt::WindowNoState);
#endif
    qApp->processEvents();
    QCOMPARE( mw->windowTitle(), mwc2 );
#ifdef USE_SHOW
    widget->showMaximized();
#else
    widget->setWindowState(Qt::WindowMaximized);
#endif
    qApp->processEvents();
    QCOMPARE( mw->windowTitle(), QString("%1 - [%2]").arg(mwc2).arg(wc2) );

#ifdef USE_SHOW
    mw->showNormal();
#else
    mw->setWindowState(Qt::WindowNoState);
#endif
    qApp->processEvents();
    QCOMPARE( mw->windowTitle(), QString("%1 - [%2]").arg(mwc2).arg(wc2) );
#ifdef USE_SHOW
    widget->showNormal();
#else
    widget->setWindowState(Qt::WindowNoState);
#endif
    QCOMPARE( mw->windowTitle(), mwc2 );

    delete mw;
}

void tst_QWorkspace::changeModified()
{
    const QString mwc( "MainWindow's Caption" );
    const QString wc( "Widget's Caption[*]" );

    QMainWindow *mw = new QMainWindow(0, Qt::X11BypassWindowManagerHint);
    mw->setWindowTitle( mwc );
    QWorkspace *ws = new QWorkspace( mw );
    mw->setCentralWidget( ws );

    QWidget *widget = new QWidget( ws );
    widget->setWindowTitle( wc );
    ws->addWindow(widget);

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
    QCOMPARE( mw->isWindowModified(), true);
    QCOMPARE( widget->isWindowModified(), true);

    widget->setWindowState(Qt::WindowNoState);
    QCOMPARE( mw->isWindowModified(), false);
    QCOMPARE( widget->isWindowModified(), true);

    widget->setWindowState(Qt::WindowMaximized);
    QCOMPARE( mw->isWindowModified(), true);
    QCOMPARE( widget->isWindowModified(), true);

    widget->setWindowModified(false);
    QCOMPARE( mw->isWindowModified(), false);
    QCOMPARE( widget->isWindowModified(), false);

    widget->setWindowModified(true);
    QCOMPARE( mw->isWindowModified(), true);
    QCOMPARE( widget->isWindowModified(), true);

    widget->setWindowState(Qt::WindowNoState);
    QCOMPARE( mw->isWindowModified(), false);
    QCOMPARE( widget->isWindowModified(), true);

    delete mw;
}

class MyChild : public QWidget
{
public:
    MyChild(QWidget *parent = 0, Qt::WFlags f = 0)
        : QWidget(parent, f)
    {
    }

    QSize sizeHint() const
    {
        return QSize(234, 123);
    }
};

void tst_QWorkspace::childSize()
{
    QWorkspace ws;

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

void tst_QWorkspace::fixedSize()
{
    QWorkspace *ws = new QWorkspace;
    int i;

    ws->resize(500, 500);
//     ws->show();

    QSize fixed(300, 300);
    for (i = 0; i < 4; ++i) {
        QWidget *child = new QWidget(ws);
        child->setFixedSize(fixed);
        child->show();
    }

    QWidgetList windows = ws->windowList();
    for (i = 0; i < (int)windows.count(); ++i) {
        QWidget *child = windows.at(i);
        QCOMPARE(child->size(), fixed);
        QCOMPARE(child->visibleRegion().boundingRect().size(), fixed);
    }

    ws->cascade();
    ws->resize(800, 800);
    for (i = 0; i < (int)windows.count(); ++i) {
        QWidget *child = windows.at(i);
        QCOMPARE(child->size(), fixed);
        QCOMPARE(child->visibleRegion().boundingRect().size(), fixed);
    }
    ws->resize(500, 500);

    ws->tile();
    ws->resize(800, 800);
    for (i = 0; i < (int)windows.count(); ++i) {
        QWidget *child = windows.at(i);
        QCOMPARE(child->size(), fixed);
        QCOMPARE(child->visibleRegion().boundingRect().size(), fixed);
    }
    ws->resize(500, 500);

    for (i = 0; i < (int)windows.count(); ++i) {
        QWidget *child = windows.at(i);
        delete child;
    }

    delete ws;
}

#if defined(Q_WS_WIN) || defined(Q_WS_X11)
void tst_QWorkspace::nativeSubWindows()
{
    { // Add native widgets after show.
    QWorkspace workspace;
    workspace.addWindow(new QWidget);
    workspace.addWindow(new QWidget);
    workspace.show();
#ifdef Q_WS_X11
    qt_x11_wait_for_window_manager(&workspace);
#endif

    // No native widgets.
    foreach (QWidget *subWindow, workspace.windowList())
        QVERIFY(!subWindow->parentWidget()->internalWinId());

    QWidget *nativeWidget = new QWidget;
    QVERIFY(nativeWidget->winId()); // enforce native window.
    workspace.addWindow(nativeWidget);

    // All the sub-windows must be native.
    foreach (QWidget *subWindow, workspace.windowList())
        QVERIFY(subWindow->parentWidget()->internalWinId());

    // Add a non-native widget. This should become native.
    QWidget *subWindow = workspace.addWindow(new QWidget);
    QVERIFY(subWindow->parentWidget()->internalWinId());
    }

    { // Add native widgets before show.
    QWorkspace workspace;
    workspace.addWindow(new QWidget);
    QWidget *nativeWidget = new QWidget;
    (void)nativeWidget->winId();
    workspace.addWindow(nativeWidget);
    workspace.show();
#ifdef Q_WS_X11
    qt_x11_wait_for_window_manager(&workspace);
#endif

    // All the sub-windows must be native.
    foreach (QWidget *subWindow, workspace.windowList())
        QVERIFY(subWindow->parentWidget()->internalWinId());
    }

    { // Make a sub-window native *after* it's added to the area.
    QWorkspace workspace;
    workspace.addWindow(new QWidget);
    workspace.addWindow(new QWidget);
    workspace.show();
#ifdef Q_WS_X11
    qt_x11_wait_for_window_manager(&workspace);
#endif

    QWidget *nativeSubWindow = workspace.windowList().last()->parentWidget();
    QVERIFY(!nativeSubWindow->internalWinId());
    (void)nativeSubWindow->winId();

    // All the sub-windows should be native at this point.
    foreach (QWidget *subWindow, workspace.windowList())
        QVERIFY(subWindow->parentWidget()->internalWinId());
    }
}
#endif

void tst_QWorkspace::task206368()
{
    // Make sure the internal list of iconified windows doesn't contain dangling pointers.
    QWorkspace workspace;
    QWidget *child = new QWidget;
    QWidget *window = workspace.addWindow(child);
    workspace.show();
    child->showMinimized();
    delete window;
    // This shouldn't crash.
    workspace.arrangeIcons();
}

QTEST_MAIN(tst_QWorkspace)
#include "tst_qworkspace.moc"
