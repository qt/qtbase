// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#define QT_STATICPLUGIN
#include <QtWidgets/qstyleplugin.h>

#include <qdebug.h>

#include <QTest>
#include <QTimer>
#include <QLibraryInfo>
#include <QSignalSpy>
#include <QFileSystemWatcher>
#include <QSharedMemory>

#include <QtCore/QAbstractEventDispatcher>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#if QT_CONFIG(process)
# include <QtCore/QProcess>
#endif
#include <QtCore/QSettings>
#include <QtCore/private/qeventloop_p.h>

#include <QtGui/QFontDatabase>
#include <QtGui/QClipboard>
#include <QtGui/QStyleHints>

#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QStyleFactory>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/private/qapplication_p.h>
#include <QtWidgets/QStyle>
#include <QtWidgets/qproxystyle.h>
#include <QtWidgets/QTextEdit>

#include <qpa/qwindowsysteminterface.h>
#include <qpa/qwindowsysteminterface_p.h>
#include <private/qevent_p.h>
#include <private/qhighdpiscaling_p.h>

#include "../../../corelib/kernel/qcoreapplication/apphelper.h"
#include "../../../../../shared/androidutils.h"

#include <algorithm>

QT_BEGIN_NAMESPACE

extern bool Q_GUI_EXPORT qt_tab_all_widgets(); // from qapplication.cpp
QT_END_NAMESPACE

class tst_QApplication : public QObject
{
Q_OBJECT

private slots:
    void cleanup();
    void sendEventsOnProcessEvents(); // this must be the first test
    void staticSetup();

    void alert();

    void multiple_data();
    void multiple();

    void nonGui();

    void setFont_data();
    void setFont();
    void setFontForClass();

    void args_data();
    void args();
    void appName();

    void lastWindowClosed();
    void quitOnLastWindowClosed();
    void closeAllWindows();
    void testDeleteLater();
    void testDeleteLaterProcessEvents1();
    void testDeleteLaterProcessEvents2();
    void testDeleteLaterProcessEvents3();
    void testDeleteLaterProcessEvents4();
    void testDeleteLaterProcessEvents5();

#ifdef QT_BUILD_INTERNAL
    void sendPostedEvents();
#endif  // ifdef QT_BUILD_INTERNAL

    void exitFromEventLoop() { QCoreApplicationTestHelper::run(); }
    void exitFromThread() { QCoreApplicationTestHelper::run(); }
    void exitFromThreadedEventLoop() { QCoreApplicationTestHelper::run(); }
    void exitWithPlugins() { QCoreApplicationTestHelper::run(); }
    void mainAppInAThread() { QCoreApplicationTestHelper::run(); }

    void thread();
    void desktopSettingsAware();

    void setActiveWindow();
    void activateDeactivateEvent();

    void focusWidget();
    void focusChanged();
    void focusOut();
    void focusMouseClick();

    void execAfterExit();

#if QT_CONFIG(wheelevent)
    void wheelScrollLines();
#endif

    void task109149();

    void style();
    void applicationPalettePolish();
    void setColorScheme();

    void allWidgets();
    void topLevelWidgets();

    void setAttribute();

    void touchEventPropagation();
    void wheelEventPropagation_data();
    void wheelEventPropagation();

    void modalDialog();
    void qtbug_103611();
    void noQuitOnHide();

    void globalStaticObjectDestruction(); // run this last

    void abortQuitOnShow();

    void staticFunctions();

    void settableStyleHints_data();
    void settableStyleHints();  // Needs to run last as it changes style hints.
};

class EventSpy : public QObject
{
   Q_OBJECT

public:
    QList<int> recordedEvents;
    bool eventFilter(QObject *, QEvent *event) override
    {
        recordedEvents.append(event->type());
        return false;
    }
};

void tst_QApplication::sendEventsOnProcessEvents()
{
    int argc = 0;
    QApplication app(argc, nullptr);

    EventSpy spy;
    app.installEventFilter(&spy);

    QCoreApplication::postEvent(&app,  new QEvent(QEvent::Type(QEvent::User + 1)));
    QCoreApplication::processEvents();

#ifdef Q_OS_LINUX
    if ((QSysInfo::productType() == "rhel" && QSysInfo::productVersion().startsWith(u'9'))
        || (QSysInfo::productType() == "ubuntu" && QSysInfo::productVersion().startsWith(u'2')))
    {
        QFile f("/proc/self/maps");
        QVERIFY(f.open(QIODevice::ReadOnly));

        QByteArray libs = f.readAll();
        if (libs.contains("libqgtk3.") || libs.contains("libqgtk3TestInfix.")) {
            QEXPECT_FAIL("", "Fails if qgtk3 (Glib) is loaded, see QTBUG-87137", Abort);
        }
    }
#endif

    QVERIFY(spy.recordedEvents.contains(QEvent::User + 1));
}


class CloseEventTestWindow : public QWidget
{
public:
    void closeEvent(QCloseEvent *event) override
    {
        QWidget dialog;
        dialog.setWindowTitle(QLatin1String("CloseEventTestWindow"));
        dialog.show();
        dialog.close();

        event->ignore();
    }
};

static  char *argv0;

void tst_QApplication::cleanup()
{
// TODO: Add cleanup code here.
// This will be executed immediately after each test is run.
    QVERIFY(QApplication::topLevelWidgets().isEmpty());
}

void tst_QApplication::staticSetup()
{
    QVERIFY(!qApp);

    QStyle *style = QStyleFactory::create(QLatin1String("Windows"));
    QVERIFY(style);
    QApplication::setStyle(style);

    QPalette pal;
    QApplication::setPalette(pal);
    QFont font;
    QApplication::setFont(font);

    int argc = 0;
    QApplication app(argc, nullptr);

    class EventWatcher : public QObject
    {
    public:
        int palette_changed = 0;
        int font_changed = 0;

        EventWatcher()
        {
            qApp->installEventFilter(this);
#if QT_DEPRECATED_SINCE(6, 0)
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
            QObject::connect(qApp, &QApplication::paletteChanged, [&]{ ++palette_changed; });
            QObject::connect(qApp, &QApplication::fontChanged, [&]{ ++font_changed; });
QT_WARNING_POP
#endif
        }

    protected:
        bool eventFilter(QObject *, QEvent *event) override
        {
            switch (event->type()) {
            case QEvent::ApplicationPaletteChange:
                ++palette_changed;
                break;
            case QEvent::ApplicationFontChange:
                ++font_changed;
                break;
            default:
                break;
            }

            return false;
        }
    };

    EventWatcher watcher;

    QCOMPARE(watcher.palette_changed, 0);
    QCOMPARE(watcher.font_changed, 0);
    qApp->setPalette(QPalette(Qt::red));

    font.setBold(!font.bold());
    qApp->setFont(font);
    QApplication::processEvents();
#if QT_DEPRECATED_SINCE(6, 0)
    QCOMPARE(watcher.palette_changed, 2);
    QCOMPARE(watcher.font_changed, 2);
#else
    QCOMPARE(watcher.palette_changed, 1);
    QCOMPARE(watcher.font_changed, 1);
#endif
}


// QApp subclass that exits the event loop after 150ms
class TestApplication : public QApplication
{
public:
    TestApplication(int &argc, char **argv) : QApplication( argc, argv)
    {
        startTimer(150);
    }

    void timerEvent(QTimerEvent *) override
    {
        quit();
    }
};

void tst_QApplication::alert()
{
    int argc = 0;
    QApplication app(argc, nullptr);
    QApplication::alert(nullptr, 0);

    QWidget widget;
    widget.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    QWidget widget2;
    widget2.setWindowTitle(QLatin1String(QTest::currentTestFunction()) + QLatin1Char('2'));
    QApplication::alert(&widget, 100);
    widget.show();
    widget2.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QVERIFY(QTest::qWaitForWindowExposed(&widget2));
    QApplication::alert(&widget, -1);
    QApplication::alert(&widget, 250);
    widget2.activateWindow();
    QApplication::alert(&widget, 0);
    widget.activateWindow();
    QApplication::alert(&widget, 200);
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
    while (i++ < 5) {
        TestApplication app(argc, nullptr);

        if (features.contains("QFont")) {
            // create font and force loading
            QFont font("Arial", 12);
            QFontInfo finfo(font);
            finfo.exactMatch();
        }
        if (features.contains("QPixmap")) {
            QPixmap pix(100, 100);
            pix.fill(Qt::black);
        }
        if (features.contains("QWidget")) {
            QWidget widget;
        }

        QVERIFY(!QCoreApplication::exec());
    }
}

void tst_QApplication::nonGui()
{
#ifdef Q_OS_HPUX
    // ### This is only to allow us to generate a test report for now.
    QSKIP("This test shuts down the window manager on HP-UX.");
#endif

    int argc = 0;
    QApplication app(argc, nullptr, false);
    QCOMPARE(qApp, &app);
}

void tst_QApplication::setFont_data()
{
    QTest::addColumn<QString>("family");
    QTest::addColumn<int>("pointsize");
    QTest::addColumn<bool>("beforeAppConstructor");

    int argc = 0;
    QApplication app(argc, nullptr); // Needed for QFontDatabase

    const QStringList &families = QFontDatabase::families();
    for (int i = 0, count = qMin(3, families.size()); i < count; ++i) {
        const auto &family = families.at(i);
        const QStringList &styles = QFontDatabase::styles(family);
        if (!styles.isEmpty()) {
            QList<int> sizes = QFontDatabase::pointSizes(family, styles.constFirst());
            if (sizes.isEmpty())
                sizes = QFontDatabase::standardSizes();
            if (!sizes.isEmpty()) {
                const QByteArray name = QByteArrayLiteral("data") + QByteArray::number(i);
                QTest::newRow((name + 'a').constData())
                    << family
                    << sizes.constFirst()
                    << false;
                QTest::newRow((name + 'b').constData())
                    << family
                    << sizes.constFirst()
                    << true;
            }
        }
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
    QApplication app(argc, nullptr);
    if (!beforeAppConstructor)
        QApplication::setFont( font );

    QCOMPARE( app.font(), font );
}

class tstHeaderView : public QHeaderView
{
    Q_OBJECT
public:
    explicit tstHeaderView(Qt::Orientation orientation, QWidget *parent = nullptr)
        : QHeaderView(orientation, parent)
    {}
};
class tstFrame : public QFrame { Q_OBJECT };
class tstWidget : public QWidget { Q_OBJECT };

void tst_QApplication::setFontForClass()
{
    // QTBUG-89910
    // If a default font was not registered for the widget's class,
    // it returns the default font of its nearest registered superclass.
    int argc = 0;
    QApplication app(argc, nullptr);

    QFont font;
    int pointSize = 10;
    const QByteArrayList classNames{"QHeaderView", "QAbstractItemView", "QAbstractScrollView", "QFrame", "QWidget", "QObject"};
    for (auto className : classNames) {
        font.setPointSizeF(pointSize++);
        app.setFont(font, className.constData());
    }

    tstHeaderView headView(Qt::Horizontal);
    tstFrame frame;
    tstWidget widget;

    QFont headViewFont = QApplication::font(&headView);
    QFont frameFont = QApplication::font(&frame);
    QFont widgetFont = QApplication::font(&widget);

    QCOMPARE(headViewFont.pointSize(), QApplication::font("QHeaderView").pointSize());
    QCOMPARE(frameFont.pointSize(), QApplication::font("QFrame").pointSize());
    QCOMPARE(widgetFont.pointSize(), QApplication::font("QWidget").pointSize());
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
    QApplication app(argc, nullptr);
    QApplication::setFont(QFont("helvetica", 100));

    QWidget w;
    w.setWindowTitle("hello");
    w.show();

    QCoreApplication::processEvents();
}

static char **QString2cstrings(const QString &args)
{
    static QByteArrayList cache;

    const auto &list = QStringView{ args }.split(' ');
    auto argarray = new char*[list.size() + 1];

    int i = 0;
    for (; i < list.size(); ++i ) {
        QByteArray l1 = list[i].toLatin1();
        argarray[i] = l1.data();
        cache.append(l1);
    }
    argarray[i] = nullptr;

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
            string += QLatin1Char(' ');
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
    QApplicationPrivate::styleOverride.clear();
}

void tst_QApplication::appName()
{
    char argv0[] = "tst_qapplication";
    char *argv[] = { argv0, nullptr };
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
    void timerEvent(QTimerEvent *) override
    {
        close();
    }

};

void tst_QApplication::lastWindowClosed()
{
    int argc = 0;
    QApplication app(argc, nullptr);

    QSignalSpy spy(&app, &QGuiApplication::lastWindowClosed);

    QPointer<QDialog> dialog = new QDialog;
    dialog->setWindowTitle(QLatin1String(QTest::currentTestFunction()) + QLatin1String("Dialog"));
    QVERIFY(dialog->testAttribute(Qt::WA_QuitOnClose));
    QTimer::singleShot(1000, dialog.data(), &QDialog::accept);
    dialog->exec();
    QVERIFY(dialog);
    QCOMPARE(spy.size(), 0);

    QPointer<CloseWidget>widget = new CloseWidget;
    widget->setWindowTitle(QLatin1String(QTest::currentTestFunction()) + QLatin1String("CloseWidget"));
    QVERIFY(widget->testAttribute(Qt::WA_QuitOnClose));
    widget->show();
    QObject::connect(&app, &QGuiApplication::lastWindowClosed, widget.data(), &QObject::deleteLater);
    QCoreApplication::exec();
    QVERIFY(!widget);
    QCOMPARE(spy.size(), 1);
    spy.clear();

    delete dialog;

    // show 3 windows, close them, should only get lastWindowClosed once
    QWidget w1;
    w1.setWindowTitle(QLatin1String(QTest::currentTestFunction()) + QLatin1Char('1'));
    QWidget w2;
    w1.setWindowTitle(QLatin1String(QTest::currentTestFunction()) + QLatin1Char('2'));
    QWidget w3;
    w1.setWindowTitle(QLatin1String(QTest::currentTestFunction()) + QLatin1Char('3'));
    w1.show();
    w2.show();
    w3.show();

    QTimer::singleShot(1000, &app, &QApplication::closeAllWindows);
    QCoreApplication::exec();
    QCOMPARE(spy.size(), 1);
}

class QuitOnLastWindowClosedDialog : public QDialog
{
    Q_OBJECT
public:
    QuitOnLastWindowClosedDialog()
    {
        QHBoxLayout *hbox = new QHBoxLayout(this);
        m_okButton = new QPushButton("&ok", this);

        hbox->addWidget(m_okButton);
        connect(m_okButton, &QAbstractButton::clicked, this, &QDialog::accept);
        connect(m_okButton, &QAbstractButton::clicked, this, &QuitOnLastWindowClosedDialog::ok_clicked);
    }

public slots:
    void animateOkClick() { m_okButton->animateClick(); }

    void ok_clicked()
    {
        QDialog other;

        QTimer timer;
        connect(&timer, &QTimer::timeout, &other, &QDialog::accept);
        QSignalSpy spy(&timer, &QTimer::timeout);
        QSignalSpy appSpy(qApp, &QGuiApplication::lastWindowClosed);

        timer.start(1000);
        other.exec();

        // verify that the eventloop ran and let the timer fire
        QCOMPARE(spy.size(), 1);
        QCOMPARE(appSpy.size(), 1);
    }

private:
    QPushButton *m_okButton;
};

class QuitOnLastWindowClosedWindow : public QWidget
{
    Q_OBJECT

public:
    QuitOnLastWindowClosedWindow() = default;

public slots:
    void execDialogThenShow()
    {
        QDialog dialog;
        dialog.setWindowTitle(QLatin1String("QuitOnLastWindowClosedWindow Dialog"));
        QTimer timer1;
        connect(&timer1, &QTimer::timeout, &dialog, &QDialog::accept);
        QSignalSpy spy1(&timer1, &QTimer::timeout);
        timer1.setSingleShot(true);
        timer1.start(1000);
        dialog.exec();
        QCOMPARE(spy1.size(), 1);

        show();
    }
};

void tst_QApplication::quitOnLastWindowClosed()
{
    {
        int argc = 0;
        QApplication app(argc, nullptr);

        QuitOnLastWindowClosedDialog d;
        d.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        d.show();
        QTimer::singleShot(1000, &d, &QuitOnLastWindowClosedDialog::animateOkClick);

        QSignalSpy appSpy(&app, &QGuiApplication::lastWindowClosed);
        QCoreApplication::exec();

        // lastWindowClosed() signal should only be sent after the last dialog is closed
        QCOMPARE(appSpy.size(), 2);
    }
    {
        int argc = 0;
        QApplication app(argc, nullptr);
        QSignalSpy appSpy(&app, &QGuiApplication::lastWindowClosed);

        QDialog dialog;
        dialog.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        QTimer timer1;
        connect(&timer1, &QTimer::timeout, &dialog, &QDialog::accept);
        QSignalSpy spy1(&timer1, &QTimer::timeout);
        timer1.setSingleShot(true);
        timer1.start(1000);
        dialog.exec();
        QCOMPARE(spy1.size(), 1);
        QCOMPARE(appSpy.size(), 0);

        QTimer timer2;
        connect(&timer2, &QTimer::timeout, &app, &QCoreApplication::quit);
        QSignalSpy spy2(&timer2, &QTimer::timeout);
        timer2.setSingleShot(true);
        timer2.start(1000);
        int returnValue = QCoreApplication::exec();
        QCOMPARE(returnValue, 0);
        QCOMPARE(spy2.size(), 1);
        QCOMPARE(appSpy.size(), 0);
    }
    {
        int argc = 0;
        QApplication app(argc, nullptr);
        QTimer timer;
        timer.setInterval(100);

        QSignalSpy spy(&app, &QCoreApplication::aboutToQuit);
        QSignalSpy spy2(&timer, &QTimer::timeout);

        QMainWindow mainWindow;
        mainWindow.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        QDialog *dialog = new QDialog(&mainWindow);
        dialog->setWindowTitle(QLatin1String(QTest::currentTestFunction()) + QLatin1String("Dialog"));

        QVERIFY(app.quitOnLastWindowClosed());
        QVERIFY(mainWindow.testAttribute(Qt::WA_QuitOnClose));
        QVERIFY(dialog->testAttribute(Qt::WA_QuitOnClose));

        mainWindow.show();
        QVERIFY(QTest::qWaitForWindowExposed(&mainWindow));
        dialog->show();
        QVERIFY(QTest::qWaitForWindowExposed(dialog));

        timer.start();
        QTimer::singleShot(1000, &mainWindow, &QWidget::close); // This should quit the application
        QTimer::singleShot(2000, &app, &QCoreApplication::quit); // This makes sure we quit even if it didn't

        QCoreApplication::exec();

        QCOMPARE(spy.size(), 1);
        QVERIFY(spy2.size() < 15);      // Should be around 10 if closing caused the quit
    }

    bool quitApplicationTriggered = false;
    auto quitSlot = [&quitApplicationTriggered] () {
        quitApplicationTriggered = true;
        QCoreApplication::exit();
    };

    {
        int argc = 0;
        QApplication app(argc, nullptr);

        QSignalSpy spy(&app, &QCoreApplication::aboutToQuit);

        CloseEventTestWindow mainWindow;
        mainWindow.setWindowTitle(QLatin1String(QTest::currentTestFunction()));

        QVERIFY(app.quitOnLastWindowClosed());
        QVERIFY(mainWindow.testAttribute(Qt::WA_QuitOnClose));

        mainWindow.show();
        QVERIFY(QTest::qWaitForWindowExposed(&mainWindow));

        QTimer::singleShot(1000, &mainWindow, &QWidget::close); // This should NOT quit the application (see CloseEventTestWindow)
        quitApplicationTriggered = false;
        QTimer::singleShot(2000, this, quitSlot); // This actually quits the application.

        QCoreApplication::exec();

        QCOMPARE(spy.size(), 1);
        QVERIFY(quitApplicationTriggered);
    }
    {
        int argc = 0;
        QApplication app(argc, nullptr);
        QSignalSpy appSpy(&app, &QGuiApplication::lastWindowClosed);

        // exec a dialog for 1 second, then show the window
        QuitOnLastWindowClosedWindow window;
        window.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        QTimer::singleShot(0, &window, &QuitOnLastWindowClosedWindow::execDialogThenShow);

        QTimer timer;
        QSignalSpy timerSpy(&timer, &QTimer::timeout);
        connect(&timer, &QTimer::timeout, &window, &QWidget::close);
        timer.setSingleShot(true);
        timer.start(2000);
        int returnValue = QCoreApplication::exec();
        QCOMPARE(returnValue, 0);
        // failure here means the timer above didn't fire, and the
        // quit was caused the dialog being closed (not the window)
        QCOMPARE(timerSpy.size(), 1);
        QCOMPARE(appSpy.size(), 2);
    }
    {
        int argc = 0;
        QApplication app(argc, nullptr);
        QVERIFY(app.quitOnLastWindowClosed());

        QWindow w;
        w.setTitle(QLatin1String(QTest::currentTestFunction()) + QLatin1String("Window"));
        w.show();

        QWidget wid;
        wid.setWindowTitle(QLatin1String(QTest::currentTestFunction()) + QLatin1String("Widget"));
        wid.show();

        QTimer::singleShot(1000, &wid, &QWidget::close); // This should NOT quit the application because the
                                                       // QWindow is still there.
        quitApplicationTriggered = false;
        QTimer::singleShot(2000, this, quitSlot);  // This causes the quit.

        QCoreApplication::exec();

        QVERIFY(quitApplicationTriggered);      // Should be around 20 if closing did not caused the quit
    }
    {   // QTBUG-31569: If the last widget with Qt::WA_QuitOnClose set is closed, other
        // widgets that don't have the attribute set should be closed automatically.
        int argc = 0;
        QApplication app(argc, nullptr);
        QVERIFY(app.quitOnLastWindowClosed());

        QWidget w1;
        w1.setWindowTitle(QLatin1String(QTest::currentTestFunction()) + QLatin1Char('1'));
        w1.show();

        QWidget w2;
        w1.setWindowTitle(QLatin1String(QTest::currentTestFunction()) + QLatin1Char('2'));
        w2.setAttribute(Qt::WA_QuitOnClose, false);
        w2.show();

        QVERIFY(QTest::qWaitForWindowExposed(&w2));

        QTimer timer;
        timer.setInterval(100);
        timer.start();
        QSignalSpy timerSpy(&timer, &QTimer::timeout);

        QTimer::singleShot(100, &w1, &QWidget::close);
        QCoreApplication::exec();

        QVERIFY(timerSpy.size() < 10);
    }
}

static inline bool isVisible(const QWidget *w)
{
    return w->isVisible();
}

class PromptOnCloseWidget : public QWidget
{
public:
    void closeEvent(QCloseEvent *event) override
    {
        QMessageBox *messageBox = new QMessageBox(this);
        messageBox->setWindowTitle("Unsaved data");
        messageBox->setText("Would you like to save or discard your current data?");
        messageBox->setStandardButtons(QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel);
        messageBox->setDefaultButton(QMessageBox::Save);

        messageBox->show();
        QVERIFY(QTest::qWaitForWindowExposed(messageBox));

        // verify that all windows are visible
        const auto &topLevels = QApplication::topLevelWidgets();
        QVERIFY(std::all_of(topLevels.cbegin(), topLevels.cend(), ::isVisible));
        // flush event queue
        QCoreApplication::processEvents();
        // close all windows
        QApplication::closeAllWindows();

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
    QApplication app(argc, nullptr);
    app.setAttribute(Qt::AA_DontUseNativeDialogs, true);

    // create some windows
    new QWidget;
    new QWidget;
    new QWidget;

    // show all windows
    auto topLevels = QApplication::topLevelWidgets();
    for (QWidget *w : std::as_const(topLevels)) {
        w->show();
        QVERIFY(QTest::qWaitForWindowExposed(w));
    }
    // verify that they are visible
    QVERIFY(std::all_of(topLevels.cbegin(), topLevels.cend(), isVisible));
    // empty event queue
    QCoreApplication::processEvents();
    // close all windows
    QApplication::closeAllWindows();
    // all windows should no longer be visible
    QVERIFY(std::all_of(topLevels.cbegin(), topLevels.cend(), [] (const QWidget *w) { return !w->isVisible(); }));

    // add a window that prompts the user when closed
    PromptOnCloseWidget *promptOnCloseWidget = new PromptOnCloseWidget;
    // show all windows
    topLevels = QApplication::topLevelWidgets();
    for (QWidget *w : std::as_const(topLevels)) {
        w->show();
        QVERIFY(QTest::qWaitForWindowExposed(w));
    }
    // close the last window to open the prompt (eventloop recurses)
    promptOnCloseWidget->close();
    // all windows should not be visible, except the one that opened the prompt
    for (QWidget *w : std::as_const(topLevels)) {
        if (w == promptOnCloseWidget)
            QVERIFY(w->isVisible());
        else
            QVERIFY(!w->isVisible());
    }

    qDeleteAll(QApplication::topLevelWidgets());
}

#ifdef QT_BUILD_INTERNAL
class SendPostedEventsTester : public QObject
{
    Q_OBJECT
public:
    QList<int> eventSpy;
    bool event(QEvent *e) override;
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
    deleteLater();

    QEventLoop eventLoop;
    QMetaObject::invokeMethod(&eventLoop, "quit", Qt::QueuedConnection);
    eventLoop.exec();
    QVERIFY(p != nullptr);

    QCOMPARE(eventSpy.size(), 2);
    QCOMPARE(eventSpy.at(0), int(QEvent::MetaCall));
    QCOMPARE(eventSpy.at(1), int(QEvent::User));
    eventSpy.clear();
}

void tst_QApplication::sendPostedEvents()
{
    int argc = 0;
    QApplication app(argc, nullptr);
    SendPostedEventsTester *tester = new SendPostedEventsTester;
    QMetaObject::invokeMethod(tester, "doTest", Qt::QueuedConnection);
    QMetaObject::invokeMethod(&app, "quit", Qt::QueuedConnection);
    QPointer<SendPostedEventsTester> p = tester;
    (void) QCoreApplication::exec();
    QVERIFY(p.isNull());
}
#endif

void tst_QApplication::thread()
{
    QThread *currentThread = QThread::currentThread();
    // no app, but still have a valid thread
    QVERIFY(currentThread != nullptr);

    // the thread should be running and not finished
    QVERIFY(currentThread->isRunning());
    QVERIFY(!currentThread->isFinished());

    // this should probably be in the tst_QObject::thread() test, but
    // we put it here since we want to make sure that objects created
    // *before* the QApplication has a thread
    QObject object;
    QObject child(&object);
    QCOMPARE(object.thread(), currentThread);
    QCOMPARE(child.thread(), currentThread);

    {
        int argc = 0;
        QApplication app(argc, nullptr);

        // current thread still valid
        QVERIFY(QThread::currentThread() != nullptr);
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
    QVERIFY(QThread::currentThread() != nullptr);
    QCOMPARE(QThread::currentThread(), currentThread);

    // the thread should still be running and not finished
    QVERIFY(currentThread->isRunning());
    QVERIFY(!currentThread->isFinished());

    // should still have a thread
    QCOMPARE(object.thread(), currentThread);
    QCOMPARE(child.thread(), currentThread);

    // do the test again, making sure that the thread is the same as
    // before
    {
        int argc = 0;
        QApplication app(argc, nullptr);

        // current thread still valid
        QVERIFY(QThread::currentThread() != nullptr);
        // thread should be the same as before
        QCOMPARE(QThread::currentThread(), currentThread);

        // app's thread should be the current thread
        QCOMPARE(app.thread(), currentThread);

        // the thread should be running and not finished
        QVERIFY(currentThread->isRunning());
        QVERIFY(!currentThread->isFinished());

        // should still have a thread
        QCOMPARE(object.thread(), currentThread);
        QCOMPARE(child.thread(), currentThread);

        QTestEventLoop::instance().enterLoop(1);
    }

    // app dead, current thread still valid
    QVERIFY(QThread::currentThread() != nullptr);
    QCOMPARE(QThread::currentThread(), currentThread);

    // the thread should still be running and not finished
    QVERIFY(currentThread->isRunning());
    QVERIFY(!currentThread->isFinished());

    // should still have a thread
    QCOMPARE(object.thread(), currentThread);
    QCOMPARE(child.thread(), currentThread);
}

class DeleteLaterWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DeleteLaterWidget(QApplication *_app, QWidget *parent = nullptr)
        : QWidget(parent), app(_app) {}

    bool child_deleted = false;
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
    connect(w, &QObject::destroyed, this, &DeleteLaterWidget::childDeleted);

    w->deleteLater();
    QVERIFY(!child_deleted);

    QDialog dlg;
    QTimer::singleShot(500, &dlg, &QDialog::reject);
    dlg.exec();

    QVERIFY(!child_deleted);
    QCoreApplication::processEvents();
    QVERIFY(!child_deleted);

    QTimer::singleShot(500, this, &DeleteLaterWidget::checkDeleteLater);

    QCoreApplication::processEvents();

    // At this point, the event queue is empty. As we want a deferred
    // deletion to occur before the timer event, we should provoke the
    // event dispatcher for the next spin.
    QCoreApplication::eventDispatcher()->interrupt();

    QVERIFY(!stillAlive); // verify at the end to make test terminate
}

void DeleteLaterWidget::checkDeleteLater()
{
    QVERIFY(child_deleted);

    close();
}

void tst_QApplication::testDeleteLater()
{
    int argc = 0;
    QApplication app(argc, nullptr);
    connect(&app, &QGuiApplication::lastWindowClosed, &app, &QCoreApplication::quit);

    DeleteLaterWidget *wgt = new DeleteLaterWidget(&app);
    QTimer::singleShot(500, wgt, &DeleteLaterWidget::runTest);

    QObject *object = new QObject(wgt);
    object->setObjectName("deleteLater");
    object->deleteLater();

    QObject *stillAlive = wgt->findChild<QObject*>("deleteLater");
    QVERIFY(stillAlive);

    wgt->show();
    QCoreApplication::exec();

    QVERIFY(wgt->isHidden());
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
        QTimer::singleShot(1000, &eventLoop, &QEventLoop::quit);
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
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
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
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(!p);
    }
};

void tst_QApplication::testDeleteLaterProcessEvents1()
{
    // Calling processEvents() with no event dispatcher does nothing.
    QObject *object = new QObject;
    QPointer<QObject> p(object);
    object->deleteLater();
    QApplication::processEvents();
    QVERIFY(p);
    delete object;
}

void tst_QApplication::testDeleteLaterProcessEvents2()
{
    int argc = 0;
    QApplication app(argc, nullptr);
    // If you call processEvents() with an event dispatcher present, but
    // outside any event loops, deferred deletes are not processed unless
    // sendPostedEvents(0, DeferredDelete) is called.
    auto object = new QObject;
    QPointer<QObject> p(object);
    object->deleteLater();
    QCoreApplication::processEvents();
    QVERIFY(p);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!p);

    // If you call deleteLater() on an object when there is no parent
    // event loop, and then enter an event loop, the object will get
    // deleted.
    QEventLoop loop;
    object = new QObject;
    connect(object, &QObject::destroyed, &loop, &QEventLoop::quit);
    p = object;
    object->deleteLater();
    QTimer::singleShot(1000, &loop, &QEventLoop::quit);
    loop.exec();
    QVERIFY(!p);
}

void tst_QApplication::testDeleteLaterProcessEvents3()
{
    int argc = 0;
    // When an object is in an event loop, then calls deleteLater() and enters
    // an event loop recursively, it should not die until the parent event
    // loop continues.
    QApplication app(argc, nullptr);
    QEventLoop loop;
    EventLoopNester *nester = new EventLoopNester;
    QPointer<QObject> p(nester);
    QTimer::singleShot(3000, &loop, &QEventLoop::quit);
    QTimer::singleShot(0, nester, &EventLoopNester::deleteLaterAndEnterLoop);

    loop.exec();
    QVERIFY(!p);
}

void tst_QApplication::testDeleteLaterProcessEvents4()
{
    int argc = 0;
    // When the event loop that calls deleteLater() is exited
    // immediately, the object should die when returning to the
    // parent event loop
    QApplication app(argc, nullptr);
    QEventLoop loop;
    EventLoopNester *nester = new EventLoopNester;
    QPointer<QObject> p(nester);
    QTimer::singleShot(3000, &loop, &QEventLoop::quit);
    QTimer::singleShot(0, nester, &EventLoopNester::deleteLaterAndExitLoop);

    loop.exec();
    QVERIFY(!p);
}

void tst_QApplication::testDeleteLaterProcessEvents5()
{
    // when the event loop that calls deleteLater() also calls
    // processEvents() immediately afterwards, the object should
    // not die until the parent loop continues
    int argc = 0;
    QApplication app(argc, nullptr);
    QEventLoop loop;
    EventLoopNester *nester = new EventLoopNester();
    QPointer<QObject> p(nester);
    QTimer::singleShot(3000, &loop, &QEventLoop::quit);
    QTimer::singleShot(0, nester, &EventLoopNester::deleteLaterAndProcessEvents);

    loop.exec();
    QVERIFY(!p);
}

/*
    Test for crash with QApplication::setDesktopSettingsAware(false).
*/
void tst_QApplication::desktopSettingsAware()
{
#if QT_CONFIG(process)
    QProcess testProcess;
#ifdef Q_OS_MACOS
    QStringList environment = QProcess::systemEnvironment();
    environment += QLatin1String("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM=1");
    testProcess.setEnvironment(environment);
#endif
    testProcess.start("./desktopsettingsaware_helper");
    QVERIFY2(testProcess.waitForStarted(),
             qPrintable(QString::fromLatin1("Cannot start 'desktopsettingsaware_helper': %1").arg(testProcess.errorString())));
    QVERIFY(testProcess.waitForFinished(10000));
    QCOMPARE(int(testProcess.state()), int(QProcess::NotRunning));
    QVERIFY(int(testProcess.error()) != int(QProcess::Crashed));
#endif
}

void tst_QApplication::setActiveWindow()
{
    int argc = 0;
    QApplication MyApp(argc, nullptr);

    QWidget* w = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(w);

    QLineEdit* pb1 = new QLineEdit("Testbutton1", w);
    QLineEdit* pb2 = new QLineEdit("Test Line Edit", w);

    layout->addWidget(pb1);
    layout->addWidget(pb2);

    pb2->setFocus();
    pb2->setParent(nullptr);
    delete pb2;

    w->show();
    QApplicationPrivate::setActiveWindow(w); // needs this on twm (focus follows mouse)
    QVERIFY(pb1->hasFocus());
    delete w;
}

void tst_QApplication::activateDeactivateEvent()
{
    // Ensure that QWindows (other than QWidgetWindow)
    // are activated / deactivated.
    class Window : public QWindow
    {
    public:
        using QWindow::QWindow;

        int activateCount = 0;
        int deactivateCount = 0;
    protected:
        bool event(QEvent *e) override
        {
            switch (e->type()) {
            case QEvent::WindowActivate:
                ++activateCount;
                break;
            case QEvent::WindowDeactivate:
                ++deactivateCount;
                break;
            default:
                break;
            }
            return QWindow::event(e);
        }
    };

    int argc = 0;
    QApplication app(argc, nullptr);

    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("QWindow::requestActivate() is not supported.");

    Window w1;
    Window w2;

    w1.show();
    w1.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&w1));
    QCOMPARE(w1.activateCount, 1);
    QCOMPARE(w1.deactivateCount, 0);

    w2.show();
    w2.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&w2));
    QCOMPARE(w1.deactivateCount, 1);
    QCOMPARE(w2.activateCount, 1);
}

void tst_QApplication::focusWidget()
{
    int argc = 0;
    QApplication app(argc, nullptr);

    // The focus widget is the active window itself
    {
        QTextEdit te;
        te.show();

        QVERIFY(QTest::qWaitForWindowFocused(&te));

        const auto focusWidget = QApplication::focusWidget();
        QVERIFY(focusWidget);
        QVERIFY(focusWidget->hasFocus());
        QVERIFY(te.hasFocus());
        QCOMPARE(focusWidget, te.focusWidget());
    }

    // The focus widget is a child of the active window
    {
        QWidget w;
        QTextEdit te(&w);
        w.show();

        QVERIFY(QTest::qWaitForWindowFocused(&w));

        const auto focusWidget = QApplication::focusWidget();
        QVERIFY(focusWidget);
        QVERIFY(focusWidget->hasFocus());
        QVERIFY(!w.hasFocus());
        QVERIFY(te.hasFocus());
        QCOMPARE(te.focusWidget(), w.focusWidget());
        QCOMPARE(focusWidget, w.focusWidget());
    }
}

/* This might fail on some X11 window managers? */
void tst_QApplication::focusChanged()
{
    int argc = 0;
    QApplication app(argc, nullptr);

    QSignalSpy spy(&app, QOverload<QWidget*,QWidget*>::of(&QApplication::focusChanged));
    QWidget *now = nullptr;
    QWidget *old = nullptr;

    QWidget parent1;
    parent1.setWindowTitle(QLatin1String(QTest::currentTestFunction()) + QLatin1Char('1'));
    QHBoxLayout hbox1(&parent1);
    QLabel lb1(&parent1);
    QLineEdit le1(&parent1);
    QPushButton pb1(&parent1);
    hbox1.addWidget(&lb1);
    hbox1.addWidget(&le1);
    hbox1.addWidget(&pb1);

    QCOMPARE(spy.size(), 0);

    parent1.show();
    QApplicationPrivate::setActiveWindow(&parent1); // needs this on twm (focus follows mouse)
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.at(0).size(), 2);
    old = qvariant_cast<QWidget*>(spy.at(0).at(0));
    now = qvariant_cast<QWidget*>(spy.at(0).at(1));
    QCOMPARE(now, &le1);
    QCOMPARE(now, QApplication::focusWidget());
    QVERIFY(!old);
    spy.clear();
    QCOMPARE(spy.size(), 0);

    pb1.setFocus();
    QCOMPARE(spy.size(), 1);
    old = qvariant_cast<QWidget*>(spy.at(0).at(0));
    now = qvariant_cast<QWidget*>(spy.at(0).at(1));
    QCOMPARE(now, &pb1);
    QCOMPARE(now, QApplication::focusWidget());
    QCOMPARE(old, &le1);
    spy.clear();

    lb1.setFocus();
    QCOMPARE(spy.size(), 1);
    old = qvariant_cast<QWidget*>(spy.at(0).at(0));
    now = qvariant_cast<QWidget*>(spy.at(0).at(1));
    QCOMPARE(now, &lb1);
    QCOMPARE(now, QApplication::focusWidget());
    QCOMPARE(old, &pb1);
    spy.clear();

    lb1.clearFocus();
    QCOMPARE(spy.size(), 1);
    old = qvariant_cast<QWidget*>(spy.at(0).at(0));
    now = qvariant_cast<QWidget*>(spy.at(0).at(1));
    QVERIFY(!now);
    QCOMPARE(now, QApplication::focusWidget());
    QCOMPARE(old, &lb1);
    spy.clear();

    QWidget parent2;
    parent2.setWindowTitle(QLatin1String(QTest::currentTestFunction()) + QLatin1Char('1'));
    QHBoxLayout hbox2(&parent2);
    QLabel lb2(&parent2);
    QLineEdit le2(&parent2);
    QPushButton pb2(&parent2);
    hbox2.addWidget(&lb2);
    hbox2.addWidget(&le2);
    hbox2.addWidget(&pb2);

    parent2.show();
    QApplicationPrivate::setActiveWindow(&parent2); // needs this on twm (focus follows mouse)
    QVERIFY(spy.size() > 0); // one for deactivation, one for activation on Windows
    old = qvariant_cast<QWidget*>(spy.at(spy.size()-1).at(0));
    now = qvariant_cast<QWidget*>(spy.at(spy.size()-1).at(1));
    QCOMPARE(now, &le2);
    QCOMPARE(now, QApplication::focusWidget());
    QVERIFY(!old);
    spy.clear();

    QTestKeyEvent tab(QTest::Press, Qt::Key_Tab, Qt::KeyboardModifiers(), 0);
    QTestKeyEvent backtab(QTest::Press, Qt::Key_Backtab, Qt::KeyboardModifiers(), 0);
    QTestMouseEvent click(QTest::MouseClick, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(5, 5), 0);

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
        QCOMPARE(spy.size(), 0);
        QCOMPARE(now, QApplication::focusWidget());
    } else {
        QVERIFY(spy.size() > 0);
        old = qvariant_cast<QWidget*>(spy.at(0).at(0));
        now = qvariant_cast<QWidget*>(spy.at(0).at(1));
        QCOMPARE(now, &pb2);
        QCOMPARE(now, QApplication::focusWidget());
        QCOMPARE(old, &le2);
        spy.clear();
    }

    if (!tabAllControls) {
        QCOMPARE(spy.size(), 0);
        QCOMPARE(now, QApplication::focusWidget());
    } else {
        tab.simulate(now);
        QVERIFY(spy.size() > 0);
        old = qvariant_cast<QWidget*>(spy.at(0).at(0));
        now = qvariant_cast<QWidget*>(spy.at(0).at(1));
        QCOMPARE(now, &le2);
        QCOMPARE(now, QApplication::focusWidget());
        QCOMPARE(old, &pb2);
        spy.clear();
    }

    if (!tabAllControls) {
        QCOMPARE(spy.size(), 0);
        QCOMPARE(now, QApplication::focusWidget());
    } else {
        backtab.simulate(now);
        QVERIFY(spy.size() > 0);
        old = qvariant_cast<QWidget*>(spy.at(0).at(0));
        now = qvariant_cast<QWidget*>(spy.at(0).at(1));
        QCOMPARE(now, &pb2);
        QCOMPARE(now, QApplication::focusWidget());
        QCOMPARE(old, &le2);
        spy.clear();
    }


    if (!tabAllControls) {
        QCOMPARE(spy.size(), 0);
        QCOMPARE(now, QApplication::focusWidget());
        old = &pb2;
    } else {
        backtab.simulate(now);
        QVERIFY(spy.size() > 0);
        old = qvariant_cast<QWidget*>(spy.at(0).at(0));
        now = qvariant_cast<QWidget*>(spy.at(0).at(1));
        QCOMPARE(now, &le2);
        QCOMPARE(now, QApplication::focusWidget());
        QCOMPARE(old, &pb2);
        spy.clear();
    }

    click.simulate(old);
    if (!(pb2.focusPolicy() & Qt::ClickFocus)) {
        QCOMPARE(spy.size(), 0);
        QCOMPARE(now, QApplication::focusWidget());
    } else {
        QVERIFY(spy.size() > 0);
        old = qvariant_cast<QWidget*>(spy.at(0).at(0));
        now = qvariant_cast<QWidget*>(spy.at(0).at(1));
        QCOMPARE(now, &pb2);
        QCOMPARE(now, QApplication::focusWidget());
        QCOMPARE(old, &le2);
        spy.clear();

        click.simulate(old);
        QVERIFY(spy.size() > 0);
        old = qvariant_cast<QWidget*>(spy.at(0).at(0));
        now = qvariant_cast<QWidget*>(spy.at(0).at(1));
        QCOMPARE(now, &le2);
        QCOMPARE(now, QApplication::focusWidget());
        QCOMPARE(old, &pb2);
        spy.clear();
    }

    parent1.activateWindow();
    QApplicationPrivate::setActiveWindow(&parent1); // needs this on twm (focus follows mouse)
    QVERIFY(spy.size() == 1 || spy.size() == 2); // one for deactivation, one for activation on Windows

    //on windows, the change of focus is made in 2 steps
    //(the focusChanged SIGNAL is emitted twice)
    if (spy.size()==1)
        old = qvariant_cast<QWidget*>(spy.at(spy.size()-1).at(0));
    else
        old = qvariant_cast<QWidget*>(spy.at(spy.size()-2).at(0));
    now = qvariant_cast<QWidget*>(spy.at(spy.size()-1).at(1));
    QCOMPARE(now, &le1);
    QCOMPARE(now, QApplication::focusWidget());
    QCOMPARE(old, &le2);
    spy.clear();
}

class LineEdit : public QLineEdit
{
public:
    using QLineEdit::QLineEdit;

protected:
    void focusOutEvent(QFocusEvent *e) override
    {
        QLineEdit::focusOutEvent(e);
        if (objectName() == "le1")
            setStyleSheet("");
    }

    void focusInEvent(QFocusEvent *e)  override
    {
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
    w.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    QLineEdit *le1 = new LineEdit(&w);
    le1->setObjectName("le1");
    le1->setStyleSheet("background: #fee");
    le1->setFocus();

    QLineEdit *le2 = new LineEdit(&w);
    le2->setObjectName("le2");
    le2->setStyleSheet("background: #fee");
    le2->move(100, 100);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    QTest::qWait(2000);
    le2->setFocus();
    QTest::qWait(2000);
}

void tst_QApplication::focusMouseClick()
{
    int argc = 1;
    QApplication app(argc, &argv0);

    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("Window activation is not supported");

    QWidget w;
    w.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    w.setFocusPolicy(Qt::StrongFocus);
    QWidget w2(&w);
    w2.setFocusPolicy(Qt::TabFocus);
    w.show();
    w.setFocus();
    QTRY_COMPARE(QApplication::focusWidget(), &w);

    // front most widget has Qt::TabFocus, parent widget accepts clicks as well
    // now send a mouse button press event and check what happens with the focus
    // it should be given to the parent widget
    QMouseEvent ev(QEvent::MouseButtonPress, QPointF(), w.mapToGlobal(QPointF()), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QSpontaneKeyEvent::setSpontaneous(&ev);
    QVERIFY(ev.spontaneous());
    qApp->notify(&w2, &ev);
    QCOMPARE(QApplication::focusWidget(), &w);

    // then we give the inner widget strong focus -> it should get focus
    w2.setFocusPolicy(Qt::StrongFocus);
    QSpontaneKeyEvent::setSpontaneous(&ev);
    QVERIFY(ev.spontaneous());
    qApp->notify(&w2, &ev);
    QTRY_COMPARE(QApplication::focusWidget(), &w2);

    // now back to tab focus and click again (it already had focus) -> focus should stay
    // (focus was revoked as of QTBUG-34042)
    w2.setFocusPolicy(Qt::TabFocus);
    QSpontaneKeyEvent::setSpontaneous(&ev);
    QVERIFY(ev.spontaneous());
    qApp->notify(&w2, &ev);
    QCOMPARE(QApplication::focusWidget(), &w2);
}

void tst_QApplication::execAfterExit()
{
    int argc = 1;
    QApplication app(argc, &argv0);
    QMetaObject::invokeMethod(&app, "quit", Qt::QueuedConnection);
    // this should be ignored, as exec() will reset the exitCode
    QApplication::exit(1);
    int exitCode = QCoreApplication::exec();
    QCOMPARE(exitCode, 0);

    // the quitNow flag should have been reset, so we can spin an
    // eventloop after QApplication::exec() returns
    QEventLoop eventLoop;
    QMetaObject::invokeMethod(&eventLoop, "quit", Qt::QueuedConnection);
    exitCode = eventLoop.exec();
    QCOMPARE(exitCode, 0);
}

#if QT_CONFIG(wheelevent)
void tst_QApplication::wheelScrollLines()
{
    int argc = 1;
    QApplication app(argc, &argv0);
    // If wheelScrollLines returns 0, the mose wheel will be disabled.
    QVERIFY(app.wheelScrollLines() > 0);
}
#endif // QT_CONFIG(wheelevent)

void tst_QApplication::style()
{
    int argc = 1;

    {
        QApplication app(argc, &argv0);
        QPointer<QStyle> style = QApplication::style();
        QApplication::setStyle(QStyleFactory::create(QLatin1String("Windows")));
        QVERIFY(style.isNull());
    }

    QApplication app(argc, &argv0);

    // qApp style can never be 0
    QVERIFY(QApplication::style() != nullptr);
}

class CustomStyle : public QProxyStyle
{
public:
    CustomStyle() : QProxyStyle("Windows") { Q_ASSERT(!polished); }
    ~CustomStyle() { polished = 0; }
    void polish(QPalette &palette) override
    {
        polished++;
        palette.setColor(QPalette::Active, QPalette::Link, Qt::red);
    }
    static int polished;
};

int CustomStyle::polished = 0;

class CustomStylePlugin : public QStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QStyleFactoryInterface" FILE "customstyle.json")
public:
    QStyle *create(const QString &) override { return new CustomStyle; }
};

Q_IMPORT_PLUGIN(CustomStylePlugin)

void tst_QApplication::applicationPalettePolish()
{
    int argc = 1;

#if defined(QT_BUILD_INTERNAL)
    {
        qputenv("QT_DESKTOP_STYLE_KEY", "customstyle");
        QApplication app(argc, &argv0);
        QVERIFY(CustomStyle::polished);
        QVERIFY(!app.palette().resolveMask());
        QCOMPARE(app.palette().color(QPalette::Link), Qt::red);
        qunsetenv("QT_DESKTOP_STYLE_KEY");
    }
#endif

    {
        QApplication::setStyle(new CustomStyle);
        QApplication app(argc, &argv0);
        QVERIFY(CustomStyle::polished);
        QVERIFY(!app.palette().resolveMask());
        QCOMPARE(app.palette().color(QPalette::Link), Qt::red);
    }

    {
        QApplication app(argc, &argv0);
        app.setStyle(new CustomStyle);
        QVERIFY(CustomStyle::polished);
        QVERIFY(!app.palette().resolveMask());
        QCOMPARE(app.palette().color(QPalette::Link), Qt::red);

        CustomStyle::polished = 0;
        app.setPalette(QPalette());
        QVERIFY(CustomStyle::polished);
        QVERIFY(!app.palette().resolveMask());
        QCOMPARE(app.palette().color(QPalette::Link), Qt::red);

        CustomStyle::polished = 0;
        QPalette palette;
        palette.setColor(QPalette::Active, QPalette::Highlight, Qt::green);
        app.setPalette(palette);
        QVERIFY(CustomStyle::polished);
        QVERIFY(app.palette().resolveMask());
        QCOMPARE(app.palette().color(QPalette::Link), Qt::red);
        QCOMPARE(app.palette().color(QPalette::Highlight), Qt::green);
    }
}

void tst_QApplication::setColorScheme()
{
    int argc = 1;
    QApplication app(argc, &argv0);

    if (QStringList{"minimal", "offscreen", "wayland", "xcb", "wasm", "webassembly"}
        .contains(QGuiApplication::platformName(), Qt::CaseInsensitive)) {
        QSKIP("Setting the colorScheme is not implemented on this platform.");
    }
    qDebug() << "Testing setColorScheme on platform" << QGuiApplication::platformName();

    if (QByteArrayView(app.style()->metaObject()->className()) == "QWindowsVistaStyle")
        QSKIP("Setting the colorScheme is not supported with the Windows Vista style.");

    const Qt::ColorScheme defaultColorScheme = QApplication::styleHints()->colorScheme();
    // if we implement setColorScheme, then we must be able to read it
    QVERIFY(defaultColorScheme != Qt::ColorScheme::Unknown);
    const Qt::ColorScheme newColorScheme = defaultColorScheme == Qt::ColorScheme::Light
                                         ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light;

    class TopLevelWidget : public QWidget
    {
        QList<QEvent::Type> events;
    public:
        TopLevelWidget()
        {
            setObjectName("colorScheme TopLevelWidget");
        }

        void clearEvents()
        {
            events.clear();
        }
        qsizetype eventCount(QEvent::Type type) const
        {
            return events.count(type);
        }
    protected:
        bool event(QEvent *event) override
        {
            switch (event->type()) {
            case QEvent::ApplicationPaletteChange:
            case QEvent::PaletteChange:
            case QEvent::ThemeChange:
                events << event->type();
                break;
            default:
                break;
            }

            return QWidget::event(event);
        }
    } topLevelWidget;
    topLevelWidget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevelWidget));

    QSignalSpy colorSchemeChangedSpy(app.styleHints(), &QStyleHints::colorSchemeChanged);

    // always start with a clean list
    topLevelWidget.clearEvents();
    const QPalette defaultPalette = topLevelWidget.palette();

    bool oldPaletteWhenSchemeChanged = false;
    connect(app.styleHints(), &QStyleHints::colorSchemeChanged, this,
            [defaultPalette, &topLevelWidget, &oldPaletteWhenSchemeChanged]{
        oldPaletteWhenSchemeChanged = defaultPalette == topLevelWidget.palette();
    });

    app.styleHints()->setColorScheme(newColorScheme);
    QTRY_COMPARE(colorSchemeChangedSpy.count(), 1);
    // We have not yet updated the palette when we emit the colorSchemeChanged
    // signal, so the toplevel widget should still use the previous palette
    QVERIFY(oldPaletteWhenSchemeChanged);
    QCOMPARE(topLevelWidget.eventCount(QEvent::ThemeChange), 1);
    // We can't guarantee that there is only one ApplicationPaletteChange,
    // and they might arrive asynchronously in response to ThemeChange
    QTRY_COMPARE_GE(topLevelWidget.eventCount(QEvent::ApplicationPaletteChange), 1);
    // But we can guarantee a single PaletteChange event for the widget
    QCOMPARE(topLevelWidget.eventCount(QEvent::PaletteChange), 1);
    // The palette should have changed
    QCOMPARE_NE(topLevelWidget.palette(), defaultPalette);

    topLevelWidget.clearEvents();
    colorSchemeChangedSpy.clear();

    // verify that a widget shown with a color scheme override in place respect that
    QWidget newWidget;
    newWidget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&newWidget));
    QCOMPARE(newWidget.palette(), topLevelWidget.palette());

    // Setting to Unknown should follow the system preference again
    app.styleHints()->setColorScheme(Qt::ColorScheme::Unknown);
    QTRY_COMPARE(colorSchemeChangedSpy.count(), 1);
    QCOMPARE(app.styleHints()->colorScheme(), defaultColorScheme);
    QTRY_COMPARE(topLevelWidget.eventCount(QEvent::PaletteChange), 1);

    auto debugPalette = qScopeGuard([defaultPalette, &topLevelWidget]{
        qDebug() << "Inspecting palettes for differences";
        const QPalette palette = topLevelWidget.palette();
        for (int g = 0; g < QPalette::NColorGroups; ++g) {
            for (int r = 0; r < QPalette::NColorRoles; ++r) {
                const auto group = static_cast<QPalette::ColorGroup>(g);
                const auto role = static_cast<QPalette::ColorRole>(r);
                qDebug() << "...Checking" << group << role;
                const auto actualBrush = palette.brush(group, role);
                const auto expectedBrush = defaultPalette.brush(group, role);
                if (palette.brush(group, role) != defaultPalette.brush(group, role))
                    qWarning() << "...Difference in" << group << role << actualBrush << expectedBrush;
            }
        }
    });
    QCOMPARE(topLevelWidget.palette(), defaultPalette);
    debugPalette.dismiss();
}

void tst_QApplication::allWidgets()
{
    int argc = 1;
    QApplication app(argc, &argv0);
    QWidget *w = new QWidget;
    QVERIFY(QApplication::allWidgets().contains(w)); // uncreate widget test
    delete w;
    QVERIFY(!QApplication::allWidgets().contains(w)); // removal test
}

void tst_QApplication::topLevelWidgets()
{
    int argc = 1;
    QApplication app(argc, &argv0);
    QWidget *w = new QWidget;
    w->show();
#ifndef QT_NO_CLIPBOARD
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(QLatin1String("newText"));
#endif
    QCoreApplication::processEvents();
    QVERIFY(QApplication::topLevelWidgets().contains(w));
    QCOMPARE(QApplication::topLevelWidgets().size(), 1);
    delete w;
    w = nullptr;
    QCoreApplication::processEvents();
    QCOMPARE(QApplication::topLevelWidgets().size(), 0);
}



void tst_QApplication::setAttribute()
{
    int argc = 1;
    QApplication app(argc, &argv0);
    QVERIFY(!QApplication::testAttribute(Qt::AA_NativeWindows));
    QWidget  *w = new QWidget;
    w->show(); // trigger creation;
    QVERIFY(!w->testAttribute(Qt::WA_NativeWindow));
    delete w;

    QApplication::setAttribute(Qt::AA_NativeWindows);
    QVERIFY(QApplication::testAttribute(Qt::AA_NativeWindows));
    w = new QWidget;
    w->show(); // trigger creation
    QVERIFY(w->testAttribute(Qt::WA_NativeWindow));
    delete w;

    QApplication::setAttribute(Qt::AA_NativeWindows, false);
    QVERIFY(!QApplication::testAttribute(Qt::AA_NativeWindows));
    w = new QWidget;
    w->show(); // trigger creation;
    QVERIFY(!w->testAttribute(Qt::WA_NativeWindow));
    delete w;
}

class TouchEventPropagationTestWidget : public QWidget
{
    Q_OBJECT

public:
    bool seenTouchEvent = false, acceptTouchEvent = false, seenMouseEvent = false, acceptMouseEvent = false;


    explicit TouchEventPropagationTestWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        setAttribute(Qt::WA_TouchPadAcceptSingleTouchEvents);
    }

    void reset()
    {
        seenTouchEvent = acceptTouchEvent = seenMouseEvent = acceptMouseEvent = false;
    }

    bool event(QEvent *event) override
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


    QPointingDevice *device = QTest::createTouchDevice();

    {
        // touch event behavior on a window
        TouchEventPropagationTestWidget window;
        window.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        window.resize(200, 200);
        window.setObjectName("1. window");
        window.show(); // Must have an explicitly specified QWindow for handleTouchEvent,
                       // passing 0 would result in using topLevelAt() which is not ok in this case
                       // as the screen position in the point is bogus.
        auto handle = window.windowHandle();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        // QPA always takes screen positions and since we map the TouchPoint back to QPA's structure first,
        // we must ensure there is a screen position in the TouchPoint that maps to a local 0, 0.
        const QPoint deviceGlobalPos = window.mapToGlobal(QPoint(0, 0));
        auto pressedTouchPoints = QList<QEventPoint>() <<
            QEventPoint(0, QEventPoint::State::Pressed, QPointF(), deviceGlobalPos);
        auto releasedTouchPoints = QList<QEventPoint>() <<
            QEventPoint(0, QEventPoint::State::Released, QPointF(), deviceGlobalPos);

        QWindowSystemInterface::handleTouchEvent(handle,
                                                 0,
                                                 device,
                                                 QWindowSystemInterfacePrivate::toNativeTouchPoints(pressedTouchPoints, handle));
        QWindowSystemInterface::handleTouchEvent(handle,
                                                 0,
                                                 device,
                                                 QWindowSystemInterfacePrivate::toNativeTouchPoints(releasedTouchPoints, handle));
        QCoreApplication::processEvents();
        QVERIFY(!window.seenTouchEvent);
        QVERIFY(window.seenMouseEvent); // QApplication may transform ignored touch events in mouse events

        window.reset();
        window.setAttribute(Qt::WA_AcceptTouchEvents);
        QWindowSystemInterface::handleTouchEvent(handle,
                                                 0,
                                                 device,
                                                 QWindowSystemInterfacePrivate::toNativeTouchPoints(pressedTouchPoints, handle));
        QWindowSystemInterface::handleTouchEvent(handle,
                                                 0,
                                                 device,
                                                 QWindowSystemInterfacePrivate::toNativeTouchPoints(releasedTouchPoints, handle));
        QCoreApplication::processEvents();
        QVERIFY(window.seenTouchEvent);
        QVERIFY(window.seenMouseEvent);

        window.reset();
        window.acceptTouchEvent = true;
        QWindowSystemInterface::handleTouchEvent(handle,
                                                 0,
                                                 device,
                                                 QWindowSystemInterfacePrivate::toNativeTouchPoints(pressedTouchPoints, handle));
        QWindowSystemInterface::handleTouchEvent(handle,
                                                 0,
                                                 device,
                                                 QWindowSystemInterfacePrivate::toNativeTouchPoints(releasedTouchPoints, handle));
        QCoreApplication::processEvents();
        QVERIFY(window.seenTouchEvent);
        QVERIFY(!window.seenMouseEvent);
    }

    {
        // touch event behavior on a window with a child widget
        TouchEventPropagationTestWidget window;
        window.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
        window.resize(200, 200);
        window.setObjectName("2. window");
        TouchEventPropagationTestWidget widget(&window);
        widget.resize(200, 200);
        widget.setObjectName("2. widget");
        window.show();
        auto handle = window.windowHandle();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        const QPoint deviceGlobalPos = window.mapToGlobal(QPoint(50, 150));
        auto pressedTouchPoints = QList<QEventPoint>() <<
            QEventPoint(0, QEventPoint::State::Pressed, QPointF(), deviceGlobalPos);
        auto releasedTouchPoints = QList<QEventPoint>() <<
            QEventPoint(0, QEventPoint::State::Released, QPointF(), deviceGlobalPos);

        QWindowSystemInterface::handleTouchEvent(handle,
                                                 0,
                                                 device,
                                                 QWindowSystemInterfacePrivate::toNativeTouchPoints(pressedTouchPoints, handle));
        QWindowSystemInterface::handleTouchEvent(handle,
                                                 0,
                                                 device,
                                                 QWindowSystemInterfacePrivate::toNativeTouchPoints(releasedTouchPoints, handle));
        QTRY_VERIFY(widget.seenMouseEvent);
        QVERIFY(!widget.seenTouchEvent);
        QVERIFY(!window.seenTouchEvent);
        QVERIFY(window.seenMouseEvent);

        window.reset();
        widget.reset();
        widget.setAttribute(Qt::WA_AcceptTouchEvents);
        QWindowSystemInterface::handleTouchEvent(handle,
                                                 0,
                                                 device,
                                                 QWindowSystemInterfacePrivate::toNativeTouchPoints(pressedTouchPoints, handle));
        QWindowSystemInterface::handleTouchEvent(handle,
                                                 0,
                                                 device,
                                                 QWindowSystemInterfacePrivate::toNativeTouchPoints(releasedTouchPoints, handle));
        QCoreApplication::processEvents();
        QVERIFY(widget.seenTouchEvent);
        QVERIFY(widget.seenMouseEvent);
        QVERIFY(!window.seenTouchEvent);
        QVERIFY(window.seenMouseEvent);

        window.reset();
        widget.reset();
        widget.acceptMouseEvent = true;
        QWindowSystemInterface::handleTouchEvent(handle,
                                                 0,
                                                 device,
                                                 QWindowSystemInterfacePrivate::toNativeTouchPoints(pressedTouchPoints, handle));
        QWindowSystemInterface::handleTouchEvent(handle,
                                                 0,
                                                 device,
                                                 QWindowSystemInterfacePrivate::toNativeTouchPoints(releasedTouchPoints, handle));
        QCoreApplication::processEvents();
        QVERIFY(widget.seenTouchEvent);
        QVERIFY(widget.seenMouseEvent);
        QVERIFY(!window.seenTouchEvent);
        QVERIFY(!window.seenMouseEvent);

        window.reset();
        widget.reset();
        widget.acceptTouchEvent = true;
        QWindowSystemInterface::handleTouchEvent(handle,
                                                 0,
                                                 device,
                                                 QWindowSystemInterfacePrivate::toNativeTouchPoints(pressedTouchPoints, handle));
        QWindowSystemInterface::handleTouchEvent(handle,
                                                 0,
                                                 device,
                                                 QWindowSystemInterfacePrivate::toNativeTouchPoints(releasedTouchPoints, handle));
        QCoreApplication::processEvents();
        QVERIFY(widget.seenTouchEvent);
        QVERIFY(!widget.seenMouseEvent);
        QVERIFY(!window.seenTouchEvent);
        QVERIFY(!window.seenMouseEvent);

        window.reset();
        widget.reset();
        widget.setAttribute(Qt::WA_AcceptTouchEvents, false);
        window.setAttribute(Qt::WA_AcceptTouchEvents);
        QWindowSystemInterface::handleTouchEvent(handle,
                                                 0,
                                                 device,
                                                 QWindowSystemInterfacePrivate::toNativeTouchPoints(pressedTouchPoints, handle));
        QWindowSystemInterface::handleTouchEvent(handle,
                                                 0,
                                                 device,
                                                 QWindowSystemInterfacePrivate::toNativeTouchPoints(releasedTouchPoints, handle));
        QCoreApplication::processEvents();
        QVERIFY(!widget.seenTouchEvent);
        QVERIFY(widget.seenMouseEvent);
        QVERIFY(window.seenTouchEvent);
        QVERIFY(window.seenMouseEvent);

        window.reset();
        widget.reset();
        window.acceptTouchEvent = true;
        QWindowSystemInterface::handleTouchEvent(handle,
                                                 0,
                                                 device,
                                                 QWindowSystemInterfacePrivate::toNativeTouchPoints(pressedTouchPoints, handle));
        QWindowSystemInterface::handleTouchEvent(handle,
                                                 0,
                                                 device,
                                                 QWindowSystemInterfacePrivate::toNativeTouchPoints(releasedTouchPoints, handle));
        QCoreApplication::processEvents();
        QVERIFY(!widget.seenTouchEvent);
        QVERIFY(!widget.seenMouseEvent);
        QVERIFY(window.seenTouchEvent);
        QVERIFY(!window.seenMouseEvent);

        window.reset();
        widget.reset();
        widget.acceptMouseEvent = true; // doesn't matter, touch events are propagated first
        window.acceptTouchEvent = true;
        QWindowSystemInterface::handleTouchEvent(handle,
                                                 0,
                                                 device,
                                                 QWindowSystemInterfacePrivate::toNativeTouchPoints(pressedTouchPoints, handle));
        QWindowSystemInterface::handleTouchEvent(handle,
                                                 0,
                                                 device,
                                                 QWindowSystemInterfacePrivate::toNativeTouchPoints(releasedTouchPoints, handle));
        QCoreApplication::processEvents();
        QVERIFY(!widget.seenTouchEvent);
        QVERIFY(!widget.seenMouseEvent);
        QVERIFY(window.seenTouchEvent);
        QVERIFY(!window.seenMouseEvent);
    }
}

/*!
    Test that wheel events are propagated correctly.

    The event propagation of wheel events is complex: generally, they are propagated
    up the parent tree like other input events, until a widget accepts the event. However,
    wheel events are ignored by default (unlike mouse events, which are accepted by default,
    and ignored in the default implementation of the event handler of QWidget).

    And Qt tries to make sure that wheel events that "belong together" are going to the same
    widget. However, for low-precision events as generated by an old-fashioned
    mouse wheel, each event is a distinct event, so Qt has no choice than to deliver the event
    to the widget under the mouse.
    High-precision events, as generated by track pads or other kinetic scrolling devices, come
    in a continuous stream, with different phases. Qt tries to make sure that all events in the
    same stream go to the widget that accepted the first event.

    Also, QAbstractScrollArea forwards wheel events from the viewport to the relevant scrollbar,
    which adds more complexity to the handling.

    This tests two scenarios:
    1) a large widget inside a scrollarea that scrolls, inside a scrollarea that also scrolls
    2) a large widget inside a scrollarea that doesn't scroll, within a scrollarea that does

    For scenario 1 "inner", the expectation is that the inner scrollarea handles all wheel
    events.
    For scenario 2 "outer", the expectation is that the outer scrollarea handles all wheel
    events.
*/
struct WheelEvent
{
    WheelEvent(Qt::ScrollPhase p = Qt::NoScrollPhase, Qt::Orientation o = Qt::Vertical)
    : phase(p), orientation(o)
    {}
    Qt::ScrollPhase phase = Qt::NoScrollPhase;
    Qt::Orientation orientation = Qt::Vertical;
};
using WheelEventList = QList<WheelEvent>;

void tst_QApplication::wheelEventPropagation_data()
{
    qRegisterMetaType<WheelEventList>();

    QTest::addColumn<bool>("innerScrolls");
    QTest::addColumn<WheelEventList>("events");

    QTest::addRow("inner, classic")
        << true
        << WheelEventList{{}, {}, {}};
    QTest::addRow("outer, classic")
        << false
        << WheelEventList{{}, {}, {}};
    QTest::addRow("inner, kinetic")
        << true
        << WheelEventList{Qt::ScrollBegin, Qt::ScrollUpdate, Qt::ScrollMomentum, Qt::ScrollEnd};
    QTest::addRow("outer, kinetic")
        << false
        << WheelEventList{Qt::ScrollBegin, Qt::ScrollUpdate, Qt::ScrollMomentum, Qt::ScrollEnd};
    QTest::addRow("inner, partial kinetic")
        << true
        << WheelEventList{Qt::ScrollUpdate, Qt::ScrollMomentum, Qt::ScrollEnd};
    QTest::addRow("outer, partial kinetic")
        << false
        << WheelEventList{Qt::ScrollUpdate, Qt::ScrollMomentum, Qt::ScrollEnd};
    QTest::addRow("inner, changing direction")
        << true
        << WheelEventList{Qt::ScrollUpdate, {Qt::ScrollUpdate, Qt::Horizontal}, Qt::ScrollMomentum, Qt::ScrollEnd};
    QTest::addRow("outer, changing direction")
        << false
        << WheelEventList{Qt::ScrollUpdate, {Qt::ScrollUpdate, Qt::Horizontal}, Qt::ScrollMomentum, Qt::ScrollEnd};
}

void tst_QApplication::wheelEventPropagation()
{
    QFETCH(bool, innerScrolls);
    QFETCH(WheelEventList, events);

    const QSize baseSize(500, 500);
    const QPointF center(baseSize.width() / 2, baseSize.height() / 2);
    int scrollStep = 50;

    int argc = 1;
    QApplication app(argc, &argv0);

    QScrollArea outerArea;
    outerArea.setObjectName("outerArea");
    outerArea.viewport()->setObjectName("outerArea_viewport");
    QScrollArea innerArea;
    innerArea.setObjectName("innerArea");
    innerArea.viewport()->setObjectName("innerArea_viewport");
    QWidget largeWidget;
    largeWidget.setObjectName("largeWidget");
    QScrollBar trap(Qt::Vertical, &largeWidget);
    trap.setObjectName("It's a trap!");

    largeWidget.setFixedSize(baseSize * 8);

    // classic wheel events will be grabbed by the widget under the mouse, so don't place a trap
    if (events.at(0).phase == Qt::NoScrollPhase)
        trap.hide();
    // kinetic wheel events should all go to the first widget; place a trap
    else
        trap.setGeometry(center.x() - 50, center.y() + scrollStep, 100, baseSize.height());

    // if the inner area is large enough to host the widget, then it won't scroll
    innerArea.setWidget(&largeWidget);
    innerArea.setFixedSize(innerScrolls ? baseSize * 4
                                        : largeWidget.minimumSize() + QSize(100, 100));
    // the outer area always scrolls
    outerArea.setFixedSize(baseSize);
    outerArea.setWidget(&innerArea);
    outerArea.show();

    if (!QTest::qWaitForWindowExposed(&outerArea))
        QSKIP("Window failed to show, can't run test");

    auto innerVBar = innerArea.verticalScrollBar();
    innerVBar->setObjectName("innerArea_vbar");
    QCOMPARE(innerVBar->isVisible(), innerScrolls);
    auto innerHBar = innerArea.horizontalScrollBar();
    innerHBar->setObjectName("innerArea_hbar");
    QCOMPARE(innerHBar->isVisible(), innerScrolls);
    auto outerVBar = outerArea.verticalScrollBar();
    outerVBar->setObjectName("outerArea_vbar");
    QVERIFY(outerVBar->isVisible());
    auto outerHBar = outerArea.horizontalScrollBar();
    outerHBar->setObjectName("outerArea_hbar");
    QVERIFY(outerHBar->isVisible());

    const QPointF global(outerArea.mapToGlobal(center.toPoint()));

    QSignalSpy innerVSpy(innerVBar, &QAbstractSlider::valueChanged);
    QSignalSpy innerHSpy(innerHBar, &QAbstractSlider::valueChanged);
    QSignalSpy outerVSpy(outerVBar, &QAbstractSlider::valueChanged);
    QSignalSpy outerHSpy(outerHBar, &QAbstractSlider::valueChanged);

    int vcount = 0;
    int hcount = 0;

    for (const auto &event : std::as_const(events)) {
        const QPoint pixelDelta = event.orientation == Qt::Vertical ? QPoint(0, -scrollStep) : QPoint(-scrollStep, 0);
        const QPoint angleDelta = event.orientation == Qt::Vertical ? QPoint(0, -120) : QPoint(-120, 0);
        QWindowSystemInterface::handleWheelEvent(outerArea.windowHandle(), center, global,
                                                 pixelDelta, angleDelta, Qt::NoModifier,
                                                 event.phase);
        if (event.orientation == Qt::Vertical)
            ++vcount;
        else
            ++hcount;
        QCoreApplication::processEvents();
        QCOMPARE(innerVSpy.size(), innerScrolls ? vcount : 0);
        QCOMPARE(innerHSpy.size(), innerScrolls ? hcount : 0);
        QCOMPARE(outerVSpy.size(), innerScrolls ? 0 : vcount);
        QCOMPARE(outerHSpy.size(), innerScrolls ? 0 : hcount);
    }
}

QString modalHelperPath()
{
#ifdef Q_OS_ANDROID
    int argc = 1;
    QApplication app(argc, &argv0);
    return app.applicationDirPath() + QString("/libmodal_helper_%1.so").arg(androidAbi());
#else
    return "./modal_helper";
#endif
}

// QTBUG-133037, QTBUG-12673
void tst_QApplication::modalDialog()
{
#if QT_CONFIG(process)
    QProcess testProcess;
    QStringList arguments;
    testProcess.start(modalHelperPath(), arguments);
    QVERIFY2(testProcess.waitForStarted(),
             qPrintable(QString::fromLatin1("Cannot start 'modal_helper': %1").arg(testProcess.errorString())));
    QVERIFY(testProcess.waitForFinished(20000));
    QCOMPARE(testProcess.exitStatus(), QProcess::NormalExit);
#else
    QSKIP("No QProcess support");
#endif
}

void tst_QApplication::qtbug_103611()
{
    {
        int argc = 0;
        QApplication app(argc, nullptr);
        auto ll = QLocale().uiLanguages();
    }
    {
        int argc = 0;
        QApplication app(argc, nullptr);
        auto ll = QLocale().uiLanguages();
    }
}

class NoQuitOnHideWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NoQuitOnHideWidget(QWidget *parent = nullptr)
      : QWidget(parent)
    {
        QTimer::singleShot(0, this, &QWidget::hide);
        QTimer::singleShot(500, this, [] () { QCoreApplication::exit(1); });
    }
};

void tst_QApplication::noQuitOnHide()
{
    int argc = 0;
    QApplication app(argc, nullptr);
    NoQuitOnHideWidget window1;
    window1.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    window1.show();
    QCOMPARE(QCoreApplication::exec(), 1);
}

class ShowCloseShowWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ShowCloseShowWidget(bool showAgain, QWidget *parent = nullptr)
        : QWidget(parent), showAgain(showAgain)
    {
        int timeout = 500;
#ifdef Q_OS_ANDROID
        // On Android, CI Android emulator is not running HW accelerated graphics and can be slow,
        // use a longer timeout to avoid flaky failures
        timeout = 1000;
#endif
        QTimer::singleShot(timeout, this, [] () { QCoreApplication::exit(1); });
    }

    bool shown = false;

protected:
    void showEvent(QShowEvent *) override
    {
        QTimer::singleShot(0, this, &ShowCloseShowWidget::doClose);
        shown = true;
    }
    void hideEvent(QHideEvent *) override
    {
        shown = false;
    }

private slots:
    void doClose() {
        close();
        if (showAgain)
            show();
    }

private:
    const bool showAgain;
};

void tst_QApplication::abortQuitOnShow()
{
    int argc = 0;
    QApplication app(argc, nullptr);

    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This crash, see QTBUG-123172.");

    ShowCloseShowWidget window1(false);
    window1.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    window1.show();
    QVERIFY(QTest::qWaitFor([&window1](){ return window1.shown; }));
    QCOMPARE(QCoreApplication::exec(), 0);

    ShowCloseShowWidget window2(true);
    window2.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    window2.show();
    QVERIFY(QTest::qWaitFor([&window2](){ return window2.shown; }));
    QCOMPARE(QCoreApplication::exec(), 1);
}

// Test that static functions do not crash if there is no application instance.
void tst_QApplication::staticFunctions()
{
    QApplication::setStyle(QStringLiteral("blub"));
    QApplication::allWidgets();
    QApplication::topLevelWidgets();
    QApplication::activePopupWidget();
    QTest::ignoreMessage(QtWarningMsg, "Must construct a QGuiApplication first.");
    QApplication::activeModalWidget();
    QApplication::focusWidget();
    QApplication::activeWindow();
    QApplicationPrivate::setActiveWindow(nullptr);
    QApplication::widgetAt(QPoint(0, 0));
    QApplication::topLevelAt(QPoint(0, 0));
    QTest::ignoreMessage(QtWarningMsg, "Must construct a QApplication first.");
    QApplication::isEffectEnabled(Qt::UI_General);
    QApplication::setEffectEnabled(Qt::UI_General, false);
}

void tst_QApplication::settableStyleHints_data()
{
    QTest::addColumn<bool>("appInstance");
    QTest::newRow("app") << true;
    QTest::newRow("no-app") << false;
}

void tst_QApplication::settableStyleHints()
{
    QFETCH(bool, appInstance);
    int argc = 0;
    QScopedPointer<QApplication> app;
    if (appInstance)
        app.reset(new QApplication(argc, nullptr));

    QApplication::setCursorFlashTime(437);
    QCOMPARE(QApplication::cursorFlashTime(), 437);
    QApplication::setDoubleClickInterval(128);
    QCOMPARE(QApplication::doubleClickInterval(), 128);
    QApplication::setStartDragDistance(122000);
    QCOMPARE(QApplication::startDragDistance(), 122000);
    QApplication::setStartDragTime(834);
    QCOMPARE(QApplication::startDragTime(), 834);
    QApplication::setKeyboardInputInterval(309);
    QCOMPARE(QApplication::keyboardInputInterval(), 309);
}

/*
    This test is meant to ensure that certain objects (public & commonly used)
    can safely be used in a Q_GLOBAL_STATIC such that their destructors are
    executed *after* the destruction of QApplication.
 */
Q_GLOBAL_STATIC(QLocale, tst_qapp_locale);
#if QT_CONFIG(process)
Q_GLOBAL_STATIC(QProcess, tst_qapp_process);
#endif
#if QT_CONFIG(filesystemwatcher)
Q_GLOBAL_STATIC(QFileSystemWatcher, tst_qapp_fileSystemWatcher);
#endif
#ifndef QT_NO_SHAREDMEMORY
Q_GLOBAL_STATIC(QSharedMemory, tst_qapp_sharedMemory);
#endif
Q_GLOBAL_STATIC(QElapsedTimer, tst_qapp_elapsedTimer);
Q_GLOBAL_STATIC(QMutex, tst_qapp_mutex);
Q_GLOBAL_STATIC(QWidget, tst_qapp_widget);
Q_GLOBAL_STATIC(QPixmap, tst_qapp_pixmap);
Q_GLOBAL_STATIC(QFont, tst_qapp_font);
Q_GLOBAL_STATIC(QRegion, tst_qapp_region);
#ifndef QT_NO_CURSOR
Q_GLOBAL_STATIC(QCursor, tst_qapp_cursor);
#endif

void tst_QApplication::globalStaticObjectDestruction()
{
    int argc = 1;
    QApplication app(argc, &argv0);
    QVERIFY(tst_qapp_locale());
#if QT_CONFIG(process)
    QVERIFY(tst_qapp_process());
#endif
#if QT_CONFIG(filesystemwatcher)
    QVERIFY(tst_qapp_fileSystemWatcher());
#endif
#ifndef QT_NO_SHAREDMEMORY
    QVERIFY(tst_qapp_sharedMemory());
#endif
    QVERIFY(tst_qapp_elapsedTimer());
    QVERIFY(tst_qapp_mutex());
    QVERIFY(tst_qapp_widget());
    QVERIFY(tst_qapp_pixmap());
    QVERIFY(tst_qapp_font());
    QVERIFY(tst_qapp_region());
#ifndef QT_NO_CURSOR
    QVERIFY(tst_qapp_cursor());
#endif
}

//QTEST_APPLESS_MAIN(tst_QApplication)
int main(int argc, char *argv[])
{
    tst_QApplication tc;
    argv0 = argv[0];
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&tc, argc, argv);
}

#include "tst_qapplication.moc"
