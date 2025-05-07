// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>
#include <qapplication.h>
#include <qlineedit.h>
#include <qmenu.h>
#include <qlabel.h>
#include <qdialog.h>
#include <qevent.h>
#include <qlineedit.h>
#include <QBoxLayout>
#include <QSysInfo>

#include <qpa/qplatformintegration.h>
#include <private/qguiapplication_p.h>

#include <QtWidgets/private/qapplication_p.h>

QT_FORWARD_DECLARE_CLASS(QWidget)

class FocusLineEdit : public QLineEdit
{
public:
    FocusLineEdit(QWidget *parent = nullptr, const char *name = nullptr ) : QLineEdit(name, parent) {}
    int focusInEventReason;
    int focusOutEventReason;
    bool focusInEventRecieved;
    bool focusInEventGotFocus;
    bool focusOutEventRecieved;
    bool focusOutEventLostFocus;
protected:
    virtual void keyPressEvent( QKeyEvent *e ) override
    {
//        qDebug( QString("keyPressEvent: %1").arg(e->key()) );
        QLineEdit::keyPressEvent( e );
    }
    void focusInEvent( QFocusEvent* e ) override
    {
        QLineEdit::focusInEvent( e );
        focusInEventReason = e->reason();
        focusInEventGotFocus = e->gotFocus();
        focusInEventRecieved = true;
    }
    void focusOutEvent( QFocusEvent* e ) override
    {
        QLineEdit::focusOutEvent( e );
        focusOutEventReason = e->reason();
        focusOutEventLostFocus = !e->gotFocus();
        focusOutEventRecieved = true;
    }
};

class tst_QFocusEvent : public QObject
{
    Q_OBJECT

public:
    void initWidget();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();
    void checkReason_Tab();
    void checkReason_ShiftTab();
    void checkReason_BackTab();
    void checkReason_Popup();
    void checkReason_focusWidget();
#if QT_CONFIG(shortcut)
    void checkReason_Shortcut();
#endif
    void checkReason_ActiveWindow();

private:
    QWidget* testFocusWidget = nullptr;
    FocusLineEdit* childFocusWidgetOne;
    FocusLineEdit* childFocusWidgetTwo;
};

void tst_QFocusEvent::initTestCase()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))
        QSKIP("QWindow::requestActivate() is not supported on this platform.");

    testFocusWidget = new QWidget( nullptr );
    childFocusWidgetOne = new FocusLineEdit( testFocusWidget );
    childFocusWidgetOne->setGeometry( 10, 10, 180, 20 );
    childFocusWidgetTwo = new FocusLineEdit( testFocusWidget );
    childFocusWidgetTwo->setGeometry( 10, 50, 180, 20 );

    //qApp->setMainWidget( testFocusWidget ); Qt4
    testFocusWidget->resize( 200,100 );
    testFocusWidget->show();
    QVERIFY(QTest::qWaitForWindowExposed(testFocusWidget));
// Applications don't get focus when launched from the command line on Mac.
#ifdef Q_OS_MAC
    testFocusWidget->raise();
#endif
}

void tst_QFocusEvent::cleanupTestCase()
{
    delete testFocusWidget;
}

void tst_QFocusEvent::cleanup()
{
    childFocusWidgetTwo->setGeometry( 10, 50, 180, 20 );
}

void tst_QFocusEvent::initWidget()
{
    // On X11 we have to ensure the event was processed before doing any checking, on Windows
    // this is processed straight away.
    QApplicationPrivate::setActiveWindow(testFocusWidget);
    childFocusWidgetOne->setFocus(); // The first lineedit should have focus
    QVERIFY(QTest::qWaitForWindowActive(testFocusWidget));
    QTRY_VERIFY(childFocusWidgetOne->hasFocus());

    childFocusWidgetOne->focusInEventRecieved = false;
    childFocusWidgetOne->focusInEventGotFocus = false;
    childFocusWidgetOne->focusInEventReason = -1;
    childFocusWidgetOne->focusOutEventRecieved = false;
    childFocusWidgetOne->focusOutEventLostFocus = false;
    childFocusWidgetOne->focusOutEventReason = -1;
    childFocusWidgetTwo->focusInEventRecieved = false;
    childFocusWidgetTwo->focusInEventGotFocus = false;
    childFocusWidgetTwo->focusInEventReason = -1;
    childFocusWidgetTwo->focusOutEventRecieved = false;
    childFocusWidgetTwo->focusOutEventLostFocus = false;
    childFocusWidgetTwo->focusOutEventReason = -1;
}

void tst_QFocusEvent::checkReason_Tab()
{
    initWidget();

    // Now test the tab key
    QTest::keyClick( childFocusWidgetOne, Qt::Key_Tab );

    QVERIFY(childFocusWidgetOne->focusOutEventRecieved);
    QVERIFY(childFocusWidgetTwo->focusInEventRecieved);
    QVERIFY(childFocusWidgetOne->focusOutEventLostFocus);
    QVERIFY(childFocusWidgetTwo->focusInEventGotFocus);

    QVERIFY( childFocusWidgetTwo->hasFocus() );
    QCOMPARE( childFocusWidgetOne->focusOutEventReason, (int) Qt::TabFocusReason );
    QCOMPARE( childFocusWidgetTwo->focusInEventReason, (int) Qt::TabFocusReason );
}

void tst_QFocusEvent::checkReason_ShiftTab()
{
    initWidget();

    // Now test the shift + tab key
    QTest::keyClick( childFocusWidgetOne, Qt::Key_Tab, Qt::ShiftModifier );

    QVERIFY(childFocusWidgetOne->focusOutEventRecieved);
    QVERIFY(childFocusWidgetTwo->focusInEventRecieved);
    QVERIFY(childFocusWidgetOne->focusOutEventLostFocus);
    QVERIFY(childFocusWidgetTwo->focusInEventGotFocus);

    QVERIFY( childFocusWidgetTwo->hasFocus() );
    QCOMPARE( childFocusWidgetOne->focusOutEventReason, (int)Qt::BacktabFocusReason );
    QCOMPARE( childFocusWidgetTwo->focusInEventReason, (int)Qt::BacktabFocusReason );

}

/*!
    In this test we verify that the Qt::KeyBacktab key is handled in a qfocusevent
*/
void tst_QFocusEvent::checkReason_BackTab()
{
#ifdef Q_OS_WIN32 // key is not supported on Windows
    QSKIP( "Backtab is not supported on Windows");
#else
    initWidget();
    QVERIFY( childFocusWidgetOne->hasFocus() );

    // Now test the backtab key
    QTest::keyClick( childFocusWidgetOne, Qt::Key_Backtab );

    QTRY_VERIFY(childFocusWidgetOne->focusOutEventRecieved);
    QVERIFY(childFocusWidgetTwo->focusInEventRecieved);
    QVERIFY(childFocusWidgetOne->focusOutEventLostFocus);
    QVERIFY(childFocusWidgetTwo->focusInEventGotFocus);

    QVERIFY( childFocusWidgetTwo->hasFocus() );
    QCOMPARE( childFocusWidgetOne->focusOutEventReason, int(Qt::BacktabFocusReason) );
    QCOMPARE( childFocusWidgetTwo->focusInEventReason, int(Qt::BacktabFocusReason) );
#endif
}

void tst_QFocusEvent::checkReason_Popup()
{
    initWidget();

    // Now test the popup reason
    QMenu* popupMenu = new QMenu( testFocusWidget );
    popupMenu->addMenu( "Test" );
    popupMenu->popup( QPoint(0,0) );

    QTRY_VERIFY(childFocusWidgetOne->focusOutEventLostFocus);

    QTRY_VERIFY( childFocusWidgetOne->hasFocus() );
    QVERIFY( !childFocusWidgetOne->focusInEventRecieved );
    QVERIFY( childFocusWidgetOne->focusOutEventRecieved );
    QVERIFY( !childFocusWidgetTwo->focusInEventRecieved );
    QVERIFY( !childFocusWidgetTwo->focusOutEventRecieved );
    QCOMPARE( childFocusWidgetOne->focusOutEventReason, int(Qt::PopupFocusReason));

    popupMenu->hide();

    QVERIFY(childFocusWidgetOne->focusInEventRecieved);
    QVERIFY(childFocusWidgetOne->focusInEventGotFocus);

    QVERIFY( childFocusWidgetOne->hasFocus() );
    QVERIFY( childFocusWidgetOne->focusInEventRecieved );
    QVERIFY( childFocusWidgetOne->focusOutEventRecieved );
    QVERIFY( !childFocusWidgetTwo->focusInEventRecieved );
    QVERIFY( !childFocusWidgetTwo->focusOutEventRecieved );
}

#ifdef Q_OS_MAC
QT_BEGIN_NAMESPACE
    extern void qt_set_sequence_auto_mnemonic(bool);
QT_END_NAMESPACE
#endif

#if QT_CONFIG(shortcut)

void tst_QFocusEvent::checkReason_Shortcut()
{
    initWidget();
#ifdef Q_OS_MAC
    qt_set_sequence_auto_mnemonic(true);
#endif
    QLabel* label = new QLabel( "&Test", testFocusWidget );
    label->setBuddy( childFocusWidgetTwo );
    label->setGeometry( 10, 50, 90, 20 );
    childFocusWidgetTwo->setGeometry( 105, 50, 95, 20 );
    label->show();

    QVERIFY( childFocusWidgetOne->hasFocus() );
    QVERIFY( !childFocusWidgetTwo->hasFocus() );

    QTest::keyClick( label, Qt::Key_T, Qt::AltModifier );

    QVERIFY(childFocusWidgetOne->focusOutEventRecieved);
    QVERIFY(childFocusWidgetTwo->focusInEventRecieved);
    QVERIFY(childFocusWidgetOne->focusOutEventLostFocus);
    QVERIFY(childFocusWidgetTwo->focusInEventGotFocus);

    QVERIFY( childFocusWidgetTwo->hasFocus() );
    QVERIFY( !childFocusWidgetOne->focusInEventRecieved );
    QVERIFY( childFocusWidgetOne->focusOutEventRecieved );
    QCOMPARE( childFocusWidgetOne->focusOutEventReason, (int)Qt::ShortcutFocusReason );
    QVERIFY( childFocusWidgetTwo->focusInEventRecieved );
    QCOMPARE( childFocusWidgetTwo->focusInEventReason, (int)Qt::ShortcutFocusReason );
    QVERIFY( !childFocusWidgetTwo->focusOutEventRecieved );

    label->hide();
    QVERIFY( childFocusWidgetTwo->hasFocus() );
    QVERIFY( !childFocusWidgetOne->hasFocus() );
#ifdef Q_OS_MAC
    qt_set_sequence_auto_mnemonic(false);
#endif
}

#endif // QT_CONFIG(shortcut)

void tst_QFocusEvent::checkReason_focusWidget()
{
    // This test checks that a widget doesn't loose
    // its focuswidget just because the focuswidget loses focus.
    QWidget window1;
    QWidget frame1;
    QWidget frame2;
    QLineEdit edit1;
    QLineEdit edit2;

    QVBoxLayout outerLayout;
    outerLayout.addWidget(&frame1);
    outerLayout.addWidget(&frame2);
    window1.setLayout(&outerLayout);

    QVBoxLayout leftLayout;
    QVBoxLayout rightLayout;
    leftLayout.addWidget(&edit1);
    rightLayout.addWidget(&edit2);
    frame1.setLayout(&leftLayout);
    frame2.setLayout(&rightLayout);
    window1.show();
    QVERIFY(QTest::qWaitForWindowActive(&window1));

    edit1.setFocus();
    QTRY_VERIFY(edit1.hasFocus());
    edit2.setFocus();

    QVERIFY(frame1.focusWidget() != nullptr);
    QVERIFY(frame2.focusWidget() != nullptr);
}

void tst_QFocusEvent::checkReason_ActiveWindow()
{
    initWidget();

    QDialog* d = new QDialog( testFocusWidget );
    d->show();
    QVERIFY(QTest::qWaitForWindowExposed(d));
    QVERIFY(QTest::qWaitForWindowActive(d));

    QTRY_VERIFY(childFocusWidgetOne->focusOutEventRecieved);
    QVERIFY(childFocusWidgetOne->focusOutEventLostFocus);

#if defined(Q_OS_WIN)
    if (QSysInfo::kernelVersion() == "10.0.15063") {
        // Activate window of testFocusWidget, focus in that window goes to childFocusWidgetOne
        qWarning("Windows 10 Creators Update (10.0.15063) requires explicit activateWindow()");
        testFocusWidget->activateWindow();
    }
#endif

    QVERIFY( !childFocusWidgetOne->focusInEventRecieved );
    QVERIFY( childFocusWidgetOne->focusOutEventRecieved );
    QCOMPARE( childFocusWidgetOne->focusOutEventReason, (int)Qt::ActiveWindowFocusReason);
    QVERIFY( !childFocusWidgetOne->hasFocus() );

    d->hide();

    if (!QGuiApplication::platformName().compare(QLatin1String("offscreen"), Qt::CaseInsensitive)
            || !QGuiApplication::platformName().compare(QLatin1String("minimal"), Qt::CaseInsensitive)
            || !QGuiApplication::platformName().compare(QLatin1String("cocoa"), Qt::CaseInsensitive)) {
        // Activate window of testFocusWidget, focus in that window goes to childFocusWidgetOne
        qWarning("Platforms offscreen, minimal and macOS require explicit activateWindow()");
        testFocusWidget->activateWindow();
    }

    QTRY_VERIFY(childFocusWidgetOne->focusInEventRecieved);
    QVERIFY(childFocusWidgetOne->focusInEventGotFocus);

    QVERIFY( childFocusWidgetOne->hasFocus() );
    QVERIFY( childFocusWidgetOne->focusInEventRecieved );
    QCOMPARE( childFocusWidgetOne->focusInEventReason, (int)Qt::ActiveWindowFocusReason);

    const bool windowActivationReasonFail =
        QGuiApplication::platformName().toLower() == "minimal";

    struct Window : public QWindow
    {
        Qt::FocusReason lastReason = Qt::NoFocusReason;
    protected:
        void focusInEvent(QFocusEvent *event) override
        {
            lastReason = event->reason();
        }
        void focusOutEvent(QFocusEvent *event) override
        {
            lastReason = event->reason();
        }
    };

    Window window;
    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    if (windowActivationReasonFail)
        QEXPECT_FAIL("", "Platform doesn't set window activation reason for QWindow", Continue);
    QCOMPARE(window.lastReason, Qt::ActiveWindowFocusReason);
    window.lastReason = Qt::NoFocusReason;

    Window window2;
    window2.show();
    window2.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window2));

    if (windowActivationReasonFail)
        QEXPECT_FAIL("", "Platform doesn't set window activation reason for QWindow", Continue);
    QCOMPARE(window.lastReason, Qt::ActiveWindowFocusReason);
}


QTEST_MAIN(tst_QFocusEvent)
#include "tst_qfocusevent.moc"
