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

#define QT_STATICPLUGIN
#include <QtWidgets/qstyleplugin.h>

#include <qdebug.h>

#include <QtTest/QtTest>

#include <QtCore/QAbstractEventDispatcher>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#if QT_CONFIG(process)
# include <QtCore/QProcess>
#endif
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
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/private/qapplication_p.h>
#include <QtWidgets/QStyle>
#include <QtWidgets/qproxystyle.h>

#include <qpa/qwindowsysteminterface.h>
#include <qpa/qwindowsysteminterface_p.h>
#include <private/qhighdpiscaling_p.h>

#include <algorithm>

Q_LOGGING_CATEGORY(lcTests, "qt.widgets.tests")

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

#if QT_CONFIG(library)
    void libraryPaths();
    void libraryPaths_qt_plugin_path();
    void libraryPaths_qt_plugin_path_2();
#endif

    void sendPostedEvents();

    void thread();
    void desktopSettingsAware();

    void setActiveWindow();

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

    void allWidgets();
    void topLevelWidgets();

    void setAttribute();

    void touchEventPropagation();
    void wheelEventPropagation_data();
    void wheelEventPropagation();

    void qtbug_12673();
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

    bool palette_changed = false;
    QPalette pal;
    QApplication::setPalette(pal);

    /*QFont font;
    QApplication::setFont(font);*/

    int argc = 0;
    QApplication app(argc, nullptr);
    QObject::connect(&app, &QApplication::paletteChanged, [&palette_changed]{ palette_changed = true; });
    QVERIFY(!palette_changed);
    qApp->setPalette(QPalette(Qt::red));
    QVERIFY(palette_changed);
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
#ifdef Q_OS_WINRT
    QSKIP("WinRT does not support more than 1 native widget at the same time");
#endif
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
    QApplication::setActiveWindow(&widget2);
    QApplication::alert(&widget, 0);
    widget.activateWindow();
    QApplication::setActiveWindow(&widget);
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

    QFontDatabase fdb;
    const QStringList &families = fdb.families();
    for (int i = 0, count = qMin(3, families.size()); i < count; ++i) {
        const auto &family = families.at(i);
        const QStringList &styles = fdb.styles(family);
        if (!styles.isEmpty()) {
            QList<int> sizes = fdb.pointSizes(family, styles.constFirst());
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

    const auto &list = args.splitRef(' ');
    auto argarray = new char*[list.count() + 1];

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
    QCOMPARE(spy.count(), 0);

    QPointer<CloseWidget>widget = new CloseWidget;
    widget->setWindowTitle(QLatin1String(QTest::currentTestFunction()) + QLatin1String("CloseWidget"));
    QVERIFY(widget->testAttribute(Qt::WA_QuitOnClose));
    widget->show();
    QObject::connect(&app, &QGuiApplication::lastWindowClosed, widget.data(), &QObject::deleteLater);
    QCoreApplication::exec();
    QVERIFY(!widget);
    QCOMPARE(spy.count(), 1);
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
    QCOMPARE(spy.count(), 1);
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
        QCOMPARE(spy.count(), 1);
        QCOMPARE(appSpy.count(), 1);
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
        QCOMPARE(spy1.count(), 1);

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
        QCOMPARE(appSpy.count(), 2);
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
        QCOMPARE(spy1.count(), 1);
        QCOMPARE(appSpy.count(), 0);

        QTimer timer2;
        connect(&timer2, &QTimer::timeout, &app, &QCoreApplication::quit);
        QSignalSpy spy2(&timer2, &QTimer::timeout);
        timer2.setSingleShot(true);
        timer2.start(1000);
        int returnValue = QCoreApplication::exec();
        QCOMPARE(returnValue, 0);
        QCOMPARE(spy2.count(), 1);
        QCOMPARE(appSpy.count(), 0);
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

        QCOMPARE(spy.count(), 1);
        QVERIFY(spy2.count() < 15);      // Should be around 10 if closing caused the quit
    }

    bool quitApplicationTriggered = false;
    auto quitSlot = [&quitApplicationTriggered] () {
        quitApplicationTriggered = true;
        QCoreApplication::quit();
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

        QCOMPARE(spy.count(), 1);
        QVERIFY(quitApplicationTriggered);
    }
    {
        int argc = 0;
        QApplication app(argc, nullptr);
        QSignalSpy appSpy(&app, &QApplication::lastWindowClosed);

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
        QCOMPARE(timerSpy.count(), 1);
        QCOMPARE(appSpy.count(), 2);
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

        QVERIFY(timerSpy.count() < 10);
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
#ifdef Q_OS_WINRT
    QSKIP("PromptOnCloseWidget does not work on WinRT - QTBUG-68297");
#endif
    int argc = 0;
    QApplication app(argc, nullptr);

    // create some windows
    new QWidget;
    new QWidget;
    new QWidget;

    // show all windows
    auto topLevels = QApplication::topLevelWidgets();
    for (QWidget *w : qAsConst(topLevels)) {
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
    for (QWidget *w : qAsConst(topLevels)) {
        w->show();
        QVERIFY(QTest::qWaitForWindowExposed(w));
    }
    // close the last window to open the prompt (eventloop recurses)
    promptOnCloseWidget->close();
    // all windows should not be visible, except the one that opened the prompt
    for (QWidget *w : qAsConst(topLevels)) {
        if (w == promptOnCloseWidget)
            QVERIFY(w->isVisible());
        else
            QVERIFY(!w->isVisible());
    }

    qDeleteAll(QApplication::topLevelWidgets());
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

#if QT_CONFIG(library)
void tst_QApplication::libraryPaths()
{
#ifndef BUILTIN_TESTDATA
        const QString testDir = QFileInfo(QFINDTESTDATA("test/test.pro")).absolutePath();
#else
        const QString testDir = QFileInfo(QFINDTESTDATA("test.pro")).absolutePath();
#endif
        QVERIFY(!testDir.isEmpty());
    {
        QApplication::setLibraryPaths(QStringList() << testDir);
        QCOMPARE(QApplication::libraryPaths(), (QStringList() << testDir));

        // creating QApplication adds the applicationDirPath to the libraryPath
        int argc = 1;
        QApplication app(argc, &argv0);
        QString appDirPath = QDir(QCoreApplication::applicationDirPath()).canonicalPath();

        QStringList actual = QApplication::libraryPaths();
        actual.sort();
        QStringList expected;
        expected << testDir << appDirPath;
        expected = QSet<QString>(expected.constBegin(), expected.constEnd()).values();
        expected.sort();

        QVERIFY2(isPathListIncluded(actual, expected),
                 qPrintable("actual:\n - " + actual.join("\n - ") +
                            "\nexpected:\n - " + expected.join("\n - ")));
    }
    {
        // creating QApplication adds the applicationDirPath and plugin install path to the libraryPath
        int argc = 1;
        QApplication app(argc, &argv0);
        QString appDirPath = QCoreApplication::applicationDirPath();
        QString installPathPlugins =  QLibraryInfo::location(QLibraryInfo::PluginsPath);

        QStringList actual = QApplication::libraryPaths();
        actual.sort();

        QStringList expected;
        expected << installPathPlugins << appDirPath;
        expected = QSet<QString>(expected.constBegin(), expected.constEnd()).values();
        expected.sort();

#ifdef Q_OS_WINRT
        QEXPECT_FAIL("", "On WinRT PluginsPath is outside of sandbox. QTBUG-68297", Abort);
#endif
        QVERIFY2(isPathListIncluded(actual, expected),
                 qPrintable("actual:\n - " + actual.join("\n - ") +
                            "\nexpected:\n - " + expected.join("\n - ")));

        // setting the library paths overrides everything
         QApplication::setLibraryPaths(QStringList() << testDir);
        QVERIFY2(isPathListIncluded(QApplication::libraryPaths(), (QStringList() << testDir)),
                 qPrintable("actual:\n - " + QApplication::libraryPaths().join("\n - ") +
                            "\nexpected:\n - " + testDir));
    }
    {
        qCDebug(lcTests) << "Initial library path:" << QApplication::libraryPaths();

        int count = QApplication::libraryPaths().count();
#if 0
        // this test doesn't work if KDE 4 is installed
        QCOMPARE(count, 1); // before creating QApplication, only the PluginsPath is in the libraryPaths()
#endif
        QString installPathPlugins =  QLibraryInfo::location(QLibraryInfo::PluginsPath);
        QApplication::addLibraryPath(installPathPlugins);
        qCDebug(lcTests) << "installPathPlugins" << installPathPlugins;
        qCDebug(lcTests) << "After adding plugins path:" << QApplication::libraryPaths();
        QCOMPARE(QApplication::libraryPaths().count(), count);
        QApplication::addLibraryPath(testDir);
        QCOMPARE(QApplication::libraryPaths().count(), count + 1);

        // creating QApplication adds the applicationDirPath to the libraryPath
        int argc = 1;
        QApplication app(argc, &argv0);
        QString appDirPath = QCoreApplication::applicationDirPath();
        qCDebug(lcTests) << QApplication::libraryPaths();
        // On Windows CE these are identical and might also be the case for other
        // systems too
        if (appDirPath != installPathPlugins)
            QCOMPARE(QApplication::libraryPaths().count(), count + 2);
    }
    {
        int argc = 1;
        QApplication app(argc, &argv0);

        qCDebug(lcTests) << "Initial library path:" << QCoreApplication::libraryPaths();
        int count = QCoreApplication::libraryPaths().count();
        QString installPathPlugins =  QLibraryInfo::location(QLibraryInfo::PluginsPath);
        QCoreApplication::addLibraryPath(installPathPlugins);
        qCDebug(lcTests) << "installPathPlugins" << installPathPlugins;
        qCDebug(lcTests) << "After adding plugins path:" << QCoreApplication::libraryPaths();
        QCOMPARE(QCoreApplication::libraryPaths().count(), count);

        QString appDirPath = QCoreApplication::applicationDirPath();

        QCoreApplication::addLibraryPath(appDirPath);
        QCoreApplication::addLibraryPath(appDirPath + "/..");
        qCDebug(lcTests) << "appDirPath" << appDirPath;
        qCDebug(lcTests) << "After adding appDirPath && appDirPath + /..:" << QCoreApplication::libraryPaths();
        QCOMPARE(QCoreApplication::libraryPaths().count(), count + 1);
#ifdef Q_OS_MACOS
        QCoreApplication::addLibraryPath(appDirPath + "/../MacOS");
#else
        QCoreApplication::addLibraryPath(appDirPath + "/tmp/..");
#endif
        qCDebug(lcTests) << "After adding appDirPath + /tmp/..:" << QCoreApplication::libraryPaths();
        QCOMPARE(QCoreApplication::libraryPaths().count(), count + 1);
    }
}

void tst_QApplication::libraryPaths_qt_plugin_path()
{
    int argc = 1;

    QApplication app(argc, &argv0);
    QString appDirPath = QCoreApplication::applicationDirPath();

    // Our hook into libraryPaths() initialization: Set the QT_PLUGIN_PATH environment variable
    QString installPathPluginsDeCanon = appDirPath + QString::fromLatin1("/tmp/..");
    QByteArray ascii = QFile::encodeName(installPathPluginsDeCanon);
    qputenv("QT_PLUGIN_PATH", ascii);

    QVERIFY(!QCoreApplication::libraryPaths().contains(appDirPath + QString::fromLatin1("/tmp/..")));
}

void tst_QApplication::libraryPaths_qt_plugin_path_2()
{
#ifdef Q_OS_UNIX
    QByteArray validPath = QDir("/tmp").canonicalPath().toLatin1();
    QByteArray nonExistentPath = "/nonexistent";
    QByteArray pluginPath = validPath + ':' + nonExistentPath;
#elif defined(Q_OS_WIN)
    QByteArray validPath = "C:\\windows";
    QByteArray nonExistentPath = "Z:\\nonexistent";
    QByteArray pluginPath = validPath + ';' + nonExistentPath;
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
            << QDir(QCoreApplication::applicationDirPath()).canonicalPath()
            << QDir(QDir::fromNativeSeparators(QString::fromLatin1(validPath))).canonicalPath();

#ifdef Q_OS_WINRT
        QEXPECT_FAIL("", "On WinRT PluginsPath is outside of sandbox. QTBUG-68297", Abort);
#endif
        QVERIFY2(isPathListIncluded(QCoreApplication::libraryPaths(), expected),
                 qPrintable("actual:\n - " + QCoreApplication::libraryPaths().join("\n - ") +
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
            << QCoreApplication::applicationDirPath();
        QVERIFY(isPathListIncluded(QCoreApplication::libraryPaths(), expected));

        qputenv("QT_PLUGIN_PATH", QByteArray());
    }
}
#endif

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
    QApplication::postEvent(this, new QDeferredDeleteEvent());

    QEventLoop eventLoop;
    QMetaObject::invokeMethod(&eventLoop, "quit", Qt::QueuedConnection);
    eventLoop.exec();
    QVERIFY(p != nullptr);

    QCOMPARE(eventSpy.count(), 2);
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
#ifdef Q_OS_MAC
    QSKIP("This test fails and then hangs on OS X, see QTBUG-24318");
#endif
    int argc = 0;
    QApplication app(argc, nullptr);
    connect(&app, &QApplication::lastWindowClosed, &app, &QCoreApplication::quit);

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
    testProcess.start("desktopsettingsaware_helper");
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
    QApplication::setActiveWindow(w); // needs this on twm (focus follows mouse)
    QVERIFY(pb1->hasFocus());
    delete w;
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

    QCOMPARE(spy.count(), 0);

    parent1.show();
    QApplication::setActiveWindow(&parent1); // needs this on twm (focus follows mouse)
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).count(), 2);
    old = qvariant_cast<QWidget*>(spy.at(0).at(0));
    now = qvariant_cast<QWidget*>(spy.at(0).at(1));
    QCOMPARE(now, &le1);
    QCOMPARE(now, QApplication::focusWidget());
    QVERIFY(!old);
    spy.clear();
    QCOMPARE(spy.count(), 0);

    pb1.setFocus();
    QCOMPARE(spy.count(), 1);
    old = qvariant_cast<QWidget*>(spy.at(0).at(0));
    now = qvariant_cast<QWidget*>(spy.at(0).at(1));
    QCOMPARE(now, &pb1);
    QCOMPARE(now, QApplication::focusWidget());
    QCOMPARE(old, &le1);
    spy.clear();

    lb1.setFocus();
    QCOMPARE(spy.count(), 1);
    old = qvariant_cast<QWidget*>(spy.at(0).at(0));
    now = qvariant_cast<QWidget*>(spy.at(0).at(1));
    QCOMPARE(now, &lb1);
    QCOMPARE(now, QApplication::focusWidget());
    QCOMPARE(old, &pb1);
    spy.clear();

    lb1.clearFocus();
    QCOMPARE(spy.count(), 1);
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
    QApplication::setActiveWindow(&parent2); // needs this on twm (focus follows mouse)
    QVERIFY(spy.count() > 0); // one for deactivation, one for activation on Windows
    old = qvariant_cast<QWidget*>(spy.at(spy.count()-1).at(0));
    now = qvariant_cast<QWidget*>(spy.at(spy.count()-1).at(1));
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
        QCOMPARE(spy.count(), 0);
        QCOMPARE(now, QApplication::focusWidget());
    } else {
        QVERIFY(spy.count() > 0);
        old = qvariant_cast<QWidget*>(spy.at(0).at(0));
        now = qvariant_cast<QWidget*>(spy.at(0).at(1));
        QCOMPARE(now, &pb2);
        QCOMPARE(now, QApplication::focusWidget());
        QCOMPARE(old, &le2);
        spy.clear();
    }

    if (!tabAllControls) {
        QCOMPARE(spy.count(), 0);
        QCOMPARE(now, QApplication::focusWidget());
    } else {
        tab.simulate(now);
        QVERIFY(spy.count() > 0);
        old = qvariant_cast<QWidget*>(spy.at(0).at(0));
        now = qvariant_cast<QWidget*>(spy.at(0).at(1));
        QCOMPARE(now, &le2);
        QCOMPARE(now, QApplication::focusWidget());
        QCOMPARE(old, &pb2);
        spy.clear();
    }

    if (!tabAllControls) {
        QCOMPARE(spy.count(), 0);
        QCOMPARE(now, QApplication::focusWidget());
    } else {
        backtab.simulate(now);
        QVERIFY(spy.count() > 0);
        old = qvariant_cast<QWidget*>(spy.at(0).at(0));
        now = qvariant_cast<QWidget*>(spy.at(0).at(1));
        QCOMPARE(now, &pb2);
        QCOMPARE(now, QApplication::focusWidget());
        QCOMPARE(old, &le2);
        spy.clear();
    }


    if (!tabAllControls) {
        QCOMPARE(spy.count(), 0);
        QCOMPARE(now, QApplication::focusWidget());
        old = &pb2;
    } else {
        backtab.simulate(now);
        QVERIFY(spy.count() > 0);
        old = qvariant_cast<QWidget*>(spy.at(0).at(0));
        now = qvariant_cast<QWidget*>(spy.at(0).at(1));
        QCOMPARE(now, &le2);
        QCOMPARE(now, QApplication::focusWidget());
        QCOMPARE(old, &pb2);
        spy.clear();
    }

    click.simulate(old);
    if (!(pb2.focusPolicy() & Qt::ClickFocus)) {
        QCOMPARE(spy.count(), 0);
        QCOMPARE(now, QApplication::focusWidget());
    } else {
        QVERIFY(spy.count() > 0);
        old = qvariant_cast<QWidget*>(spy.at(0).at(0));
        now = qvariant_cast<QWidget*>(spy.at(0).at(1));
        QCOMPARE(now, &pb2);
        QCOMPARE(now, QApplication::focusWidget());
        QCOMPARE(old, &le2);
        spy.clear();

        click.simulate(old);
        QVERIFY(spy.count() > 0);
        old = qvariant_cast<QWidget*>(spy.at(0).at(0));
        now = qvariant_cast<QWidget*>(spy.at(0).at(1));
        QCOMPARE(now, &le2);
        QCOMPARE(now, QApplication::focusWidget());
        QCOMPARE(old, &pb2);
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
    QMouseEvent ev(QEvent::MouseButtonPress, QPointF(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QSpontaneKeyEvent::setSpontaneous(&ev);
    QVERIFY(ev.spontaneous());
    qApp->notify(&w2, &ev);
    QCOMPARE(QApplication::focusWidget(), &w);

    // then we give the inner widget strong focus -> it should get focus
    w2.setFocusPolicy(Qt::StrongFocus);
    QSpontaneKeyEvent::setSpontaneous(&ev);
    QVERIFY(ev.spontaneous());
    qApp->notify(&w2, &ev);
#ifdef Q_OS_WINRT
    QEXPECT_FAIL("", "Fails on WinRT - QTBUG-68297", Abort);
#endif
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
    void polish(QPalette &palette)
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
    QStyle *create(const QString &) { return new CustomStyle; }
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
        QVERIFY(!app.palette().resolve());
        QCOMPARE(app.palette().color(QPalette::Link), Qt::red);
        qunsetenv("QT_DESKTOP_STYLE_KEY");
    }
#endif

    {
        QApplication::setStyle(new CustomStyle);
        QApplication app(argc, &argv0);
        QVERIFY(CustomStyle::polished);
        QVERIFY(!app.palette().resolve());
        QCOMPARE(app.palette().color(QPalette::Link), Qt::red);
    }

    {
        QApplication app(argc, &argv0);
        app.setStyle(new CustomStyle);
        QVERIFY(CustomStyle::polished);
        QVERIFY(!app.palette().resolve());
        QCOMPARE(app.palette().color(QPalette::Link), Qt::red);

        CustomStyle::polished = 0;
        app.setPalette(QPalette());
        QVERIFY(CustomStyle::polished);
        QVERIFY(!app.palette().resolve());
        QCOMPARE(app.palette().color(QPalette::Link), Qt::red);

        CustomStyle::polished = 0;
        QPalette palette;
        palette.setColor(QPalette::Active, QPalette::Highlight, Qt::green);
        app.setPalette(palette);
        QVERIFY(CustomStyle::polished);
        QVERIFY(app.palette().resolve());
        QCOMPARE(app.palette().color(QPalette::Link), Qt::red);
        QCOMPARE(app.palette().color(QPalette::Highlight), Qt::green);
    }
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
    QCOMPARE(QApplication::topLevelWidgets().count(), 1);
    delete w;
    w = nullptr;
    QCoreApplication::processEvents();
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
    w2->setParent(nullptr);
    QVERIFY(w2->testAttribute(Qt::WA_WState_Created));
    delete w;
    delete w2;

    QApplication::setAttribute(Qt::AA_ImmediateWidgetCreation, false);
    QVERIFY(!QApplication::testAttribute(Qt::AA_ImmediateWidgetCreation));
    w = new QWidget;
    QVERIFY(!w->testAttribute(Qt::WA_WState_Created));
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

    QList<QTouchEvent::TouchPoint> pressedTouchPoints;
    QTouchEvent::TouchPoint press(0);
    press.setState(Qt::TouchPointPressed);
    pressedTouchPoints << press;

    QList<QTouchEvent::TouchPoint> releasedTouchPoints;
    QTouchEvent::TouchPoint release(0);
    release.setState(Qt::TouchPointReleased);
    releasedTouchPoints << release;

    QTouchDevice *device = QTest::createTouchDevice();

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
        const QPoint deviceGlobalPos =
            QHighDpi::toNativePixels(window.mapToGlobal(QPoint(0, 0)), window.windowHandle()->screen());
        pressedTouchPoints[0].setScreenPos(deviceGlobalPos);
        releasedTouchPoints[0].setScreenPos(deviceGlobalPos);

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
        const QPoint deviceGlobalPos =
            QHighDpi::toNativePixels(window.mapToGlobal(QPoint(50, 150)), window.windowHandle()->screen());
        pressedTouchPoints[0].setScreenPos(deviceGlobalPos);
        releasedTouchPoints[0].setScreenPos(deviceGlobalPos);

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
Q_DECLARE_METATYPE(WheelEvent);

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
#ifdef Q_OS_WINRT
    QSKIP("Not enough available screen space on WinRT for this test");
#endif

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

    for (const auto &event : qAsConst(events)) {
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
        QCOMPARE(innerVSpy.count(), innerScrolls ? vcount : 0);
        QCOMPARE(innerHSpy.count(), innerScrolls ? hcount : 0);
        QCOMPARE(outerVSpy.count(), innerScrolls ? 0 : vcount);
        QCOMPARE(outerHSpy.count(), innerScrolls ? 0 : hcount);
    }
}

void tst_QApplication::qtbug_12673()
{
#if QT_CONFIG(process)
    QProcess testProcess;
    QStringList arguments;
    testProcess.start("modal_helper", arguments);
    QVERIFY2(testProcess.waitForStarted(),
             qPrintable(QString::fromLatin1("Cannot start 'modal_helper': %1").arg(testProcess.errorString())));
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
        QTimer::singleShot(0, this, &ShowCloseShowWidget::doClose);
        QTimer::singleShot(500, this, [] () { QCoreApplication::exit(1); });
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
    ShowCloseShowWidget window1(false);
    window1.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    window1.show();
    QCOMPARE(QCoreApplication::exec(), 0);

    ShowCloseShowWidget window2(true);
    window2.setWindowTitle(QLatin1String(QTest::currentTestFunction()));
    window2.show();
    QCOMPARE(QCoreApplication::exec(), 1);
}

// Test that static functions do not crash if there is no application instance.
void tst_QApplication::staticFunctions()
{
    QApplication::setStyle(QStringLiteral("blub"));
    QApplication::allWidgets();
    QApplication::topLevelWidgets();
    QApplication::desktop();
    QApplication::activePopupWidget();
    QApplication::activeModalWidget();
    QApplication::focusWidget();
    QApplication::activeWindow();
    QApplication::setActiveWindow(nullptr);
    QApplication::widgetAt(QPoint(0, 0));
    QApplication::topLevelAt(QPoint(0, 0));
    QApplication::setGlobalStrut(QSize(0, 0));
    QApplication::globalStrut();
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
Q_GLOBAL_STATIC(QFontDatabase, tst_qapp_fontDatabase);
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
    QVERIFY(tst_qapp_fontDatabase());
#ifndef QT_NO_CURSOR
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
