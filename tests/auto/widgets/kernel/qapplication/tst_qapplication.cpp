/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


//#define QT_TST_QAPP_DEBUG
#include <qdebug.h>

#include <QtTest/QtTest>

#include <QtCore/QAbstractEventDispatcher>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/private/qeventloop_p.h>

#include <QtGui/QFontDatabase>
#include <QtGui/QClipboard>

#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QStyleFactory>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/private/qapplication_p.h>
#include <QtWidgets/QStyle>

#ifdef Q_OS_WINCE
#include <windows.h>
#endif

#include <qpa/qwindowsysteminterface.h>

#include "../../../qtest-config.h"

QT_BEGIN_NAMESPACE
static QWindowSystemInterface::TouchPoint touchPoint(const QTouchEvent::TouchPoint& pt)
{
    QWindowSystemInterface::TouchPoint p;
    p.id = pt.id();
    p.flags = pt.flags();
    p.normalPosition = pt.normalizedPos();
    p.area = pt.screenRect();
    p.pressure = pt.pressure();
    p.state = pt.state();
    p.velocity = pt.velocity();
    p.rawPositions = pt.rawScreenPositions();
    return p;
}

static QList<struct QWindowSystemInterface::TouchPoint> touchPointList(const QList<QTouchEvent::TouchPoint>& pointList)
{
    QList<struct QWindowSystemInterface::TouchPoint> newList;

    Q_FOREACH (QTouchEvent::TouchPoint p, pointList)
    {
        newList.append(touchPoint(p));
    }
    return newList;
}



extern bool Q_GUI_EXPORT qt_tab_all_widgets(); // from qapplication.cpp
QT_END_NAMESPACE

class tst_QApplication : public QObject
{
Q_OBJECT

public:
    tst_QApplication();
    virtual ~tst_QApplication();

public slots:
    void initTestCase();
    void init();
    void cleanup();
private slots:
    void sendEventsOnProcessEvents(); // this must be the first test
    void staticSetup();

    void alert();

    void multiple_data();
    void multiple();

    void nonGui();

    void setFont_data();
    void setFont();

    void args_data();
    void args();
    void appName();

    void lastWindowClosed();
    void quitOnLastWindowClosed();
    void closeAllWindows();
    void testDeleteLater();
    void testDeleteLaterProcessEvents();

    void libraryPaths();
    void libraryPaths_qt_plugin_path();
    void libraryPaths_qt_plugin_path_2();

    void sendPostedEvents();

    void thread();
    void desktopSettingsAware();

    void setActiveWindow();

    void focusChanged();
    void focusOut();

    void execAfterExit();

    void wheelScrollLines();

    void task109149();

    void style();

    void allWidgets();
    void topLevelWidgets();

    void setAttribute();

    void windowsCommandLine_data();
    void windowsCommandLine();

    void touchEventPropagation();

    void qtbug_12673();
    void noQuitOnHide();

    void globalStaticObjectDestruction(); // run this last

    void abortQuitOnShow();
};

class EventSpy : public QObject
{
   Q_OBJECT

public:
    QList<int> recordedEvents;
    bool eventFilter(QObject *, QEvent *event)
    {
        recordedEvents.append(event->type());
        return false;
    }
};

void tst_QApplication::initTestCase()
{
    // chdir to our testdata path and execute helper apps relative to that.
    const QString testdataDir = QFileInfo(QFINDTESTDATA("desktopsettingsaware")).absolutePath();
    QVERIFY2(QDir::setCurrent(testdataDir), qPrintable("Could not chdir to " + testdataDir));
}

void tst_QApplication::sendEventsOnProcessEvents()
{
    int argc = 0;
    QApplication app(argc, 0);

    EventSpy spy;
    app.installEventFilter(&spy);

    QCoreApplication::postEvent(&app,  new QEvent(QEvent::Type(QEvent::User + 1)));
    QCoreApplication::processEvents();
    QVERIFY(spy.recordedEvents.contains(QEvent::User + 1));
}


class CloseEventTestWindow : public QWidget
{
public:
    CloseEventTestWindow(QWidget *parent = 0)
        : QWidget(parent)
    {
    }

    void closeEvent(QCloseEvent *event)
    {
        QWidget dialog;
        dialog.show();
        dialog.close();

        event->ignore();
    }
};

static  char *argv0;

tst_QApplication::tst_QApplication()
{
#ifdef Q_OS_WINCE
    // Clean up environment previously to launching test
    qputenv("QT_PLUGIN_PATH", QByteArray());
#endif
}

tst_QApplication::~tst_QApplication()
{

}

void tst_QApplication::init()
{
// TODO: Add initialization code here.
// This will be executed immediately before each test is run.
}

void tst_QApplication::cleanup()
{
// TODO: Add cleanup code here.
// This will be executed immediately after each test is run.
}

void tst_QApplication::staticSetup()
{
    QVERIFY(!qApp);

    QStyle *style = QStyleFactory::create(QLatin1String("Windows"));
    QVERIFY(style);
    QApplication::setStyle(style);

    QPalette pal;
    QApplication::setPalette(pal);

    /*QFont font;
    QApplication::setFont(font);*/

    int argc = 0;
    QApplication app(argc, 0);
}


// QApp subclass that exits the event loop after 150ms
class TestApplication : public QApplication
{
public:
    TestApplication( int &argc, char **argv )
    : QApplication( argc, argv)
    {
	startTimer( 150 );
    }

    void timerEvent( QTimerEvent * )
    {
        quit();
    }
};

void tst_QApplication::alert()
{
    int argc = 0;
    QApplication app(argc, 0);
    app.alert(0, 0);

    QWidget widget;
    QWidget widget2;
    app.alert(&widget, 100);
    widget.show();
    widget2.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QVERIFY(QTest::qWaitForWindowExposed(&widget2));
    QTest::qWait(100);
    app.alert(&widget, -1);
    app.alert(&widget, 250);
    widget2.activateWindow();
    QApplication::setActiveWindow(&widget2);
    app.alert(&widget, 0);
    widget.activateWindow();
    QApplication::setActiveWindow(&widget);
    app.alert(&widget, 200);
}

void tst_QApplication::multiple_data()
{
    QTest::addColumn<QStringList>("features");

    // return a list of things to try
    QTest::newRow( "data0" ) << QStringList( "" );
    QTest::newRow( "data1" ) << QStringList( "QFont" );
    QTest::newRow( "data2" ) << QStringList( "QPixmap" );
    QTest::newRow( "data3" ) << QStringList( "QWidget" );
}

void tst_QApplication::multiple()
{
    QFETCH(QStringList,features);

    int i = 0;
    int argc = 0;
    while ( i++ < 5 ) {
	TestApplication app( argc, 0 );

	if ( features.contains( "QFont" ) ) {
	    // create font and force loading
	    QFont font( "Arial", 12 );
	    QFontInfo finfo( font );
	    finfo.exactMatch();
	}
	if ( features.contains( "QPixmap" ) ) {
	    QPixmap pix( 100, 100 );
	    pix.fill( Qt::black );
	}
	if ( features.contains( "QWidget" ) ) {
	    QWidget widget;
	}

	QVERIFY(!app.exec());
    }
}

void tst_QApplication::nonGui()
{
#ifdef Q_OS_HPUX
    // ### This is only to allow us to generate a test report for now.
    QSKIP("This test shuts down the window manager on HP-UX.");
#endif

    int argc = 0;
    QApplication app(argc, 0, false);
    QCOMPARE(qApp, &app);
}

void tst_QApplication::setFont_data()
{
    QTest::addColumn<QString>("family");
    QTest::addColumn<int>("pointsize");
    QTest::addColumn<bool>("beforeAppConstructor");

    int argc = 0;
    QApplication app(argc, 0); // Needed for QFontDatabase

    int cnt = 0;
    QFontDatabase fdb;
    QStringList families = fdb.families();
    for (QStringList::const_iterator itr = families.begin();
	 itr != families.end();
	 ++itr) {
	if (cnt < 3) {
	    QString family = *itr;
	    QStringList styles = fdb.styles(family);
	    if (styles.size() > 0) {
		QString style = styles.first();
		QList<int> sizes = fdb.pointSizes(family, style);
		if (!sizes.size())
		    sizes = fdb.standardSizes();
		if (sizes.size() > 0) {
		    QTest::newRow(QString("data%1a").arg(cnt).toLatin1().constData())
			<< family
			<< sizes.first()
                        << false;
		    QTest::newRow(QString("data%1b").arg(cnt).toLatin1().constData())
			<< family
			<< sizes.first()
                        << true;
                }
	    }
	}
	++cnt;
    }

    QTest::newRow("nonexistingfont after") << "nosuchfont_probably_quiteunlikely"
        << 0 << false;
    QTest::newRow("nonexistingfont before") << "nosuchfont_probably_quiteunlikely"
        << 0 << true;

    QTest::newRow("largescaleable after") << "smoothtimes" << 100 << false;
    QTest::newRow("largescaleable before") << "smoothtimes" << 100 << true;

    QTest::newRow("largeunscaleale after") << "helvetica" << 100 << false;
    QTest::newRow("largeunscaleale before") << "helvetica" << 100 << true;
}

void tst_QApplication::setFont()
{
    QFETCH( QString, family );
    QFETCH( int, pointsize );
    QFETCH( bool, beforeAppConstructor );

    QFont font( family, pointsize );
    if (beforeAppConstructor) {
        QApplication::setFont( font );
        QCOMPARE(QApplication::font(), font);
    }

    int argc = 0;
    QApplication app(argc, 0);
    if (!beforeAppConstructor)
        QApplication::setFont( font );

    QCOMPARE( app.font(), font );
}

void tst_QApplication::args_data()
{
    QTest::addColumn<int>("argc_in");
    QTest::addColumn<QString>("args_in");
    QTest::addColumn<int>("argc_out");
    QTest::addColumn<QString>("args_out");

    QTest::newRow( "App name" ) << 1 << "/usr/bin/appname" << 1 << "/usr/bin/appname";
    QTest::newRow( "No arguments" ) << 0 << QString() << 0 << QString();
    QTest::newRow( "App name, style" ) << 3 << "/usr/bin/appname -style windows" << 1 << "/usr/bin/appname";
    QTest::newRow( "App name, style, arbitrary, reverse" ) << 5 << "/usr/bin/appname -style windows -arbitrary -reverse"
							<< 2 << "/usr/bin/appname -arbitrary";
}

void tst_QApplication::task109149()
{
    int argc = 0;
    QApplication app(argc, 0);
    QApplication::setFont(QFont("helvetica", 100));

    QWidget w;
    w.setWindowTitle("hello");
    w.show();

    app.processEvents();
}

static char ** QString2cstrings( const QString &args )
{
    static QList<QByteArray> cache;

    int i;
    char **argarray = 0;
    QStringList list = args.split(' ');;
    argarray = new char*[list.count()+1];

    for (i = 0; i < (int)list.count(); ++i ) {
        QByteArray l1 = list[i].toLatin1();
        argarray[i] = l1.data();
        cache.append(l1);
    }
    argarray[i] = 0;

    return argarray;
}

static QString cstrings2QString( char **args )
{
    QString string;
    if ( !args )
	return string;

    int i = 0;
    while ( args[i] ) {
	string += args[i];
	if ( args[i+1] )
	    string += " ";
	++i;
    }
    return string;
}

void tst_QApplication::args()
{
    QFETCH( int, argc_in );
    QFETCH( QString, args_in );
    QFETCH( int, argc_out );
    QFETCH( QString, args_out );

    char **argv = QString2cstrings( args_in );

    QApplication app( argc_in, argv);
    QString argv_out = cstrings2QString(argv);

    QCOMPARE( argc_in, argc_out );
    QCOMPARE( argv_out, args_out );

    delete [] argv;
    // Make sure we switch back to native style.
    QApplicationPrivate::styleOverride = QString();
}

void tst_QApplication::appName()
{
    char argv0[] = "tst_qapplication";
    char *argv[] = { argv0, 0 };
    int argc = 1;
    QApplication app(argc, argv);
    QCOMPARE(::qAppName(), QString::fromLatin1("tst_qapplication"));
    QCOMPARE(QCoreApplication::applicationName(), QString::fromLatin1("tst_qapplication"));
}

class CloseWidget : public QWidget
{
    Q_OBJECT
public:
    CloseWidget()
    {
        startTimer(500);
    }

protected:
    void timerEvent(QTimerEvent *)
    {
        close();
    }

};

void tst_QApplication::lastWindowClosed()
{
    int argc = 0;
    QApplication app(argc, 0);

    QSignalSpy spy(&app, SIGNAL(lastWindowClosed()));

    QPointer<QDialog> dialog = new QDialog;
    QVERIFY(dialog->testAttribute(Qt::WA_QuitOnClose));
    QTimer::singleShot(1000, dialog, SLOT(accept()));
    dialog->exec();
    QVERIFY(dialog);
    QCOMPARE(spy.count(), 0);

    QPointer<CloseWidget>widget = new CloseWidget;
    QVERIFY(widget->testAttribute(Qt::WA_QuitOnClose));
    widget->show();
    QObject::connect(&app, SIGNAL(lastWindowClosed()), widget, SLOT(deleteLater()));
    app.exec();
    QVERIFY(!widget);
    QCOMPARE(spy.count(), 1);
    spy.clear();

    delete dialog;

    // show 3 windows, close them, should only get lastWindowClosed once
    QWidget w1;
    QWidget w2;
    QWidget w3;
    w1.show();
    w2.show();
    w3.show();

    QTimer::singleShot(1000, &app, SLOT(closeAllWindows()));
    app.exec();
    QCOMPARE(spy.count(), 1);
}

class QuitOnLastWindowClosedDialog : public QDialog
{
    Q_OBJECT
public:
    QPushButton *okButton;

    QuitOnLastWindowClosedDialog()
    {
        QHBoxLayout *hbox = new QHBoxLayout(this);
        okButton = new QPushButton("&ok", this);

        hbox->addWidget(okButton);
        connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
        connect(okButton, SIGNAL(clicked()), this, SLOT(ok_clicked()));
    }

public slots:
    void ok_clicked()
    {
        QDialog other;

        QTimer timer;
        connect(&timer, SIGNAL(timeout()), &other, SLOT(accept()));
        QSignalSpy spy(&timer, SIGNAL(timeout()));
        QSignalSpy appSpy(qApp, SIGNAL(lastWindowClosed()));

        timer.start(1000);
        other.exec();

        // verify that the eventloop ran and let the timer fire
        QCOMPARE(spy.count(), 1);
        QCOMPARE(appSpy.count(), 1);
    }
};

class QuitOnLastWindowClosedWindow : public QWidget
{
    Q_OBJECT

public:
    QuitOnLastWindowClosedWindow()
    { }

public slots:
    void execDialogThenShow()
    {
        QDialog dialog;
        QTimer timer1;
        connect(&timer1, SIGNAL(timeout()), &dialog, SLOT(accept()));
        QSignalSpy spy1(&timer1, SIGNAL(timeout()));
        timer1.setSingleShot(true);
        timer1.start(1000);
        dialog.exec();
        QCOMPARE(spy1.count(), 1);

        show();
    }
};

void tst_QApplication::quitOnLastWindowClosed()
{
    {
        int argc = 0;
        QApplication app(argc, 0);

        QuitOnLastWindowClosedDialog d;
        d.show();
        QTimer::singleShot(1000, d.okButton, SLOT(animateClick()));

        QSignalSpy appSpy(&app, SIGNAL(lastWindowClosed()));
        app.exec();

        // lastWindowClosed() signal should only be sent after the last dialog is closed
        QCOMPARE(appSpy.count(), 2);
    }
    {
        int argc = 0;
        QApplication app(argc, 0);
        QSignalSpy appSpy(&app, SIGNAL(lastWindowClosed()));

        QDialog dialog;
        QTimer timer1;
        connect(&timer1, SIGNAL(timeout()), &dialog, SLOT(accept()));
        QSignalSpy spy1(&timer1, SIGNAL(timeout()));
        timer1.setSingleShot(true);
        timer1.start(1000);
        dialog.exec();
        QCOMPARE(spy1.count(), 1);
        QCOMPARE(appSpy.count(), 0);

        QTimer timer2;
        connect(&timer2, SIGNAL(timeout()), &app, SLOT(quit()));
        QSignalSpy spy2(&timer2, SIGNAL(timeout()));
        timer2.setSingleShot(true);
        timer2.start(1000);
        int returnValue = app.exec();
        QCOMPARE(returnValue, 0);
        QCOMPARE(spy2.count(), 1);
        QCOMPARE(appSpy.count(), 0);
    }
    {
        int argc = 0;
        QApplication app(argc, 0);
        QTimer timer;
        timer.setInterval(100);

        QSignalSpy spy(&app, SIGNAL(aboutToQuit()));
        QSignalSpy spy2(&timer, SIGNAL(timeout()));

        QMainWindow mainWindow;
        QDialog *dialog = new QDialog(&mainWindow);

        QVERIFY(app.quitOnLastWindowClosed());
        QVERIFY(mainWindow.testAttribute(Qt::WA_QuitOnClose));
        QVERIFY(dialog->testAttribute(Qt::WA_QuitOnClose));

        mainWindow.show();
        QVERIFY(QTest::qWaitForWindowExposed(&mainWindow));
        dialog->show();
        QVERIFY(QTest::qWaitForWindowExposed(dialog));

        timer.start();
        QTimer::singleShot(1000, &mainWindow, SLOT(close())); // This should quit the application
        QTimer::singleShot(2000, &app, SLOT(quit()));        // This makes sure we quit even if it didn't

        app.exec();

        QCOMPARE(spy.count(), 1);
        QVERIFY(spy2.count() < 15);      // Should be around 10 if closing caused the quit
    }
    {
        int argc = 0;
        QApplication app(argc, 0);
        QTimer timer;
        timer.setInterval(100);

        QSignalSpy spy(&app, SIGNAL(aboutToQuit()));
        QSignalSpy spy2(&timer, SIGNAL(timeout()));

        CloseEventTestWindow mainWindow;

        QVERIFY(app.quitOnLastWindowClosed());
        QVERIFY(mainWindow.testAttribute(Qt::WA_QuitOnClose));

        mainWindow.show();
        QVERIFY(QTest::qWaitForWindowExposed(&mainWindow));

        timer.start();
        QTimer::singleShot(1000, &mainWindow, SLOT(close())); // This should quit the application
        QTimer::singleShot(2000, &app, SLOT(quit()));        // This makes sure we quit even if it didn't

        app.exec();

        QCOMPARE(spy.count(), 1);
        QVERIFY(spy2.count() > 15);      // Should be around 20 if closing did not caused the quit
    }
    {
        int argc = 0;
        QApplication app(argc, 0);
        QSignalSpy appSpy(&app, SIGNAL(lastWindowClosed()));

        // exec a dialog for 1 second, then show the window
        QuitOnLastWindowClosedWindow window;
        QTimer::singleShot(0, &window, SLOT(execDialogThenShow()));

        QTimer timer;
        QSignalSpy timerSpy(&timer, SIGNAL(timeout()));
        connect(&timer, SIGNAL(timeout()), &window, SLOT(close()));
        timer.setSingleShot(true);
        timer.start(2000);
        int returnValue = app.exec();
        QCOMPARE(returnValue, 0);
        // failure here means the timer above didn't fire, and the
        // quit was caused the dialog being closed (not the window)
        QCOMPARE(timerSpy.count(), 1);
        QCOMPARE(appSpy.count(), 2);
    }
    {
        int argc = 0;
        QApplication app(argc, 0);
        QVERIFY(app.quitOnLastWindowClosed());

        QTimer timer;
        timer.setInterval(100);
        QSignalSpy timerSpy(&timer, SIGNAL(timeout()));

        QWindow w;
        w.show();

        QWidget wid;
        wid.show();

        timer.start();
        QTimer::singleShot(1000, &wid, SLOT(close())); // This should NOT quit the application because the
                                                       // QWindow is still there.
        QTimer::singleShot(2000, &app, SLOT(quit()));  // This causes the quit.

        app.exec();

        QVERIFY(timerSpy.count() > 15);      // Should be around 20 if closing did not caused the quit
    }
    {   // QTBUG-31569: If the last widget with Qt::WA_QuitOnClose set is closed, other
        // widgets that don't have the attribute set should be closed automatically.
        int argc = 0;
        QApplication app(argc, 0);
        QVERIFY(app.quitOnLastWindowClosed());

        QWidget w1;
        w1.show();

        QWidget w2;
        w2.setAttribute(Qt::WA_QuitOnClose, false);
        w2.show();

        QVERIFY(QTest::qWaitForWindowExposed(&w2));

        QTimer timer;
        timer.setInterval(100);
        timer.start();
        QSignalSpy timerSpy(&timer, SIGNAL(timeout()));

        QTimer::singleShot(100, &w1, SLOT(close()));
        app.exec();

        QVERIFY(timerSpy.count() < 10);
    }
}

class PromptOnCloseWidget : public QWidget
{
public:
    void closeEvent(QCloseEvent *event)
    {
        QMessageBox *messageBox = new QMessageBox(this);
        messageBox->setWindowTitle("Unsaved data");
        messageBox->setText("Would you like to save or discard your current data?");
        messageBox->setStandardButtons(QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel);
        messageBox->setDefaultButton(QMessageBox::Save);

        messageBox->show();
        QVERIFY(QTest::qWaitForWindowExposed(messageBox));

        // verify that all windows are visible
        foreach (QWidget *w, qApp->topLevelWidgets())
            QVERIFY(w->isVisible());
        // flush event queue
        qApp->processEvents();
        // close all windows
        qApp->closeAllWindows();

        if (messageBox->standardButton(messageBox->clickedButton()) == QMessageBox::Cancel)
            event->ignore();
        else
            event->accept();

        delete messageBox;
    }
};

void tst_QApplication::closeAllWindows()
{
    int argc = 0;
    QApplication app(argc, 0);

    // create some windows
    new QWidget;
    new QWidget;
    new QWidget;

    // show all windows
    foreach (QWidget *w, app.topLevelWidgets()) {
        w->show();
        QVERIFY(QTest::qWaitForWindowExposed(w));
    }
    // verify that they are visible
    foreach (QWidget *w, app.topLevelWidgets())
        QVERIFY(w->isVisible());
    // empty event queue
    app.processEvents();
    // close all windows
    app.closeAllWindows();
    // all windows should no longer be visible
    foreach (QWidget *w, app.topLevelWidgets())
        QVERIFY(!w->isVisible());

    // add a window that prompts the user when closed
    PromptOnCloseWidget *promptOnCloseWidget = new PromptOnCloseWidget;
    // show all windows
    foreach (QWidget *w, app.topLevelWidgets()) {
        w->show();
        QVERIFY(QTest::qWaitForWindowExposed(w));
    }
    // close the last window to open the prompt (eventloop recurses)
    promptOnCloseWidget->close();
    // all windows should not be visible, except the one that opened the prompt
    foreach (QWidget *w, app.topLevelWidgets()) {
        if (w == promptOnCloseWidget)
            QVERIFY(w->isVisible());
        else
            QVERIFY(!w->isVisible());
    }

    qDeleteAll(app.topLevelWidgets());
}

bool isPathListIncluded(const QStringList &l, const QStringList &r)
{
    int size = r.count();
    if (size > l.count())
        return false;
#if defined (Q_OS_WIN)
    Qt::CaseSensitivity cs = Qt::CaseInsensitive;
#else
    Qt::CaseSensitivity cs = Qt::CaseSensitive;
#endif
    int i = 0, j = 0;
    for ( ; i < l.count() && j < r.count(); ++i) {
        if (QDir::toNativeSeparators(l[i]).compare(QDir::toNativeSeparators(r[j]), cs) == 0) {
            ++j;
            i = -1;
        }
    }
    return j == r.count();
}

#define QT_TST_QAPP_DEBUG
void tst_QApplication::libraryPaths()
{
    {
#ifndef Q_OS_WINCE
        QString testDir = QFileInfo(QFINDTESTDATA("test/test.pro")).absolutePath();
#else
        // On Windows CE we need QApplication object to have valid
        // current Path. Therefore we need to identify it ourselves
        // here for the test.
        QFileInfo filePath;
        wchar_t module_name[MAX_PATH];
        GetModuleFileName(0, module_name, MAX_PATH);
        filePath = QString::fromWCharArray(module_name);
        QString testDir = filePath.path() + "/test";
#endif
        QApplication::setLibraryPaths(QStringList() << testDir);
        QCOMPARE(QApplication::libraryPaths(), (QStringList() << testDir));

        // creating QApplication adds the applicationDirPath to the libraryPath
        int argc = 1;
        QApplication app(argc, &argv0);
        QString appDirPath = QDir(app.applicationDirPath()).canonicalPath();

        QStringList actual = QApplication::libraryPaths();
        actual.sort();
        QStringList expected = QSet<QString>::fromList((QStringList() << testDir << appDirPath)).toList();
        expected.sort();

        QVERIFY2(isPathListIncluded(actual, expected),
                 qPrintable("actual:\n - " + actual.join("\n - ") +
                            "\nexpected:\n - " + expected.join("\n - ")));
    }
    {
        // creating QApplication adds the applicationDirPath and plugin install path to the libraryPath
        int argc = 1;
        QApplication app(argc, &argv0);
        QString appDirPath = app.applicationDirPath();
        QString installPathPlugins =  QLibraryInfo::location(QLibraryInfo::PluginsPath);

        QStringList actual = QApplication::libraryPaths();
        actual.sort();

        QStringList expected = QSet<QString>::fromList((QStringList() << installPathPlugins << appDirPath)).toList();
        expected.sort();

        QVERIFY2(isPathListIncluded(actual, expected),
                 qPrintable("actual:\n - " + actual.join("\n - ") +
                            "\nexpected:\n - " + expected.join("\n - ")));

        // setting the library paths overrides everything
        QString testDir = QFileInfo(QFINDTESTDATA("test/test.pro")).absolutePath();
        QApplication::setLibraryPaths(QStringList() << testDir);
        QVERIFY2(isPathListIncluded(QApplication::libraryPaths(), (QStringList() << testDir)),
                 qPrintable("actual:\n - " + QApplication::libraryPaths().join("\n - ") +
                            "\nexpected:\n - " + testDir));
    }
    {
#ifdef QT_TST_QAPP_DEBUG
        qDebug() << "Initial library path:" << QApplication::libraryPaths();
#endif

        int count = QApplication::libraryPaths().count();
#if 0
        // this test doesn't work if KDE 4 is installed
        QCOMPARE(count, 1); // before creating QApplication, only the PluginsPath is in the libraryPaths()
#endif
        QString installPathPlugins =  QLibraryInfo::location(QLibraryInfo::PluginsPath);
        QApplication::addLibraryPath(installPathPlugins);
#ifdef QT_TST_QAPP_DEBUG
        qDebug() << "installPathPlugins" << installPathPlugins;
        qDebug() << "After adding plugins path:" << QApplication::libraryPaths();
#endif
        QCOMPARE(QApplication::libraryPaths().count(), count);
        QString testDir = QFileInfo(QFINDTESTDATA("test/test.pro")).absolutePath();
        QApplication::addLibraryPath(testDir);
        QCOMPARE(QApplication::libraryPaths().count(), count + 1);

        // creating QApplication adds the applicationDirPath to the libraryPath
        int argc = 1;
        QApplication app(argc, &argv0);
        QString appDirPath = app.applicationDirPath();
        qDebug() << QApplication::libraryPaths();
        // On Windows CE these are identical and might also be the case for other
        // systems too
        if (appDirPath != installPathPlugins)
            QCOMPARE(QApplication::libraryPaths().count(), count + 2);
    }
    {
        int argc = 1;
        QApplication app(argc, &argv0);

#ifdef QT_TST_QAPP_DEBUG
        qDebug() << "Initial library path:" << app.libraryPaths();
#endif
        int count = app.libraryPaths().count();
        QString installPathPlugins =  QLibraryInfo::location(QLibraryInfo::PluginsPath);
        app.addLibraryPath(installPathPlugins);
#ifdef QT_TST_QAPP_DEBUG
        qDebug() << "installPathPlugins" << installPathPlugins;
        qDebug() << "After adding plugins path:" << app.libraryPaths();
#endif
        QCOMPARE(app.libraryPaths().count(), count);

        QString appDirPath = app.applicationDirPath();

        app.addLibraryPath(appDirPath);
#ifdef Q_OS_WINCE
        app.addLibraryPath(appDirPath + "/../..");
#else
        app.addLibraryPath(appDirPath + "/..");
#endif
#ifdef QT_TST_QAPP_DEBUG
        qDebug() << "appDirPath" << appDirPath;
        qDebug() << "After adding appDirPath && appDirPath + /..:" << app.libraryPaths();
#endif
        QCOMPARE(app.libraryPaths().count(), count + 1);
#ifdef Q_OS_MAC
        app.addLibraryPath(appDirPath + "/../MacOS");
#else
        app.addLibraryPath(appDirPath + "/tmp/..");
#endif
#ifdef QT_TST_QAPP_DEBUG
        qDebug() << "After adding appDirPath + /tmp/..:" << app.libraryPaths();
#endif
        QCOMPARE(app.libraryPaths().count(), count + 1);
    }
}

void tst_QApplication::libraryPaths_qt_plugin_path()
{
    int argc = 1;

    QApplication app(argc, &argv0);
    QString appDirPath = app.applicationDirPath();

    // Our hook into libraryPaths() initialization: Set the QT_PLUGIN_PATH environment variable
    QString installPathPluginsDeCanon = appDirPath + QString::fromLatin1("/tmp/..");
    QByteArray ascii = QFile::encodeName(installPathPluginsDeCanon);
    qputenv("QT_PLUGIN_PATH", ascii);

    QVERIFY(!app.libraryPaths().contains(appDirPath + QString::fromLatin1("/tmp/..")));
}

void tst_QApplication::libraryPaths_qt_plugin_path_2()
{
#ifdef Q_OS_UNIX
    QByteArray validPath = QDir("/tmp").canonicalPath().toLatin1();
    QByteArray nonExistentPath = "/nonexistent";
    QByteArray pluginPath = validPath + ":" + nonExistentPath;
#elif defined(Q_OS_WIN)
# ifdef Q_OS_WINCE
    QByteArray validPath = "/Temp";
    QByteArray nonExistentPath = "/nonexistent";
    QByteArray pluginPath = validPath + ";" + nonExistentPath;
# else
    QByteArray validPath = "C:\\windows";
    QByteArray nonExistentPath = "Z:\\nonexistent";
    QByteArray pluginPath = validPath + ";" + nonExistentPath;
# endif
#endif

    {
        // Our hook into libraryPaths() initialization: Set the QT_PLUGIN_PATH environment variable
        qputenv("QT_PLUGIN_PATH", pluginPath);

        int argc = 1;

        QApplication app(argc, &argv0);

        // library path list should contain the default plus the one valid path
        QStringList expected =
            QStringList()
            << QLibraryInfo::location(QLibraryInfo::PluginsPath)
            << QDir(app.applicationDirPath()).canonicalPath()
            << QDir(QDir::fromNativeSeparators(QString::fromLatin1(validPath))).canonicalPath();
# ifdef Q_OS_WINCE
        expected = QSet<QString>::fromList(expected).toList();
# endif
        QVERIFY2(isPathListIncluded(app.libraryPaths(), expected),
                 qPrintable("actual:\n - " + app.libraryPaths().join("\n - ") +
                            "\nexpected:\n - " + expected.join("\n - ")));
    }

    {
        int argc = 1;

        QApplication app(argc, &argv0);

        // library paths are initialized by the QApplication, setting
        // the environment variable here doesn't work
        qputenv("QT_PLUGIN_PATH", pluginPath);

        // library path list should contain the default
        QStringList expected =
            QStringList()
            << QLibraryInfo::location(QLibraryInfo::PluginsPath)
            << app.applicationDirPath();
# ifdef Q_OS_WINCE
        expected = QSet<QString>::fromList(expected).toList();
# endif
        QVERIFY(isPathListIncluded(app.libraryPaths(), expected));

        qputenv("QT_PLUGIN_PATH", QByteArray());
    }
}

class SendPostedEventsTester : public QObject
{
    Q_OBJECT
public:
    QList<int> eventSpy;
    bool event(QEvent *e);
private slots:
    void doTest();
};

bool SendPostedEventsTester::event(QEvent *e)
{
    eventSpy.append(e->type());
    return QObject::event(e);
}

void SendPostedEventsTester::doTest()
{
    QPointer<SendPostedEventsTester> p = this;
    QApplication::postEvent(this, new QEvent(QEvent::User));
    // DeferredDelete should not be delivered until returning from this function
    QApplication::postEvent(this, new QDeferredDeleteEvent());

    QEventLoop eventLoop;
    QMetaObject::invokeMethod(&eventLoop, "quit", Qt::QueuedConnection);
    eventLoop.exec();
    QVERIFY(p != 0);

    QCOMPARE(eventSpy.count(), 2);
    QCOMPARE(eventSpy.at(0), int(QEvent::MetaCall));
    QCOMPARE(eventSpy.at(1), int(QEvent::User));
    eventSpy.clear();
}

void tst_QApplication::sendPostedEvents()
{
    int argc = 0;
    QApplication app(argc, 0);
    SendPostedEventsTester *tester = new SendPostedEventsTester;
    QMetaObject::invokeMethod(tester, "doTest", Qt::QueuedConnection);
    QMetaObject::invokeMethod(&app, "quit", Qt::QueuedConnection);
    QPointer<SendPostedEventsTester> p = tester;
    (void) app.exec();
    QVERIFY(p == 0);
}

void tst_QApplication::thread()
{
    QThread *currentThread = QThread::currentThread();
    // no app, but still have a valid thread
    QVERIFY(currentThread != 0);

    // the thread should be running and not finished
    QVERIFY(currentThread->isRunning());
    QVERIFY(!currentThread->isFinished());

    // this should probably be in the tst_QObject::thread() test, but
    // we put it here since we want to make sure that objects created
    // *before* the QApplication has a thread
    QObject object;
    QObject child(&object);
    QVERIFY(object.thread() == currentThread);
    QVERIFY(child.thread() == currentThread);

    {
        int argc = 0;
        QApplication app(argc, 0);

        // current thread still valid
        QVERIFY(QThread::currentThread() != 0);
        // thread should be the same as before
        QCOMPARE(QThread::currentThread(), currentThread);

        // app's thread should be the current thread
        QCOMPARE(app.thread(), currentThread);

        // the thread should still be running and not finished
        QVERIFY(currentThread->isRunning());
        QVERIFY(!currentThread->isFinished());

        QTestEventLoop::instance().enterLoop(1);
    }

    // app dead, current thread still valid
    QVERIFY(QThread::currentThread() != 0);
    QCOMPARE(QThread::currentThread(), currentThread);

    // the thread should still be running and not finished
    QVERIFY(currentThread->isRunning());
    QVERIFY(!currentThread->isFinished());

    // should still have a thread
    QVERIFY(object.thread() == currentThread);
    QVERIFY(child.thread() == currentThread);

    // do the test again, making sure that the thread is the same as
    // before
    {
        int argc = 0;
        QApplication app(argc, 0);

        // current thread still valid
        QVERIFY(QThread::currentThread() != 0);
        // thread should be the same as before
        QCOMPARE(QThread::currentThread(), currentThread);

        // app's thread should be the current thread
        QCOMPARE(app.thread(), currentThread);

        // the thread should be running and not finished
        QVERIFY(currentThread->isRunning());
        QVERIFY(!currentThread->isFinished());

        // should still have a thread
        QVERIFY(object.thread() == currentThread);
        QVERIFY(child.thread() == currentThread);

        QTestEventLoop::instance().enterLoop(1);
    }

    // app dead, current thread still valid
    QVERIFY(QThread::currentThread() != 0);
    QCOMPARE(QThread::currentThread(), currentThread);

    // the thread should still be running and not finished
    QVERIFY(currentThread->isRunning());
    QVERIFY(!currentThread->isFinished());

    // should still have a thread
    QVERIFY(object.thread() == currentThread);
    QVERIFY(child.thread() == currentThread);
}

class DeleteLaterWidget : public QWidget
{
    Q_OBJECT
public:
    DeleteLaterWidget(QApplication *_app, QWidget *parent = 0)
        : QWidget(parent) { app = _app; child_deleted = false; }

    bool child_deleted;
    QApplication *app;

public slots:
    void runTest();
    void checkDeleteLater();
    void childDeleted() { child_deleted = true; }
};


void DeleteLaterWidget::runTest()
{
    QObject *stillAlive = this->findChild<QObject*>("deleteLater");

    QWidget *w = new QWidget(this);
    connect(w, SIGNAL(destroyed()), this, SLOT(childDeleted()));

    w->deleteLater();
    QVERIFY(!child_deleted);

    QDialog dlg;
    QTimer::singleShot(500, &dlg, SLOT(reject()));
    dlg.exec();

    QVERIFY(!child_deleted);
    app->processEvents();
    QVERIFY(!child_deleted);

    QTimer::singleShot(500, this, SLOT(checkDeleteLater()));

    app->processEvents();

    QVERIFY(!stillAlive); // verify at the end to make test terminate
}

void DeleteLaterWidget::checkDeleteLater()
{
    QVERIFY(child_deleted);

    close();
}

void tst_QApplication::testDeleteLater()
{
#ifdef Q_OS_MAC
    QSKIP("This test fails and then hangs on Mac OS X, see QTBUG-24318");
#endif
    int argc = 0;
    QApplication app(argc, 0);
    connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

    DeleteLaterWidget *wgt = new DeleteLaterWidget(&app);
    QTimer::singleShot(500, wgt, SLOT(runTest()));

    QObject *object = new QObject(wgt);
    object->setObjectName("deleteLater");
    object->deleteLater();

    QObject *stillAlive = wgt->findChild<QObject*>("deleteLater");
    QVERIFY(stillAlive);

    app.exec();

    delete wgt;

}

class EventLoopNester : public QObject
{
    Q_OBJECT
public slots:
    void deleteLaterAndEnterLoop()
    {
        QEventLoop eventLoop;
        QPointer<QObject> p(this);
        deleteLater();
        /*
          DeferredDelete events are compressed, meaning this second
          deleteLater() will *not* delete the object in the nested
          event loop
        */
        QMetaObject::invokeMethod(this, "deleteLater", Qt::QueuedConnection);
        QTimer::singleShot(1000, &eventLoop, SLOT(quit()));
        eventLoop.exec();
        QVERIFY(p);
    }
    void deleteLaterAndExitLoop()
    {
        // Check that 'p' is not deleted before exec returns, since the call
        // to QEventLoop::quit() should stop 'eventLoop' from processing
        // any more events (that is, delete later) until we return to the
        // _current_ event loop:
        QEventLoop eventLoop;
        QPointer<QObject> p(this);
        QMetaObject::invokeMethod(this, "deleteLater", Qt::QueuedConnection);
        QMetaObject::invokeMethod(&eventLoop, "quit", Qt::QueuedConnection);
        eventLoop.exec();
        QVERIFY(p); // not dead yet
    }

    void processEventsOnly()
    {
        QApplication::processEvents();
    }
    void sendPostedEventsWithDeferredDelete()
    {
        QApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    }

    void deleteLaterAndProcessEvents()
    {
        QEventLoop eventLoop;

        QPointer<QObject> p = this;
        deleteLater();

        // trying to delete this object in a deeper eventloop just won't work
        QMetaObject::invokeMethod(this,
                                  "processEventsOnly",
                                  Qt::QueuedConnection);
        QMetaObject::invokeMethod(&eventLoop, "quit", Qt::QueuedConnection);
        eventLoop.exec();
        QVERIFY(p);
        QMetaObject::invokeMethod(this,
                                  "sendPostedEventsWithDeferredDelete",
                                  Qt::QueuedConnection);
        QMetaObject::invokeMethod(&eventLoop, "quit", Qt::QueuedConnection);
        eventLoop.exec();
        QVERIFY(p);

        // trying to delete it from this eventloop still doesn't work
        QApplication::processEvents();
        QVERIFY(p);

        // however, it *will* work with this magic incantation
        QApplication::sendPostedEvents(0, QEvent::DeferredDelete);
        QVERIFY(!p);
    }
};

void tst_QApplication::testDeleteLaterProcessEvents()
{
    int argc = 0;

    // Calling processEvents() with no event dispatcher does nothing.
    QObject *object = new QObject;
    QPointer<QObject> p(object);
    object->deleteLater();
    QApplication::processEvents();
    QVERIFY(p);
    delete object;

    {
        QApplication app(argc, 0);
        // If you call processEvents() with an event dispatcher present, but
        // outside any event loops, deferred deletes are not processed unless
        // sendPostedEvents(0, DeferredDelete) is called.
        object = new QObject;
        p = object;
        object->deleteLater();
        app.processEvents();
        QVERIFY(p);
        QApplication::sendPostedEvents(0, QEvent::DeferredDelete);
        QVERIFY(!p);

        // If you call deleteLater() on an object when there is no parent
        // event loop, and then enter an event loop, the object will get
        // deleted.
        object = new QObject;
        p = object;
        object->deleteLater();
        QEventLoop loop;
        QTimer::singleShot(1000, &loop, SLOT(quit()));
        loop.exec();
        QVERIFY(!p);
    }
    {
        // When an object is in an event loop, then calls deleteLater() and enters
        // an event loop recursively, it should not die until the parent event
        // loop continues.
        QApplication app(argc, 0);
        QEventLoop loop;
        EventLoopNester *nester = new EventLoopNester;
        p = nester;
        QTimer::singleShot(3000, &loop, SLOT(quit()));
        QTimer::singleShot(0, nester, SLOT(deleteLaterAndEnterLoop()));

        loop.exec();
        QVERIFY(!p);
    }

    {
        // When the event loop that calls deleteLater() is exited
        // immediately, the object should die when returning to the
        // parent event loop
        QApplication app(argc, 0);
        QEventLoop loop;
        EventLoopNester *nester = new EventLoopNester;
        p = nester;
        QTimer::singleShot(3000, &loop, SLOT(quit()));
        QTimer::singleShot(0, nester, SLOT(deleteLaterAndExitLoop()));

        loop.exec();
        QVERIFY(!p);
    }

    {
        // when the event loop that calls deleteLater() also calls
        // processEvents() immediately afterwards, the object should
        // not die until the parent loop continues
        QApplication app(argc, 0);
        QEventLoop loop;
        EventLoopNester *nester = new EventLoopNester();
        p = nester;
        QTimer::singleShot(3000, &loop, SLOT(quit()));
        QTimer::singleShot(0, nester, SLOT(deleteLaterAndProcessEvents()));

        loop.exec();
        QVERIFY(!p);
    }
}

/*
    Test for crash with QApplication::setDesktopSettingsAware(false).
*/
void tst_QApplication::desktopSettingsAware()
{
#ifndef QT_NO_PROCESS
    QString path;
    {
        // We need an application object for QFINDTESTDATA to work
        // properly in all cases.
        int argc = 0;
        QCoreApplication app(argc, 0);
        path = QFINDTESTDATA("desktopsettingsaware/");
    }
    QVERIFY2(!path.isEmpty(), "Cannot locate desktopsettingsaware helper application");
    path += "desktopsettingsaware";
#ifdef Q_OS_WINCE
    int argc = 0;
    QApplication tmpApp(argc, 0);
#endif
    QProcess testProcess;
    testProcess.start(path);
    QVERIFY2(testProcess.waitForStarted(),
             qPrintable(QString::fromLatin1("Cannot start '%1': %2").arg(path, testProcess.errorString())));
    QVERIFY(testProcess.waitForFinished(10000));
    QCOMPARE(int(testProcess.state()), int(QProcess::NotRunning));
    QVERIFY(int(testProcess.error()) != int(QProcess::Crashed));
#endif
}

void tst_QApplication::setActiveWindow()
{
    int argc = 0;
    QApplication MyApp(argc, 0);

    QWidget* w = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(w);

    QLineEdit* pb1 = new QLineEdit("Testbutton1", w);
    QLineEdit* pb2 = new QLineEdit("Test Line Edit", w);

    layout->addWidget(pb1);
    layout->addWidget(pb2);

    pb2->setFocus();
    pb2->setParent(0);
    delete pb2;

    w->show();
    QApplication::setActiveWindow(w); // needs this on twm (focus follows mouse)
    QVERIFY(pb1->hasFocus());
    delete w;
}


/* This might fail on some X11 window managers? */
void tst_QApplication::focusChanged()
{
    int argc = 0;
    QApplication app(argc, 0);

    QSignalSpy spy(&app, SIGNAL(focusChanged(QWidget*,QWidget*)));
    QWidget *now = 0;
    QWidget *old = 0;

    QWidget parent1;
    QHBoxLayout hbox1(&parent1);
    QLabel lb1(&parent1);
    QLineEdit le1(&parent1);
    QPushButton pb1(&parent1);
    hbox1.addWidget(&lb1);
    hbox1.addWidget(&le1);
    hbox1.addWidget(&pb1);

    QCOMPARE(spy.count(), 0);

    parent1.show();
    QApplication::setActiveWindow(&parent1); // needs this on twm (focus follows mouse)
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).count(), 2);
    old = qvariant_cast<QWidget*>(spy.at(0).at(0));
    now = qvariant_cast<QWidget*>(spy.at(0).at(1));
    QVERIFY(now == &le1);
    QVERIFY(now == QApplication::focusWidget());
    QVERIFY(old == 0);
    spy.clear();
    QCOMPARE(spy.count(), 0);

    pb1.setFocus();
    QCOMPARE(spy.count(), 1);
    old = qvariant_cast<QWidget*>(spy.at(0).at(0));
    now = qvariant_cast<QWidget*>(spy.at(0).at(1));
    QVERIFY(now == &pb1);
    QVERIFY(now == QApplication::focusWidget());
    QVERIFY(old == &le1);
    spy.clear();

    lb1.setFocus();
    QCOMPARE(spy.count(), 1);
    old = qvariant_cast<QWidget*>(spy.at(0).at(0));
    now = qvariant_cast<QWidget*>(spy.at(0).at(1));
    QVERIFY(now == &lb1);
    QVERIFY(now == QApplication::focusWidget());
    QVERIFY(old == &pb1);
    spy.clear();

    lb1.clearFocus();
    QCOMPARE(spy.count(), 1);
    old = qvariant_cast<QWidget*>(spy.at(0).at(0));
    now = qvariant_cast<QWidget*>(spy.at(0).at(1));
    QVERIFY(now == 0);
    QVERIFY(now == QApplication::focusWidget());
    QVERIFY(old == &lb1);
    spy.clear();

    QWidget parent2;
    QHBoxLayout hbox2(&parent2);
    QLabel lb2(&parent2);
    QLineEdit le2(&parent2);
    QPushButton pb2(&parent2);
    hbox2.addWidget(&lb2);
    hbox2.addWidget(&le2);
    hbox2.addWidget(&pb2);

    parent2.show();
    QApplication::setActiveWindow(&parent2); // needs this on twm (focus follows mouse)
    QVERIFY(spy.count() > 0); // one for deactivation, one for activation on Windows
    old = qvariant_cast<QWidget*>(spy.at(spy.count()-1).at(0));
    now = qvariant_cast<QWidget*>(spy.at(spy.count()-1).at(1));
    QVERIFY(now == &le2);
    QVERIFY(now == QApplication::focusWidget());
    QVERIFY(old == 0);
    spy.clear();

    QTestKeyEvent tab(QTest::Press, Qt::Key_Tab, 0, 0);
    QTestKeyEvent backtab(QTest::Press, Qt::Key_Backtab, 0, 0);
    QTestMouseEvent click(QTest::MouseClick, Qt::LeftButton, 0, QPoint(5, 5), 0);

    bool tabAllControls = true;
#ifdef Q_OS_MAC
    // Mac has two modes, one where you tab to everything, one where you can
    // only tab to input controls, here's what we get. Determine which ones we
    // should get.
    QSettings appleSettings(QLatin1String("apple.com"));
    QVariant appleValue = appleSettings.value(QLatin1String("AppleKeyboardUIMode"), 0);
    tabAllControls = (appleValue.toInt() & 0x2);
#endif

    // make sure Qt's idea of tabbing between widgets matches what we think it should
    QCOMPARE(qt_tab_all_widgets(), tabAllControls);

    tab.simulate(now);
    if (!tabAllControls) {
        QVERIFY(spy.count() == 0);
        QVERIFY(now == QApplication::focusWidget());
    } else {
        QVERIFY(spy.count() > 0);
        old = qvariant_cast<QWidget*>(spy.at(0).at(0));
        now = qvariant_cast<QWidget*>(spy.at(0).at(1));
        QVERIFY(now == &pb2);
        QVERIFY(now == QApplication::focusWidget());
        QVERIFY(old == &le2);
        spy.clear();
    }

    if (!tabAllControls) {
        QVERIFY(spy.count() == 0);
        QVERIFY(now == QApplication::focusWidget());
    } else {
        tab.simulate(now);
        QVERIFY(spy.count() > 0);
        old = qvariant_cast<QWidget*>(spy.at(0).at(0));
        now = qvariant_cast<QWidget*>(spy.at(0).at(1));
        QVERIFY(now == &le2);
        QVERIFY(now == QApplication::focusWidget());
        QVERIFY(old == &pb2);
        spy.clear();
    }

    if (!tabAllControls) {
        QVERIFY(spy.count() == 0);
        QVERIFY(now == QApplication::focusWidget());
    } else {
        backtab.simulate(now);
        QVERIFY(spy.count() > 0);
        old = qvariant_cast<QWidget*>(spy.at(0).at(0));
        now = qvariant_cast<QWidget*>(spy.at(0).at(1));
        QVERIFY(now == &pb2);
        QVERIFY(now == QApplication::focusWidget());
        QVERIFY(old == &le2);
        spy.clear();
    }


    if (!tabAllControls) {
        QVERIFY(spy.count() == 0);
        QVERIFY(now == QApplication::focusWidget());
        old = &pb2;
    } else {
        backtab.simulate(now);
        QVERIFY(spy.count() > 0);
        old = qvariant_cast<QWidget*>(spy.at(0).at(0));
        now = qvariant_cast<QWidget*>(spy.at(0).at(1));
        QVERIFY(now == &le2);
        QVERIFY(now == QApplication::focusWidget());
        QVERIFY(old == &pb2);
        spy.clear();
    }

    click.simulate(old);
    if (!(pb2.focusPolicy() & Qt::ClickFocus)) {
        QVERIFY(spy.count() == 0);
        QVERIFY(now == QApplication::focusWidget());
    } else {
        QVERIFY(spy.count() > 0);
        old = qvariant_cast<QWidget*>(spy.at(0).at(0));
        now = qvariant_cast<QWidget*>(spy.at(0).at(1));
        QVERIFY(now == &pb2);
        QVERIFY(now == QApplication::focusWidget());
        QVERIFY(old == &le2);
        spy.clear();

        click.simulate(old);
        QVERIFY(spy.count() > 0);
        old = qvariant_cast<QWidget*>(spy.at(0).at(0));
        now = qvariant_cast<QWidget*>(spy.at(0).at(1));
        QVERIFY(now == &le2);
        QVERIFY(now == QApplication::focusWidget());
        QVERIFY(old == &pb2);
        spy.clear();
    }

    parent1.activateWindow();
    QApplication::setActiveWindow(&parent1); // needs this on twm (focus follows mouse)
    QVERIFY(spy.count() == 1 || spy.count() == 2); // one for deactivation, one for activation on Windows

    //on windows, the change of focus is made in 2 steps
    //(the focusChanged SIGNAL is emitted twice)
    if (spy.count()==1)
        old = qvariant_cast<QWidget*>(spy.at(spy.count()-1).at(0));
    else
        old = qvariant_cast<QWidget*>(spy.at(spy.count()-2).at(0));
    now = qvariant_cast<QWidget*>(spy.at(spy.count()-1).at(1));
    QVERIFY(now == &le1);
    QVERIFY(now == QApplication::focusWidget());
    QVERIFY(old == &le2);
    spy.clear();
}

class LineEdit : public QLineEdit
{
public:
    LineEdit(QWidget *parent = 0) : QLineEdit(parent) { }

protected:
    void focusOutEvent(QFocusEvent *e) {
        QLineEdit::focusOutEvent(e);
        if (objectName() == "le1")
            setStyleSheet("");
    }

    void focusInEvent(QFocusEvent *e) {
        QLineEdit::focusInEvent(e);
        if (objectName() == "le2")
            setStyleSheet("");
    }
};

void tst_QApplication::focusOut()
{
    int argc = 1;
    QApplication app(argc, &argv0);

    // Tests the case where the style pointer changes when on focus in/out
    // (the above is the case when the stylesheet changes)
    QWidget w;
    QLineEdit *le1 = new LineEdit(&w);
    le1->setObjectName("le1");
    le1->setStyleSheet("background: #fee");
    le1->setFocus();

    QLineEdit *le2 = new LineEdit(&w);
    le2->setObjectName("le2");
    le2->setStyleSheet("background: #fee");
    le2->move(100, 100);
    w.show();

    QTest::qWait(2000);
    le2->setFocus();
    QTest::qWait(2000);
}

void tst_QApplication::execAfterExit()
{
    int argc = 1;
    QApplication app(argc, &argv0);
    QMetaObject::invokeMethod(&app, "quit", Qt::QueuedConnection);
    // this should be ignored, as exec() will reset the exitCode
    QApplication::exit(1);
    int exitCode = app.exec();
    QCOMPARE(exitCode, 0);

    // the quitNow flag should have been reset, so we can spin an
    // eventloop after QApplication::exec() returns
    QEventLoop eventLoop;
    QMetaObject::invokeMethod(&eventLoop, "quit", Qt::QueuedConnection);
    exitCode = eventLoop.exec();
    QCOMPARE(exitCode, 0);
}

void tst_QApplication::wheelScrollLines()
{
    int argc = 1;
    QApplication app(argc, &argv0);
    // If wheelScrollLines returns 0, the mose wheel will be disabled.
    QVERIFY(app.wheelScrollLines() > 0);
}

void tst_QApplication::style()
{
    int argc = 1;

    {
        QApplication app(argc, &argv0);
        QPointer<QStyle> style = app.style();
        app.setStyle(QStyleFactory::create(QLatin1String("Windows")));
        QVERIFY(style.isNull());
    }

    QApplication app(argc, &argv0);

    // qApp style can never be 0
    QVERIFY(QApplication::style() != 0);
}

void tst_QApplication::allWidgets()
{
    int argc = 1;
    QApplication app(argc, &argv0);
    QWidget *w = new QWidget;
    QVERIFY(app.allWidgets().contains(w)); // uncreate widget test
    QVERIFY(app.allWidgets().contains(w)); // created widget test
    delete w;
    w = 0;
    QVERIFY(!app.allWidgets().contains(w)); // removal test
}

void tst_QApplication::topLevelWidgets()
{
    int argc = 1;
    QApplication app(argc, &argv0);
    QWidget *w = new QWidget;
    w->show();
#ifndef QT_NO_CLIPBOARD
    QClipboard *clipboard = QApplication::clipboard();
    QString originalText = clipboard->text();
    clipboard->setText(QString("newText"));
#endif
    app.processEvents();
    QVERIFY(QApplication::topLevelWidgets().contains(w));
    QCOMPARE(QApplication::topLevelWidgets().count(), 1);
    delete w;
    w = 0;
    app.processEvents();
    QCOMPARE(QApplication::topLevelWidgets().count(), 0);
}



void tst_QApplication::setAttribute()
{
    int argc = 1;
    QApplication app(argc, &argv0);
    QVERIFY(!QApplication::testAttribute(Qt::AA_ImmediateWidgetCreation));
    QWidget  *w = new QWidget;
    QVERIFY(!w->testAttribute(Qt::WA_WState_Created));
    delete w;

    QApplication::setAttribute(Qt::AA_ImmediateWidgetCreation);
    QVERIFY(QApplication::testAttribute(Qt::AA_ImmediateWidgetCreation));
    w = new QWidget;
    QVERIFY(w->testAttribute(Qt::WA_WState_Created));
    QWidget *w2 = new QWidget(w);
    w2->setParent(0);
    QVERIFY(w2->testAttribute(Qt::WA_WState_Created));
    delete w;
    delete w2;

    QApplication::setAttribute(Qt::AA_ImmediateWidgetCreation, false);
    QVERIFY(!QApplication::testAttribute(Qt::AA_ImmediateWidgetCreation));
    w = new QWidget;
    QVERIFY(!w->testAttribute(Qt::WA_WState_Created));
    delete w;
}

void tst_QApplication::windowsCommandLine_data()
{
#if defined(Q_OS_WIN)
    QTest::addColumn<QString>("args");
    QTest::addColumn<QString>("expected");

    QTest::newRow("hello world")
        << QString("Hello \"World\"")
        << QString("Hello \"World\"");
    QTest::newRow("sql")
        << QString("SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA = 'PNR' AND TABLE_TYPE = 'VIEW' ORDER BY TABLE_NAME")
        << QString("SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA = 'PNR' AND TABLE_TYPE = 'VIEW' ORDER BY TABLE_NAME");
#endif
}

void tst_QApplication::windowsCommandLine()
{
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
    QFETCH(QString, args);
    QFETCH(QString, expected);

    QProcess testProcess;
    const QString path = QStringLiteral("wincmdline/wincmdline");
    testProcess.start(path, QStringList(args));
    QVERIFY2(testProcess.waitForStarted(),
             qPrintable(QString::fromLatin1("Cannot start '%1': %2").arg(path, testProcess.errorString())));
    QVERIFY(testProcess.waitForFinished(10000));
    QByteArray error = testProcess.readAllStandardError();
    QString procError(error);
    QCOMPARE(procError, expected);
#endif
}

class TouchEventPropagationTestWidget : public QWidget
{
    Q_OBJECT

public:
    bool seenTouchEvent, acceptTouchEvent, seenMouseEvent, acceptMouseEvent;

    TouchEventPropagationTestWidget(QWidget *parent = 0)
        : QWidget(parent), seenTouchEvent(false), acceptTouchEvent(false), seenMouseEvent(false), acceptMouseEvent(false)
    { }

    void reset()
    {
        seenTouchEvent = acceptTouchEvent = seenMouseEvent = acceptMouseEvent = false;
    }

    bool event(QEvent *event)
    {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseMove:
        case QEvent::MouseButtonRelease:
            // qDebug() << objectName() << "seenMouseEvent = true";
            seenMouseEvent = true;
            event->setAccepted(acceptMouseEvent);
            break;
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
            // qDebug() << objectName() << "seenTouchEvent = true";
            seenTouchEvent = true;
            event->setAccepted(acceptTouchEvent);
            break;
        default:
            return QWidget::event(event);
        }
        return true;
    }
};

void tst_QApplication::touchEventPropagation()
{
    int argc = 1;
    QApplication app(argc, &argv0);

    const bool mouseEventSynthesizing = QGuiApplicationPrivate::platformIntegration()
        ->styleHint(QPlatformIntegration::SynthesizeMouseFromTouchEvents).toBool();

    QList<QTouchEvent::TouchPoint> pressedTouchPoints;
    QTouchEvent::TouchPoint press(0);
    press.setState(Qt::TouchPointPressed);
    pressedTouchPoints << press;

    QList<QTouchEvent::TouchPoint> releasedTouchPoints;
    QTouchEvent::TouchPoint release(0);
    release.setState(Qt::TouchPointReleased);
    releasedTouchPoints << release;

    QTouchDevice *device = new QTouchDevice;
    device->setType(QTouchDevice::TouchScreen);
    QWindowSystemInterface::registerTouchDevice(device);

    {
        // touch event behavior on a window
        TouchEventPropagationTestWidget window;
        window.resize(200, 200);
        window.setObjectName("1. window");
        window.show(); // Must have an explicitly specified QWindow for handleTouchEvent,
                       // passing 0 would result in using topLevelAt() which is not ok in this case
                       // as the screen position in the point is bogus.
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        // QPA always takes screen positions and since we map the TouchPoint back to QPA's structure first,
        // we must ensure there is a screen position in the TouchPoint that maps to a local 0, 0.
        pressedTouchPoints[0].setScreenPos(window.mapToGlobal(QPoint(0, 0)));
        releasedTouchPoints[0].setScreenPos(window.mapToGlobal(QPoint(0, 0)));

        QWindowSystemInterface::handleTouchEvent(window.windowHandle(),
                                                 0,
                                                 device,
                                                 touchPointList(pressedTouchPoints));
        QWindowSystemInterface::handleTouchEvent(window.windowHandle(),
                                                 0,
                                                 device,
                                                 touchPointList(releasedTouchPoints));
        QCoreApplication::processEvents();
        QVERIFY(!window.seenTouchEvent);
        QCOMPARE(window.seenMouseEvent, mouseEventSynthesizing); // QApplication may transform ignored touch events in mouse events

        window.reset();
        window.setAttribute(Qt::WA_AcceptTouchEvents);
        QWindowSystemInterface::handleTouchEvent(window.windowHandle(),
                                                 0,
                                                 device,
                                                 touchPointList(pressedTouchPoints));
        QWindowSystemInterface::handleTouchEvent(window.windowHandle(),
                                                 0,
                                                 device,
                                                 touchPointList(releasedTouchPoints));
        QCoreApplication::processEvents();
        QVERIFY(window.seenTouchEvent);
        QCOMPARE(window.seenMouseEvent, mouseEventSynthesizing);

        window.reset();
        window.acceptTouchEvent = true;
        QWindowSystemInterface::handleTouchEvent(window.windowHandle(),
                                                 0,
                                                 device,
                                                 touchPointList(pressedTouchPoints));
        QWindowSystemInterface::handleTouchEvent(window.windowHandle(),
                                                 0,
                                                 device,
                                                 touchPointList(releasedTouchPoints));
        QCoreApplication::processEvents();
        QVERIFY(window.seenTouchEvent);
        QVERIFY(!window.seenMouseEvent);
    }

    {
        // touch event behavior on a window with a child widget
        TouchEventPropagationTestWidget window;
        window.resize(200, 200);
        window.setObjectName("2. window");
        TouchEventPropagationTestWidget widget(&window);
        widget.setObjectName("2. widget");
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        pressedTouchPoints[0].setScreenPos(window.mapToGlobal(QPoint(0, 0)));
        releasedTouchPoints[0].setScreenPos(window.mapToGlobal(QPoint(0, 0)));

        QWindowSystemInterface::handleTouchEvent(window.windowHandle(),
                                                 0,
                                                 device,
                                                 touchPointList(pressedTouchPoints));
        QWindowSystemInterface::handleTouchEvent(window.windowHandle(),
                                                 0,
                                                 device,
                                                 touchPointList(releasedTouchPoints));
        QCoreApplication::processEvents();
        QVERIFY(!widget.seenTouchEvent);
        QCOMPARE(widget.seenMouseEvent, mouseEventSynthesizing);
        QVERIFY(!window.seenTouchEvent);
        QCOMPARE(window.seenMouseEvent, mouseEventSynthesizing);

        window.reset();
        widget.reset();
        widget.setAttribute(Qt::WA_AcceptTouchEvents);
        QWindowSystemInterface::handleTouchEvent(window.windowHandle(),
                                                 0,
                                                 device,
                                                 touchPointList(pressedTouchPoints));
        QWindowSystemInterface::handleTouchEvent(window.windowHandle(),
                                                 0,
                                                 device,
                                                 touchPointList(releasedTouchPoints));
        QCoreApplication::processEvents();
        QVERIFY(widget.seenTouchEvent);
        QCOMPARE(widget.seenMouseEvent, mouseEventSynthesizing);
        QVERIFY(!window.seenTouchEvent);
        QCOMPARE(window.seenMouseEvent, mouseEventSynthesizing);

        window.reset();
        widget.reset();
        widget.acceptMouseEvent = true;
        QWindowSystemInterface::handleTouchEvent(window.windowHandle(),
                                                 0,
                                                 device,
                                                 touchPointList(pressedTouchPoints));
        QWindowSystemInterface::handleTouchEvent(window.windowHandle(),
                                                 0,
                                                 device,
                                                 touchPointList(releasedTouchPoints));
        QCoreApplication::processEvents();
        QVERIFY(widget.seenTouchEvent);
        QCOMPARE(widget.seenMouseEvent, mouseEventSynthesizing);
        QVERIFY(!window.seenTouchEvent);
        QVERIFY(!window.seenMouseEvent);

        window.reset();
        widget.reset();
        widget.acceptTouchEvent = true;
        QWindowSystemInterface::handleTouchEvent(window.windowHandle(),
                                                 0,
                                                 device,
                                                 touchPointList(pressedTouchPoints));
        QWindowSystemInterface::handleTouchEvent(window.windowHandle(),
                                                 0,
                                                 device,
                                                 touchPointList(releasedTouchPoints));
        QCoreApplication::processEvents();
        QVERIFY(widget.seenTouchEvent);
        QVERIFY(!widget.seenMouseEvent);
        QVERIFY(!window.seenTouchEvent);
        QVERIFY(!window.seenMouseEvent);

        window.reset();
        widget.reset();
        widget.setAttribute(Qt::WA_AcceptTouchEvents, false);
        window.setAttribute(Qt::WA_AcceptTouchEvents);
        QWindowSystemInterface::handleTouchEvent(window.windowHandle(),
                                                 0,
                                                 device,
                                                 touchPointList(pressedTouchPoints));
        QWindowSystemInterface::handleTouchEvent(window.windowHandle(),
                                                 0,
                                                 device,
                                                 touchPointList(releasedTouchPoints));
        QCoreApplication::processEvents();
        QVERIFY(!widget.seenTouchEvent);
        QCOMPARE(widget.seenMouseEvent, mouseEventSynthesizing);
        QVERIFY(window.seenTouchEvent);
        QCOMPARE(window.seenMouseEvent, mouseEventSynthesizing);

        window.reset();
        widget.reset();
        window.acceptTouchEvent = true;
        QWindowSystemInterface::handleTouchEvent(window.windowHandle(),
                                                 0,
                                                 device,
                                                 touchPointList(pressedTouchPoints));
        QWindowSystemInterface::handleTouchEvent(window.windowHandle(),
                                                 0,
                                                 device,
                                                 touchPointList(releasedTouchPoints));
        QCoreApplication::processEvents();
        QVERIFY(!widget.seenTouchEvent);
        QCOMPARE(widget.seenMouseEvent, mouseEventSynthesizing);
        QVERIFY(window.seenTouchEvent);
        QVERIFY(!window.seenMouseEvent);

        window.reset();
        widget.reset();
        widget.acceptMouseEvent = true; // it matters, touch events are propagated in parallel to synthesized mouse events
        window.acceptTouchEvent = true;
        QWindowSystemInterface::handleTouchEvent(window.windowHandle(),
                                                 0,
                                                 device,
                                                 touchPointList(pressedTouchPoints));
        QWindowSystemInterface::handleTouchEvent(window.windowHandle(),
                                                 0,
                                                 device,
                                                 touchPointList(releasedTouchPoints));
        QCoreApplication::processEvents();
        QVERIFY(!widget.seenTouchEvent);
        QCOMPARE(widget.seenMouseEvent, mouseEventSynthesizing);
        QCOMPARE(!window.seenTouchEvent, mouseEventSynthesizing);
        QVERIFY(!window.seenMouseEvent);
    }
}

void tst_QApplication::qtbug_12673()
{
    QString path;
    {
        // We need an application object for QFINDTESTDATA to work
        // properly in all cases.
        int argc = 0;
        QCoreApplication app(argc, 0);
        path = QFINDTESTDATA("modal/");
    }
    QVERIFY2(!path.isEmpty(), "Cannot locate modal helper application");
    path += "modal";

#ifndef QT_NO_PROCESS
    QProcess testProcess;
    QStringList arguments;
    testProcess.start(path, arguments);
    QVERIFY2(testProcess.waitForStarted(),
             qPrintable(QString::fromLatin1("Cannot start '%1': %2").arg(path, testProcess.errorString())));
    QVERIFY(testProcess.waitForFinished(20000));
    QCOMPARE(testProcess.exitStatus(), QProcess::NormalExit);
#else
    QSKIP( "No QProcess support", SkipAll);
#endif
}

class NoQuitOnHideWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NoQuitOnHideWidget(QWidget *parent = 0)
      : QWidget(parent)
    {
        QTimer::singleShot(0, this, SLOT(hide()));
        QTimer::singleShot(500, this, SLOT(exitApp()));
    }

private slots:
    void exitApp() {
      qApp->exit(1);
    }
};

void tst_QApplication::noQuitOnHide()
{
    int argc = 0;
    QApplication app(argc, 0);
    QWidget *window1 = new NoQuitOnHideWidget;
    window1->show();
    QCOMPARE(app.exec(), 1);
}

class ShowCloseShowWidget : public QWidget
{
    Q_OBJECT
public:
    ShowCloseShowWidget(bool showAgain, QWidget *parent = 0)
      : QWidget(parent), showAgain(showAgain)
    {
        QTimer::singleShot(0, this, SLOT(doClose()));
        QTimer::singleShot(500, this, SLOT(exitApp()));
    }

private slots:
    void doClose() {
        close();
        if (showAgain)
            show();
    }

    void exitApp() {
      qApp->exit(1);
    }

private:
    bool showAgain;
};

void tst_QApplication::abortQuitOnShow()
{
    int argc = 0;
    QApplication app(argc, 0);
    QWidget *window1 = new ShowCloseShowWidget(false);
    window1->show();
    QCOMPARE(app.exec(), 0);

    QWidget *window2 = new ShowCloseShowWidget(true);
    window2->show();
    QCOMPARE(app.exec(), 1);
}

/*
    This test is meant to ensure that certain objects (public & commonly used)
    can safely be used in a Q_GLOBAL_STATIC such that their destructors are
    executed *after* the destruction of QApplication.
 */
Q_GLOBAL_STATIC(QLocale, tst_qapp_locale);
#ifndef QT_NO_PROCESS
Q_GLOBAL_STATIC(QProcess, tst_qapp_process);
#endif
Q_GLOBAL_STATIC(QFileSystemWatcher, tst_qapp_fileSystemWatcher);
#ifndef QT_NO_SHAREDMEMORY
Q_GLOBAL_STATIC(QSharedMemory, tst_qapp_sharedMemory);
#endif
Q_GLOBAL_STATIC(QElapsedTimer, tst_qapp_elapsedTimer);
Q_GLOBAL_STATIC(QMutex, tst_qapp_mutex);
Q_GLOBAL_STATIC(QWidget, tst_qapp_widget);
Q_GLOBAL_STATIC(QPixmap, tst_qapp_pixmap);
Q_GLOBAL_STATIC(QFont, tst_qapp_font);
Q_GLOBAL_STATIC(QRegion, tst_qapp_region);
Q_GLOBAL_STATIC(QFontDatabase, tst_qapp_fontDatabase);
#ifndef QTEST_NO_CURSOR
Q_GLOBAL_STATIC(QCursor, tst_qapp_cursor);
#endif

void tst_QApplication::globalStaticObjectDestruction()
{
    int argc = 1;
    QApplication app(argc, &argv0);
    QVERIFY(tst_qapp_locale());
#ifndef QT_NO_PROCESS
    QVERIFY(tst_qapp_process());
#endif
    QVERIFY(tst_qapp_fileSystemWatcher());
#ifndef QT_NO_SHAREDMEMORY
    QVERIFY(tst_qapp_sharedMemory());
#endif
    QVERIFY(tst_qapp_elapsedTimer());
    QVERIFY(tst_qapp_mutex());
    QVERIFY(tst_qapp_widget());
    QVERIFY(tst_qapp_pixmap());
    QVERIFY(tst_qapp_font());
    QVERIFY(tst_qapp_region());
    QVERIFY(tst_qapp_fontDatabase());
#ifndef QTEST_NO_CURSOR
    QVERIFY(tst_qapp_cursor());
#endif
}

//QTEST_APPLESS_MAIN(tst_QApplication)
int main(int argc, char *argv[])
{
    tst_QApplication tc;
    argv0 = argv[0];
    return QTest::qExec(&tc, argc, argv);
}

#include "tst_qapplication.moc"
