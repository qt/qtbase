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


#include <QtTest/QtTest>
#include <qapplication.h>
#include <qtextedit.h>
#include <qlineedit.h>
#include <qcompleter.h>
#include <qpushbutton.h>
#include <qmainwindow.h>
#include <qstatusbar.h>
#include <qboxlayout.h>
#include <qdebug.h>
#include <qstring.h>
#include <qshortcut.h>
#include <qscreen.h>

QT_BEGIN_NAMESPACE
class QMainWindow;
class QTextEdit;
QT_END_NAMESPACE

class tst_QShortcut : public QObject
{
    Q_OBJECT
public:

    enum Action {
        SetupAccel,
        TestAccel,
        ClearAll,
        TestEnd
    };
    Q_ENUM(Action)

    enum Widget {
        NoWidget,
        TriggerSlot1,
        TriggerSlot2,
        TriggerSlot3,
        TriggerSlot4,
        TriggerSlot5,
        TriggerSlot6,
        TriggerSlot7,
    };
    Q_ENUM(Widget)

    enum Result {
        NoResult,
        Slot1Triggered,
        Slot2Triggered,
        Slot3Triggered,
        Slot4Triggered,
        Slot5Triggered,
        Slot6Triggered,
        Slot7Triggered,
        SentKeyEvent,
        Ambiguous
    } currentResult = NoResult;
    Q_ENUM(Result)

public slots:
    void slotTrig1() { currentResult = Slot1Triggered; }
    void slotTrig2() { currentResult = Slot2Triggered; }
    void slotTrig3() { currentResult = Slot3Triggered; }
    void slotTrig4() { currentResult = Slot4Triggered; }
    void slotTrig5() { currentResult = Slot5Triggered; }
    void slotTrig6() { currentResult = Slot6Triggered; }
    void slotTrig7() { currentResult = Slot7Triggered; }
    void ambigSlot1() { currentResult = Ambiguous; ambigResult = Slot1Triggered; }
    void ambigSlot2() { currentResult = Ambiguous; ambigResult = Slot2Triggered; }
    void ambigSlot3() { currentResult = Ambiguous; ambigResult = Slot3Triggered; }
    void ambigSlot4() { currentResult = Ambiguous; ambigResult = Slot4Triggered; }
    void ambigSlot5() { currentResult = Ambiguous; ambigResult = Slot5Triggered; }
    void ambigSlot6() { currentResult = Ambiguous; ambigResult = Slot6Triggered; }
    void ambigSlot7() { currentResult = Ambiguous; ambigResult = Slot7Triggered; }

private slots:
    void cleanup();
    void pmf_connect();
    void number_data();
    void number();
    void text_data();
    void text();
    void disabledItems();
    void ambiguousItems();
    void ambiguousRotation();
    void keypressConsumption();
    void unicodeCompare();
    void context();
    void duplicatedShortcutOverride();
    void shortcutToFocusProxy();
    void deleteLater();

protected:
    static Qt::KeyboardModifiers toButtons( int key );
    void defElements();

    QShortcut *setupShortcut(QWidget *parent, const QString &name, const QKeySequence &ks,
                             Qt::ShortcutContext context = Qt::WindowShortcut);
    QShortcut *setupShortcut(QWidget *parent, const QString &name, Widget testWidget,
                             const QKeySequence &ks, Qt::ShortcutContext context = Qt::WindowShortcut);

    static void sendKeyEvents(QWidget *w, int k1, QChar c1 = 0, int k2 = 0, QChar c2 = 0,
                              int k3 = 0, QChar c3 = 0, int k4 = 0, QChar c4 = 0);

    void testElement();

    Result ambigResult;
};

class TestEdit : public QTextEdit
{
    Q_OBJECT
public:
    TestEdit(QWidget *parent, const char *name)
        : QTextEdit(parent)
    {
        setObjectName(name);
    }

protected:
    bool event(QEvent *e) override
    {
        // Make testedit allow any Ctrl+Key as shortcut
        if (e->type() == QEvent::ShortcutOverride) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(e);
            if (ke->modifiers() == Qt::ControlModifier
                && ke->key() > Qt::Key_Any
                && ke->key() < Qt::Key_ydiaeresis) {
                ke->ignore();
                return true;
            }
        }

        // If keypress not processed as normal, check for
        // Ctrl+Key event, and input custom string for
        // result comparison.
        if (e->type() == QEvent::KeyPress) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(e);
            if (ke->modifiers() && ke->key() > Qt::Key_Any
                && ke->key() < Qt::Key_ydiaeresis) {
                const QChar c = QLatin1Char(char(ke->key()));
                if (ke->modifiers() == Qt::ControlModifier)
                    insertPlainText(QLatin1String("<Ctrl+") + c + QLatin1Char('>'));
                else if (ke->modifiers() == Qt::AltModifier)
                    insertPlainText(QLatin1String("<Alt+") + c + QLatin1Char('>'));
                else if (ke->modifiers() == Qt::ShiftModifier)
                    insertPlainText(QLatin1String("<Shift+") + c + QLatin1Char('>'));
                return true;
            }
        }
        return QTextEdit::event(e);
    }
};

class MainWindow : public QMainWindow
{
public:
    MainWindow();

    TestEdit *testEdit() const { return m_testEdit; }

private:
    TestEdit *m_testEdit;
};

MainWindow::MainWindow()
{
    setWindowFlags(Qt::X11BypassWindowManagerHint);
    m_testEdit = new TestEdit(this, "test_edit");
    setCentralWidget(m_testEdit);
    setFixedSize(200, 200);
}

void tst_QShortcut::cleanup()
{
    QVERIFY(QApplication::topLevelWidgets().size() <= 1);  // The data driven tests keep a widget around
}

void tst_QShortcut::pmf_connect()
{
    class MyObject : public QObject
    {
    public:
        using QObject::QObject;
        void onActivated() { ++activated; }
        void onAmbiguous() { ++ambiguous; }
        void reset() { activated = 0; ambiguous = 0; }
        int activated = 0;
        int ambiguous = 0;
    } myObject;
    QWidget parent;

    auto runCheck = [&myObject](QShortcut *sc, int activated, int ambiguous)
    {
        myObject.reset();
        sc->activated();
        sc->activatedAmbiguously();
        delete sc;
        QCOMPARE(myObject.activated, activated);
        QCOMPARE(myObject.ambiguous, ambiguous);
    };

    runCheck(new QShortcut(QKeySequence(), &parent,
                           [&myObject]() { ++myObject.activated; }),
             1, 0);
    runCheck(new QShortcut(QKeySequence(), &parent,
                           &myObject, &MyObject::onActivated),
             1, 0);
    runCheck(new QShortcut(QKeySequence(), &parent,
                           &myObject, &MyObject::onActivated, &MyObject::onAmbiguous),
             1, 1);
    runCheck(new QShortcut(QKeySequence(), &parent, &myObject,
                           &MyObject::onActivated, &myObject, &MyObject::onAmbiguous),
             1, 1);
}


Qt::KeyboardModifiers tst_QShortcut::toButtons( int key )
{
    Qt::KeyboardModifiers result = Qt::NoModifier;
    if ( key & Qt::SHIFT )
        result |= Qt::ShiftModifier;
    if ( key & Qt::CTRL )
        result |= Qt::ControlModifier;
    if ( key & Qt::META )
        result |= Qt::MetaModifier;
    if ( key & Qt::ALT )
        result |= Qt::AltModifier;
    return result;
}

void tst_QShortcut::defElements()
{
    QTest::addColumn<tst_QShortcut::Action>("action");
    QTest::addColumn<tst_QShortcut::Widget>("testWidget");
    QTest::addColumn<QString>("txt");
    QTest::addColumn<int>("k1");
    QTest::addColumn<int>("c1");
    QTest::addColumn<int>("k2");
    QTest::addColumn<int>("c2");
    QTest::addColumn<int>("k3");
    QTest::addColumn<int>("c3");
    QTest::addColumn<int>("k4");
    QTest::addColumn<int>("c4");
    QTest::addColumn<tst_QShortcut::Result>("result");
}

void tst_QShortcut::number()
{
    // We expect a failure on these tests, until QtTestKeyboard is
    // fixed to do real platform dependent keyboard simulations
    if (QTest::currentDataTag() == QString("N006a:Shift+Tab - [BackTab]")
        || QTest::currentDataTag() == QString("N006b:Shift+Tab - [Shift+BackTab]"))
        QEXPECT_FAIL("", "FLAW IN QTESTKEYBOARD: Keyboard events not passed through "
                        "platform dependent key handling code", Continue);
    testElement();
}
void tst_QShortcut::text()
{
    testElement();
}
// ------------------------------------------------------------------
// Number Elements --------------------------------------------------
// ------------------------------------------------------------------
void tst_QShortcut::number_data()
{
    defElements();

    // Clear all
    QTest::newRow("N00 - clear") << ClearAll << NoWidget <<QString()<<0<<0<<0<<0<<0<<0<<0<<0<<NoResult;

    //===========================================
    // [Shift + key] on non-shift shortcuts testing
    //===========================================

    /* Testing Single Sequences
       Shift + Qt::Key_M    on  Qt::Key_M
               Qt::Key_M    on  Qt::Key_M
       Shift + Qt::Key_Plus on  Qt::Key_Pluss
               Qt::Key_Plus on  Qt::Key_Pluss
    */
    QTest::newRow("N001 - slot1")                   << SetupAccel << TriggerSlot1 << QString()    << int(Qt::Key_M) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    //commented out because the behaviour changed, those tests should be updated
    //QTest::newRow("N001:Shift + M - [M]")         << TestAccel << NoWidget << QString()         << int(Qt::SHIFT + Qt::Key_M) << int('M') << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("N001:M - [M]")                   << TestAccel << NoWidget << QString()         << int(Qt::Key_M) << int('m') << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("N001 - slot2")                   << SetupAccel << TriggerSlot2 << QString()    << int(Qt::Key_Plus) << int('+') << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    //commented out because the behaviour changed, those tests should be updated
    //QTest::newRow("N001:Shift++ [+]")             << TestAccel << NoWidget << QString()         << int(Qt::SHIFT + Qt::Key_Plus) << int('+') << 0 << 0 << 0 << 0 << 0 << 0 << Slot2Triggered;
    QTest::newRow("N001:+ [+]")                     << TestAccel << NoWidget << QString()         << int(Qt::Key_Plus) << int('+') << 0 << 0 << 0 << 0 << 0 << 0 << Slot2Triggered;
    QTest::newRow("N001 - clear")                   << ClearAll << NoWidget << QString()          << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult; // Clear all

    /* Testing Single Sequences
       Shift + Qt::Key_M    on  Shift + Qt::Key_M
               Qt::Key_M    on  Shift + Qt::Key_M
       Shift + Qt::Key_Plus on  Shift + Qt::Key_Pluss
               Qt::Key_Plus on  Shift + Qt::Key_Pluss
    */
    QTest::newRow("N002 - slot1")                   << SetupAccel << TriggerSlot1 << QString()    << int(Qt::SHIFT + Qt::Key_M) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("N002:Shift+M - [Shift+M]")       << TestAccel << NoWidget << QString()         << int(Qt::SHIFT + Qt::Key_M) << int('M') << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("N002:M - [Shift+M]")             << TestAccel << NoWidget << QString()         << int(Qt::Key_M) << int('m') << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("N002 - slot2")                   << SetupAccel << TriggerSlot2 << QString()    << int(Qt::SHIFT + Qt::Key_Plus) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("N002:Shift++ [Shift++]")         << TestAccel << NoWidget << QString()         << int(Qt::SHIFT + Qt::Key_Plus) << int('+') << 0 << 0 << 0 << 0 << 0 << 0 << Slot2Triggered;
    QTest::newRow("N002:+ [Shift++]")               << TestAccel << NoWidget << QString()         << int(Qt::Key_Plus) << int('+') << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("N002 - clear")                   << ClearAll << NoWidget << QString()          << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult; // Clear all

    /* Testing Single Sequences
       Shift + Qt::Key_F1   on  Qt::Key_F1
               Qt::Key_F1   on  Qt::Key_F1
    */
    QTest::newRow("N003 - slot1")                   << SetupAccel << TriggerSlot1 << QString()    << int(Qt::Key_F1) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    //commented out because the behaviour changed, those tests should be updated
    //QTest::newRow("N003:Shift+F1 - [F1]")           << TestAccel << NoWidget << QString()         << int(Qt::SHIFT + Qt::Key_F1) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("N003:F1 - [F1]")                 << TestAccel << NoWidget << QString()         << int(Qt::Key_F1) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("N003 - clear")                   << ClearAll << NoWidget << QString()          << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult; // Clear all
    /* Testing Single Sequences
       Shift + Qt::Key_F1   on  Shift + Qt::Key_F1
               Qt::Key_F1   on  Shift + Qt::Key_F1
    */

    QTest::newRow("N004 - slot1")                   << SetupAccel << TriggerSlot1 << QString()    << int(Qt::SHIFT + Qt::Key_F1) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("N004:Shift+F1 - [Shift+F1]")     << TestAccel << NoWidget << QString()         << int(Qt::SHIFT + Qt::Key_F1) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("N004:F1 - [Shift+F1]")           << TestAccel << NoWidget << QString()         << int(Qt::Key_F1) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("N004 - clear")                   << ClearAll << NoWidget << QString()          << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult; // Clear all

    /* Testing Single Sequences
               Qt::Key_Tab      on  Qt::Key_Tab
       Shift + Qt::Key_Tab      on  Qt::Key_Tab
               Qt::Key_Backtab  on  Qt::Key_Tab
       Shift + Qt::Key_Backtab  on  Qt::Key_Tab
    */
    QTest::newRow("N005a - slot1")                  << SetupAccel << TriggerSlot1 << QString()    << int(Qt::Key_Tab) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("N005a:Tab - [Tab]")              << TestAccel << NoWidget << QString()         << int(Qt::Key_Tab) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    //commented out because the behaviour changed, those tests should be updated
    //QTest::newRow("N005a:Shift+Tab - [Tab]")        << TestAccel << NoWidget << QString()         << int(Qt::SHIFT + Qt::Key_Tab) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    // (Shift+)BackTab != Tab, but Shift+BackTab == Shift+Tab
    QTest::newRow("N005a:Backtab - [Tab]")          << TestAccel << NoWidget << QString()         << int(Qt::Key_Backtab) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("N005a:Shift+Backtab - [Tab]")    << TestAccel << NoWidget << QString()         << int(Qt::SHIFT + Qt::Key_Backtab) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("N005a - clear")                  << ClearAll << NoWidget << QString()          << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult; // Clear all

    /* Testing Single Sequences
               Qt::Key_Tab      on  Shift + Qt::Key_Tab
       Shift + Qt::Key_Tab      on  Shift + Qt::Key_Tab
               Qt::Key_Backtab  on  Shift + Qt::Key_Tab
       Shift + Qt::Key_Backtab  on  Shift + Qt::Key_Tab
    */
    QTest::newRow("N005b - slot1")                     << SetupAccel << TriggerSlot1 << QString() << int(Qt::SHIFT + Qt::Key_Tab) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("N005b:Tab - [Shift+Tab]")           << TestAccel << NoWidget << QString()      << int(Qt::Key_Tab) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("N005b:Shift+Tab - [Shift+Tab]")     << TestAccel << NoWidget << QString()      << int(Qt::SHIFT + Qt::Key_Tab) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("N005b:BackTab - [Shift+Tab]")       << TestAccel << NoWidget << QString()      << int(Qt::Key_Backtab) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("N005b:Shift+BackTab - [Shift+Tab]") << TestAccel << NoWidget << QString()      << int(Qt::SHIFT + Qt::Key_Backtab) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("N005b - clear")                     << ClearAll << NoWidget << QString()       << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult; // Clear all

    /* Testing Single Sequences
               Qt::Key_Tab      on  Qt::Key_Backtab
       Shift + Qt::Key_Tab      on  Qt::Key_Backtab
               Qt::Key_Backtab  on  Qt::Key_Backtab
       Shift + Qt::Key_Backtab  on  Qt::Key_Backtab
    */
    QTest::newRow("N006a - slot1")                  << SetupAccel << TriggerSlot1 << QString()    << int(Qt::Key_Backtab) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("N006a:Tab - [BackTab]")          << TestAccel << NoWidget << QString()         << int(Qt::Key_Tab) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    // This should work, since platform dependent code will transform the
    // Shift+Tab into a Shift+BackTab, which should trigger the shortcut
    QTest::newRow("N006a:Shift+Tab - [BackTab]")    << TestAccel << NoWidget << QString()         << int(Qt::SHIFT + Qt::Key_Tab) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered; //XFAIL
    QTest::newRow("N006a:BackTab - [BackTab]")      << TestAccel << NoWidget << QString()         << int(Qt::Key_Backtab) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    //commented out because the behaviour changed, those tests should be updated
    //QTest::newRow("N006a:Shift+BackTab - [BackTab]")  << TestAccel << NoWidget << QString()       << int(Qt::SHIFT + Qt::Key_Backtab) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("N006a - clear")                  << ClearAll << NoWidget<< QString()           << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult; // Clear all

    /* Testing Single Sequences
               Qt::Key_Tab      on  Shift + Qt::Key_Backtab
       Shift + Qt::Key_Tab      on  Shift + Qt::Key_Backtab
               Qt::Key_Backtab  on  Shift + Qt::Key_Backtab
       Shift + Qt::Key_Backtab  on  Shift + Qt::Key_Backtab
    */
    QTest::newRow("N006b - slot1")                         << SetupAccel << TriggerSlot1 << QString()  << int(Qt::SHIFT + Qt::Key_Backtab) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("N006b:Tab - [Shift+BackTab]")           << TestAccel << NoWidget << QString()       << int(Qt::Key_Tab) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("N006b:Shift+Tab - [Shift+BackTab]")     << TestAccel << NoWidget << QString()       << int(Qt::SHIFT + Qt::Key_Tab) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("N006b:BackTab - [Shift+BackTab]")       << TestAccel << NoWidget << QString()       << int(Qt::Key_Backtab) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("N006b:Shift+BackTab - [Shift+BackTab]") << TestAccel << NoWidget << QString()       << int(Qt::SHIFT + Qt::Key_Backtab) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered; //XFAIL
    QTest::newRow("N006b - clear")                         << ClearAll << NoWidget << QString()        << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult; // Clear all

    //===========================================
    // [Shift + key] and [key] on shortcuts with
    // and without modifiers
    //===========================================

    /* Testing Single Sequences
       Qt::Key_F1
       Shift + Qt::Key_F1
    */
    QTest::newRow("N007 - slot1")                   << SetupAccel << TriggerSlot1 << QString()    << int(Qt::Key_F1) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("N007 - slot2")                   << SetupAccel << TriggerSlot2 << QString()    << int(Qt::SHIFT + Qt::Key_F1) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("N007:F1")                        << TestAccel << NoWidget << QString()         << int(Qt::Key_F1) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("N007:Shift + F1")                << TestAccel << NoWidget << QString()         << int(Qt::SHIFT + Qt::Key_F1) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot2Triggered;
    QTest::newRow("N007 - clear")                   << ClearAll << NoWidget << QString()          << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult; // Clear all

    /* Testing Single Sequences
       Qt::Key_M
       Shift + Qt::Key_M
       Ctrl  + Qt::Key_M
       Alt   + Qt::Key_M
    */
    QTest::newRow("N01 - slot1")                    << SetupAccel << TriggerSlot1 << QString()    << int(Qt::Key_M) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("N02 - slot2")                    << SetupAccel << TriggerSlot2 << QString()    << int(Qt::SHIFT + Qt::Key_M) << int('M') << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("N03 - slot1")                    << SetupAccel << TriggerSlot1 << QString()    << int(Qt::CTRL + Qt::Key_M) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("N04 - slot2")                    << SetupAccel << TriggerSlot2 << QString()    << int(Qt::ALT + Qt::Key_M) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("N:Qt::Key_M")                    << TestAccel << NoWidget << QString()         << int(Qt::Key_M) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("N:Shift+Qt::Key_M")              << TestAccel << NoWidget << QString()         << int(Qt::SHIFT + Qt::Key_M) << int('M') << 0 << 0 << 0 << 0 << 0 << 0 << Slot2Triggered;
    QTest::newRow("N:Ctrl+Qt::Key_M")               << TestAccel << NoWidget << QString()         << int(Qt::CTRL + Qt::Key_M) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("N:Alt+Qt::Key_M")                << TestAccel << NoWidget << QString()         << int(Qt::ALT + Qt::Key_M) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot2Triggered;

    /* Testing Single Sequence Ambiguity
       Qt::Key_M on shortcut2
    */
    QTest::newRow("N05 - slot2")                    << SetupAccel << TriggerSlot2 << QString()    << int(Qt::Key_M) << int('m') << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("N:Qt::Key_M on slot")            << TestAccel << NoWidget << QString()         << int(Qt::Key_M) << int('m') << 0 << 0 << 0 << 0 << 0 << 0 << Ambiguous;
    QTest::newRow("N05 - clear")                    << ClearAll << NoWidget << QString()          << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult; // Clear all

    /* Testing Single Specialkeys
       Qt::Key_aring
       Qt::Key_Aring
       Qt::UNICODE_ACCEL + Qt::Key_K
    */
    QTest::newRow("N06 - slot1")                    << SetupAccel << TriggerSlot1 << QString()    << int(Qt::Key_Aring) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("N07 - slot2")                    << SetupAccel << TriggerSlot2 << QString()    << int(Qt::SHIFT+Qt::Key_Aring) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("N08 - slot2")                    << SetupAccel << TriggerSlot1 << QString()    << int(Qt::UNICODE_ACCEL + Qt::Key_K) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;

    QTest::newRow("N:Qt::Key_aring")                << TestAccel << NoWidget << QString()         << int(Qt::Key_Aring) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("N:Qt::Key_Aring")                << TestAccel << NoWidget << QString()         << int(Qt::SHIFT+Qt::Key_Aring) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot2Triggered;
    QTest::newRow("N:Qt::Key_aring - Text Form")    << TestAccel << NoWidget << QString()         << 0 << 0xC5 << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("N:Qt::Key_Aring - Text Form")    << TestAccel << NoWidget << QString()         << int(Qt::SHIFT+0) << 0xC5 << 0 << 0 << 0 << 0 << 0 << 0 << Slot2Triggered;
    QTest::newRow("N:Qt::UNICODE_ACCEL + Qt::Key_K")<< TestAccel << NoWidget << QString()         << int(Qt::UNICODE_ACCEL + Qt::Key_K) << int('k') << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("N09 - clear")                    << ClearAll << NoWidget << QString()          << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult; // Clear all

    /* Testing Multiple Sequences
       Qt::Key_M
       Qt::Key_I, Qt::Key_M
       Shift+Qt::Key_I, Qt::Key_M
    */
    QTest::newRow("N10 - slot1")                    << SetupAccel << TriggerSlot1 << QString()    << int(Qt::Key_M) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("N11 - slot2")                    << SetupAccel << TriggerSlot2 << QString()    << int(Qt::Key_I) << 0 << int(Qt::Key_M) << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("N12 - slot1")                    << SetupAccel << TriggerSlot1 << QString()    << int(Qt::SHIFT + Qt::Key_I) << 0 << int(Qt::Key_M) << 0 << 0 << 0 << 0 << 0 << NoResult;

    QTest::newRow("N:Qt::Key_M (2)")                << TestAccel << NoWidget << QString()         << int(Qt::Key_M) << int('m') << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("N:Qt::Key_I, Qt::Key_M")         << TestAccel << NoWidget << QString()         << int(Qt::Key_I) << int('i') << int(Qt::Key_M) << int('m') << 0 << 0 << 0 << 0 << Slot2Triggered;
    QTest::newRow("N:Shift+Qt::Key_I, Qt::Key_M")   << TestAccel << NoWidget << QString()         << int(Qt::SHIFT + Qt::Key_I) << int('I') << int(Qt::Key_M) << int('m') << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("N:end") << TestEnd << NoWidget << QString() << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
}

// ------------------------------------------------------------------
// Text Elements ----------------------------------------------------
// ------------------------------------------------------------------
void tst_QShortcut::text_data()
{
    defElements();
    // Clear all
    QTest::newRow("T00 - clear") << ClearAll << NoWidget <<QString()<<0<<0<<0<<0<<0<<0<<0<<0<< NoResult;

    //===========================================
    // [Shift + key] on non-shift shortcuts testing
    //===========================================

    /* Testing Single Sequences
       Shift + Qt::Key_M    on  Qt::Key_M
               Qt::Key_M    on  Qt::Key_M
       Shift + Qt::Key_Plus on  Qt::Key_Pluss
               Qt::Key_Plus on  Qt::Key_Pluss
    */
    QTest::newRow("T001 - slot1")                   << SetupAccel << TriggerSlot1 << QString("M")   << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    //commented out because the behaviour changed, those tests should be updated
    //QTest::newRow("T001:Shift+M - [M]")           << TestAccel << NoWidget << QString()         << int(Qt::SHIFT + Qt::Key_M) << int('M') << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("T001:M - [M]")                   << TestAccel << NoWidget << QString()         << int(Qt::Key_M) << int('m') << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("T001 - slot2")                   << SetupAccel << TriggerSlot2 << QString("+")   << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    //commented out because the behaviour changed, those tests should be updated
    //QTest::newRow("T001:Shift++ [+]")             << TestAccel << NoWidget << QString()         << int(Qt::SHIFT + Qt::Key_Plus) << int('+') << 0 << 0 << 0 << 0 << 0 << 0 << Slot2Triggered;
    QTest::newRow("T001:+ [+]")                     << TestAccel << NoWidget << QString()         << int(Qt::Key_Plus) << int('+') << 0 << 0 << 0 << 0 << 0 << 0 << Slot2Triggered;
    QTest::newRow("T001 - clear")                   << ClearAll << NoWidget << QString()          << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult; // Clear all

    /* Testing Single Sequences
       Shift + Qt::Key_M    on  Shift + Qt::Key_M
               Qt::Key_M    on  Shift + Qt::Key_M
       Shift + Qt::Key_Plus on  Shift + Qt::Key_Pluss
               Qt::Key_Plus on  Shift + Qt::Key_Pluss
       Shift + Ctrl + Qt::Key_Plus on  Ctrl + Qt::Key_Pluss
               Ctrl + Qt::Key_Plus on  Ctrl + Qt::Key_Pluss
    */
    QTest::newRow("T002 - slot1")                   << SetupAccel << TriggerSlot1 << QString("Shift+M")   << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("T002:Shift+M - [Shift+M]")       << TestAccel << NoWidget << QString()               << int(Qt::SHIFT + Qt::Key_M) << int('M') << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("T002:M - [Shift+M]")             << TestAccel << NoWidget << QString()               << int(Qt::Key_M) << int('m') << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("T002 - slot2")                   << SetupAccel << TriggerSlot2 << QString("Shift++")   << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("T002:Shift++ [Shift++]")         << TestAccel << NoWidget << QString()               << int(Qt::SHIFT + Qt::Key_Plus) << int('+') << 0 << 0 << 0 << 0 << 0 << 0 << Slot2Triggered;
    QTest::newRow("T002:+ [Shift++]")               << TestAccel << NoWidget << QString()               << int(Qt::Key_Plus) << int('+') << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("T002 - clear")                   << ClearAll << NoWidget << QString()                << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult; // Clear all

    /* Testing Single Sequences
       Shift + Ctrl + Qt::Key_Plus on  Ctrl + Qt::Key_Plus
               Ctrl + Qt::Key_Plus on  Ctrl + Qt::Key_Plus
                      Qt::Key_Plus on  Ctrl + Qt::Key_Plus
    */
    QTest::newRow("T002b - slot1")                  << SetupAccel << TriggerSlot1 << QString("Ctrl++")    << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    //commented out because the behaviour changed, those tests should be updated
    //QTest::newRow("T002b:Shift+Ctrl++ [Ctrl++]")<< TestAccel << NoWidget << QString()                   << int(Qt::SHIFT + Qt::CTRL + Qt::Key_Plus) << int('+') << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("T002b:Ctrl++ [Ctrl++]")          << TestAccel << NoWidget << QString()               << int(Qt::CTRL + Qt::Key_Plus) << int('+') << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("T002b:+ [Ctrl++]")               << TestAccel << NoWidget << QString()               << int(Qt::Key_Plus) << int('+') << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("T002b - clear")                  << ClearAll << NoWidget << QString()                << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult; // Clear all

    /* Testing Single Sequences
       Shift + Qt::Key_F1   on  Qt::Key_F1
               Qt::Key_F1   on  Qt::Key_F1
    */
    QTest::newRow("T003 - slot1")                   << SetupAccel << TriggerSlot1 << QString("F1")        << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    //commented out because the behaviour changed, those tests should be updated
    //QTest::newRow("T003:Shift+F1 - [F1]")           << TestAccel << NoWidget << QString()               << int(Qt::SHIFT + Qt::Key_F1) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("T003:F1 - [F1]")                 << TestAccel << NoWidget << QString()               << int(Qt::Key_F1) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("T003 - clear")                   << ClearAll << NoWidget << QString()                << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult; // Clear all

    /* Testing Single Sequences
       Shift + Qt::Key_F1   on  Shift + Qt::Key_F1
               Qt::Key_F1   on  Shift + Qt::Key_F1
    */
    QTest::newRow("T004 - slot1")                   << SetupAccel << TriggerSlot1 << QString("Shift+F1")  << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("T004:Shift+F1 - [Shift+F1]")     << TestAccel << NoWidget << QString()               << int(Qt::SHIFT + Qt::Key_F1) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("T004:F1 - [Shift+F1]")           << TestAccel << NoWidget << QString()               << int(Qt::Key_F1) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("T004 - clear")                   << ClearAll << NoWidget << QString()                << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult; // Clear all

    //===========================================
    // [Shift + key] and [key] on shortcuts with
    // and without modifiers
    //===========================================

    /* Testing Single Sequences
       Qt::Key_F1
       Shift + Qt::Key_F1
    */
    QTest::newRow("T007 - slot1")                   << SetupAccel << TriggerSlot1 << QString("F1")        << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("T007 - slot2")                   << SetupAccel << TriggerSlot2 << QString("Shift+F1")  << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("T007:F1")                        << TestAccel << NoWidget << QString()               << int(Qt::Key_F1) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("T007:Shift + F1")                << TestAccel << NoWidget << QString()               << int(Qt::SHIFT + Qt::Key_F1) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot2Triggered;
    QTest::newRow("T007 - clear")                   << ClearAll << NoWidget << QString()                << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult; // Clear all

    /* Testing Single Sequences
       Qt::Key_M
       Shift + Qt::Key_M
       Ctrl  + Qt::Key_M
       Alt   + Qt::Key_M
    */
    QTest::newRow("T01 - slot1")                    << SetupAccel << TriggerSlot1 << QString("M")         << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("T02 - slot2")                    << SetupAccel << TriggerSlot2 << QString("Shift+M")   << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("T03 - slot1")                    << SetupAccel << TriggerSlot1 << QString("Ctrl+M")    << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("T04 - slot2")                    << SetupAccel << TriggerSlot2 << QString("Alt+M")     << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;

    QTest::newRow("T:Qt::Key_M")                    << TestAccel << NoWidget << QString()               << int(Qt::Key_M) << int('m') << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("T:Shift + Qt::Key_M")            << TestAccel << NoWidget << QString()               << int(Qt::SHIFT + Qt::Key_M) << int('M') << 0 << 0 << 0 << 0 << 0 << 0 << Slot2Triggered;
    QTest::newRow("T:Ctrl + Qt::Key_M")             << TestAccel << NoWidget << QString()               << int(Qt::CTRL + Qt::Key_M) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("T:Alt + Qt::Key_M")              << TestAccel << NoWidget << QString()               << int(Qt::ALT + Qt::Key_M) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot2Triggered;

    /* Testing Single Sequence Ambiguity
       Qt::Key_M on shortcut2
    */
    QTest::newRow("T05 - slot2")                    << SetupAccel << TriggerSlot2 << QString("M")         << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("T:Qt::Key_M on TriggerSlot2")    << TestAccel << NoWidget << QString()               << int(Qt::Key_M) << int('m') << 0 << 0 << 0 << 0 << 0 << 0 << Ambiguous;
    QTest::newRow("T06 - clear")                    << ClearAll << NoWidget << QString()                << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult; // Clear all

    /* Testing Single Specialkeys
       Qt::Key_aring
       Qt::Key_Aring
       Qt::UNICODE_ACCEL + Qt::Key_K
    */
    /* see comments above on the #ifdef'ery */
    QTest::newRow("T06 - slot1")                    << SetupAccel << TriggerSlot1 << QString::fromLatin1("\x0C5")<< 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("T07 - slot2")                    << SetupAccel << TriggerSlot2 << QString::fromLatin1("Shift+\x0C5")<< 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("T08 - slot2")                    << SetupAccel << TriggerSlot1 << QString("K")         << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("T:Qt::Key_aring")                << TestAccel << NoWidget << QString()               << int(Qt::Key_Aring) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("T:Qt::Key_Aring")                << TestAccel << NoWidget << QString()               << int(Qt::SHIFT+Qt::Key_Aring) << 0 << 0 << 0 << 0 << 0 << 0 << 0 << Slot2Triggered;
    QTest::newRow("T:Qt::Key_aring - Text Form")    << TestAccel << NoWidget << QString()               << 0 << 0xC5 << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("T:Qt::Key_Aring - Text Form")    << TestAccel << NoWidget << QString()               << int(Qt::SHIFT+0) << 0xC5 << 0 << 0 << 0 << 0 << 0 << 0 << Slot2Triggered;
    QTest::newRow("T:Qt::UNICODE_ACCEL + Qt::Key_K")<< TestAccel << NoWidget << QString()               << int(Qt::UNICODE_ACCEL + Qt::Key_K) << int('k') << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("T09 - clear")                    << ClearAll << NoWidget << QString()                << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult; // Clear all

    /* Testing Multiple Sequences
       Qt::Key_M
       Qt::Key_I, Qt::Key_M
       Shift+Qt::Key_I, Qt::Key_M
    */
    QTest::newRow("T10 - slot1")                    << SetupAccel << TriggerSlot1 << QString("M")         << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("T11 - slot2")                    << SetupAccel << TriggerSlot2 << QString("I, M")<< 0  << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("T12 - slot1")                    << SetupAccel << TriggerSlot1 << QString("Shift+I, M")<< 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
    QTest::newRow("T:Qt::Key_M (2)")                << TestAccel << NoWidget << QString()               << int(Qt::Key_M) << int('m') << 0 << 0 << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("T:Qt::Key_I, Qt::Key_M")         << TestAccel << NoWidget << QString()               << int(Qt::Key_I) << int('i') << int(Qt::Key_M) << int('m') << 0 << 0 << 0 << 0 << Slot2Triggered;
    QTest::newRow("T:Shift+Qt::Key_I, Qt::Key_M")   << TestAccel << NoWidget << QString()               << int(Qt::SHIFT + Qt::Key_I) << int('I') << int(Qt::Key_M) << int('m') << 0 << 0 << 0 << 0 << Slot1Triggered;
    QTest::newRow("T:end") << TestEnd << NoWidget << QString() << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << NoResult;
}

class ButtonWidget : public QWidget
{
public:
    ButtonWidget();

    QPushButton *pushButton1() const { return m_pb1; }
    QPushButton *pushButton2() const { return m_pb2; }

private:
    QPushButton *m_pb1;
    QPushButton *m_pb2;
};

ButtonWidget::ButtonWidget()
{
    // Setup two identical shortcuts on different pushbuttons
    QString name = QLatin1String("pushbutton-1");
    m_pb1 = new QPushButton(name, this);
    m_pb1->setObjectName(name);
    name = QLatin1String("pushbutton-2");
    m_pb2 = new QPushButton(name, this);
    m_pb2->setObjectName(name);
    auto hLayout = new QHBoxLayout(this);
    hLayout->addWidget(m_pb1);
    hLayout->addWidget(m_pb2);
    hLayout->addStretch();
}

// ------------------------------------------------------------------
// Disabled Elements ------------------------------------------------
// ------------------------------------------------------------------
void tst_QShortcut::disabledItems()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    ButtonWidget mainW;
    mainW.setWindowTitle(QTest::currentTestFunction());
    mainW.show();
    mainW.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&mainW));

    /* Testing Disabled Shortcuts
       Qt::Key_M          on slot1
       Shift + Qt::Key_M  on slot1
       Qt::Key_M          on slot2 (disabled)
       Shift + Qt::Key_M  on slot2 (disabled)
    */

    // Setup two identical shortcuts on different pushbuttons
    auto pb1 = mainW.pushButton1();
    auto pb2 = mainW.pushButton2();
    const int shiftM = Qt::SHIFT + Qt::Key_M;
    QShortcut *cut1 = setupShortcut(pb1, "shortcut1-pb1", TriggerSlot1,
                                    QKeySequence(Qt::Key_M));
    QShortcut *cut2 = setupShortcut(pb1, "shortcut2-pb1", TriggerSlot1,
                                    QKeySequence(shiftM));
    QShortcut *cut3 = setupShortcut(pb2, "shortcut3-pb2", TriggerSlot2,
                                    QKeySequence(Qt::Key_M));
    QShortcut *cut4 = setupShortcut(pb2, "shortcut4-pb2", TriggerSlot2,
                                    QKeySequence(shiftM));

    cut3->setEnabled(false);
    cut4->setEnabled(false);

    currentResult = NoResult;
    sendKeyEvents(&mainW, Qt::Key_M, 'm');
    QCOMPARE(currentResult, Slot1Triggered);

    currentResult = NoResult;
    sendKeyEvents(&mainW, shiftM, 'M');
    QCOMPARE(currentResult, Slot1Triggered);

    cut2->setEnabled(false);
    cut4->setEnabled(true);

    /* Testing Disabled Shortcuts
       Qt::Key_M          on slot1
       Shift + Qt::Key_M  on slot1 (disabled)
       Qt::Key_M          on slot2 (disabled)
       Shift + Qt::Key_M  on slot2
    */
    currentResult = NoResult;
    sendKeyEvents(&mainW, Qt::Key_M, 'm' );
    QCOMPARE( currentResult, Slot1Triggered );

    currentResult = NoResult;
    sendKeyEvents(&mainW, shiftM, 'M' );
    QCOMPARE( currentResult, Slot2Triggered );


    /* Testing Disabled Accel
       Qt::Key_F5          on slot1
       Shift + Qt::Key_F5  on slot2 (disabled)
    */
    qDeleteAll(mainW.findChildren<QShortcut *>());
    const int shiftF5 = Qt::SHIFT + Qt::Key_F5;
    cut1 = setupShortcut(pb1, "shortcut1-pb1", TriggerSlot1, QKeySequence(Qt::Key_F5));
    cut4 = setupShortcut(pb2, "shortcut4-pb2", TriggerSlot2, QKeySequence(shiftF5));

    cut1->setEnabled(true);
    cut4->setEnabled(false);

    currentResult = NoResult;
    sendKeyEvents(&mainW, Qt::Key_F5, 0 );
    QCOMPARE( currentResult, Slot1Triggered );

    currentResult = NoResult;
    sendKeyEvents(&mainW, shiftF5, 0 );
    QCOMPARE( currentResult, NoResult );
}
// ------------------------------------------------------------------
// Ambiguous Elements -----------------------------------------------
// ------------------------------------------------------------------
void tst_QShortcut::ambiguousRotation()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    MainWindow mainW;
    const QString name = QLatin1String(QTest::currentTestFunction());
    mainW.setWindowTitle(name);
    mainW.show();
    mainW.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&mainW));

    /* Testing Shortcut rotation scheme
       Ctrl + Qt::Key_A   on slot1 (disabled)
       Ctrl + Qt::Key_A   on slot2 (disabled)
       Ctrl + Qt::Key_A   on slot3
       Ctrl + Qt::Key_A   on slot4
       Ctrl + Qt::Key_A   on slot5 (disabled)
       Ctrl + Qt::Key_A   on slot6
       Ctrl + Qt::Key_A   on slot7 (disabled)
    */
    const int ctrlA = Qt::CTRL + Qt::Key_A;
    QKeySequence ctrlA_Sequence(ctrlA);
    QShortcut *cut1 = setupShortcut(&mainW, name, TriggerSlot1, ctrlA_Sequence);
    QShortcut *cut2 = setupShortcut(&mainW, name, TriggerSlot2, ctrlA_Sequence);
    QShortcut *cut3 = setupShortcut(&mainW, name, TriggerSlot3, ctrlA_Sequence);
    QShortcut *cut4 = setupShortcut(&mainW, name, TriggerSlot4, ctrlA_Sequence);
    QShortcut *cut5 = setupShortcut(&mainW, name, TriggerSlot5, ctrlA_Sequence);
    QShortcut *cut6 = setupShortcut(&mainW, name, TriggerSlot6, ctrlA_Sequence);
    QShortcut *cut7 = setupShortcut(&mainW, name, TriggerSlot7, ctrlA_Sequence);

    cut1->setEnabled(false);
    cut2->setEnabled(false);
    cut5->setEnabled(false);
    cut7->setEnabled(false);

    // Test proper rotation
    //   Start on first
    //   Go to last
    //   Go back to first
    //   Continue...
    currentResult = NoResult;
    ambigResult = NoResult;
    sendKeyEvents(&mainW, ctrlA);
    QCOMPARE(currentResult, Ambiguous);
    QCOMPARE(ambigResult, Slot3Triggered);

    currentResult = NoResult;
    ambigResult = NoResult;
    sendKeyEvents(&mainW, ctrlA);
    QCOMPARE(currentResult, Ambiguous);
    QCOMPARE(ambigResult, Slot4Triggered);

    currentResult = NoResult;
    ambigResult = NoResult;
    sendKeyEvents(&mainW, ctrlA);
    QCOMPARE(currentResult, Ambiguous);
    QCOMPARE(ambigResult, Slot6Triggered);

    currentResult = NoResult;
    ambigResult = NoResult;
    sendKeyEvents(&mainW, ctrlA);
    QCOMPARE(currentResult, Ambiguous);
    QCOMPARE(ambigResult, Slot3Triggered);

    currentResult = NoResult;
    ambigResult = NoResult;
    sendKeyEvents(&mainW, ctrlA);
    QCOMPARE(currentResult, Ambiguous);
    QCOMPARE(ambigResult, Slot4Triggered);

    currentResult = NoResult;
    ambigResult = NoResult;
    sendKeyEvents(&mainW, ctrlA);
    QCOMPARE(currentResult, Ambiguous);
    QCOMPARE(ambigResult, Slot6Triggered);

    /* Testing Shortcut rotation scheme
       Ctrl + Qt::Key_A   on slot1
       Ctrl + Qt::Key_A   on slot2
       Ctrl + Qt::Key_A   on slot3 (disabled)
       Ctrl + Qt::Key_A   on slot4 (disabled)
       Ctrl + Qt::Key_A   on slot5
       Ctrl + Qt::Key_A   on slot6 (disabled)
       Ctrl + Qt::Key_A   on slot7
    */

    cut1->setEnabled(true);
    cut2->setEnabled(true);
    cut5->setEnabled(true);
    cut7->setEnabled(true);

    cut3->setEnabled(false);
    cut4->setEnabled(false);
    cut6->setEnabled(false);

    currentResult = NoResult;
    ambigResult = NoResult;
    sendKeyEvents(&mainW, ctrlA);
    QCOMPARE(currentResult, Ambiguous);
    QCOMPARE(ambigResult, Slot1Triggered);

    currentResult = NoResult;
    ambigResult = NoResult;
    sendKeyEvents(&mainW, ctrlA);
    QCOMPARE(currentResult, Ambiguous);
    QCOMPARE(ambigResult, Slot2Triggered);

    currentResult = NoResult;
    ambigResult = NoResult;
    sendKeyEvents(&mainW, ctrlA);
    QCOMPARE(currentResult, Ambiguous);
    QCOMPARE(ambigResult, Slot5Triggered);

    currentResult = NoResult;
    ambigResult = NoResult;
    sendKeyEvents(&mainW, ctrlA);
    QCOMPARE(currentResult, Ambiguous);
    QCOMPARE(ambigResult, Slot7Triggered);

    currentResult = NoResult;
    ambigResult = NoResult;
    sendKeyEvents(&mainW, ctrlA);
    QCOMPARE(currentResult, Ambiguous);
    QCOMPARE(ambigResult, Slot1Triggered);

    currentResult = NoResult;
    ambigResult = NoResult;
    sendKeyEvents(&mainW, ctrlA);
    QCOMPARE(currentResult, Ambiguous);
    QCOMPARE(ambigResult, Slot2Triggered);

    currentResult = NoResult;
    ambigResult = NoResult;
    sendKeyEvents(&mainW, ctrlA);
    QCOMPARE(currentResult, Ambiguous);
    QCOMPARE(ambigResult, Slot5Triggered);

    currentResult = NoResult;
    ambigResult = NoResult;
    sendKeyEvents(&mainW, ctrlA);
    QCOMPARE(currentResult, Ambiguous);
    QCOMPARE(ambigResult, Slot7Triggered);
}

void tst_QShortcut::ambiguousItems()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    ButtonWidget mainW;
    mainW.setWindowTitle(QTest::currentTestFunction());
    mainW.show();
    mainW.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&mainW));

    /* Testing Ambiguous Shortcuts
       Qt::Key_M  on Pushbutton 1
       Qt::Key_M  on Pushbutton 2
    */

    // Setup two identical shortcuts on different pushbuttons
    auto pb1 = mainW.pushButton1();
    auto pb2 = mainW.pushButton2();

    setupShortcut(pb1, "shortcut1-pb1", TriggerSlot1, QKeySequence(Qt::Key_M));
    setupShortcut(pb2, "shortcut2-pb2", TriggerSlot2, QKeySequence(Qt::Key_M));

    currentResult = NoResult;
    sendKeyEvents(&mainW, Qt::Key_M, 'm' );
    QCOMPARE( currentResult, Ambiguous );
    QCOMPARE( ambigResult, Slot1Triggered );

    currentResult = NoResult;
    sendKeyEvents(&mainW, Qt::Key_M, 'm' );
    QCOMPARE( currentResult, Ambiguous );
    QCOMPARE( ambigResult, Slot2Triggered );

    currentResult = NoResult;
    sendKeyEvents(&mainW, Qt::Key_M, 'm' );
    QCOMPARE( currentResult, Ambiguous );
    QCOMPARE( ambigResult, Slot1Triggered );
}


// ------------------------------------------------------------------
// Unicode and non-unicode Elements ---------------------------------
// ------------------------------------------------------------------
void tst_QShortcut::unicodeCompare()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    ButtonWidget mainW;
    mainW.setWindowTitle(QTest::currentTestFunction());
    mainW.show();
    mainW.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&mainW));

    /* Testing Unicode/non-Unicode Shortcuts
       Qt::Key_M  on Pushbutton 1
       Qt::Key_M  on Pushbutton 2
    */
    auto pb1 = mainW.pushButton1();
    auto pb2 = mainW.pushButton2();

    QKeySequence ks1("Ctrl+M");     // Unicode
    QKeySequence ks2(Qt::CTRL+Qt::Key_M);   // non-Unicode
    setupShortcut(pb1, "shortcut1-pb1", TriggerSlot1, ks1);
    setupShortcut(pb2, "shortcut2-pb2", TriggerSlot2, ks2);

    currentResult = NoResult;
    sendKeyEvents(&mainW, Qt::CTRL + Qt::Key_M, 0);
    QCOMPARE( currentResult, Ambiguous );
    // They _are_ ambiguous, so the QKeySequence operator==
    // should indicate the same
    QVERIFY( ks1 == ks2 );
    QVERIFY( !(ks1 != ks2) );
}

// ------------------------------------------------------------------
// Keypress consumption verification --------------------------------
// ------------------------------------------------------------------
void tst_QShortcut::keypressConsumption()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    MainWindow mainW;
    mainW.setWindowTitle(QTest::currentTestFunction());
    mainW.show();
    mainW.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&mainW));
    auto edit = mainW.testEdit();

    const int ctrlI = Qt::CTRL + Qt::Key_I;
    QShortcut *cut1 = setupShortcut(edit, "shortcut1-line", TriggerSlot1, QKeySequence(ctrlI, Qt::Key_A));
    QShortcut *cut2 = setupShortcut(edit, "shortcut1-line", TriggerSlot2, QKeySequence(ctrlI, Qt::Key_B));

    currentResult = NoResult;
    ambigResult = NoResult;
    sendKeyEvents(edit, ctrlI, 0);   // Send key to edit
    QCOMPARE( currentResult, NoResult );
    QCOMPARE( ambigResult, NoResult );
    QCOMPARE(edit->toPlainText(), QString());

    // Make sure next keypress is eaten (failing multiple keysequence)
    sendKeyEvents(edit, Qt::Key_C, 'c');         // Send key to edit
    QCOMPARE( currentResult, NoResult );
    QCOMPARE( ambigResult, NoResult );
    QCOMPARE(edit->toPlainText(), QString());

    // Next keypress should be normal
    sendKeyEvents(edit, Qt::Key_C, 'c');         // Send key to edit
    QCOMPARE( currentResult, NoResult );
    QCOMPARE( ambigResult, NoResult );
    QCOMPARE(edit->toPlainText(), QString("c"));

    currentResult = NoResult;
    ambigResult = NoResult;
    edit->clear();
    QCOMPARE(edit->toPlainText().size(), 0);

    cut1->setEnabled(false);
    cut2->setEnabled(false);

    // Make sure keypresses is passed on, since all multiple keysequences
    // with Ctrl+I are disabled
    sendKeyEvents(edit, Qt::CTRL + Qt::Key_I, 0);   // Send key to edit
    QCOMPARE( currentResult, NoResult );
    QCOMPARE( ambigResult, NoResult );
    QVERIFY(edit->toPlainText().endsWith("<Ctrl+I>"));

    sendKeyEvents(edit, Qt::Key_A, 'a');         // Send key to edit
    QCOMPARE( currentResult, NoResult );
    QCOMPARE( ambigResult, NoResult );
    QVERIFY(edit->toPlainText().endsWith("<Ctrl+I>a"));

    qDeleteAll(mainW.findChildren<QShortcut *>());
    edit->clear();
    QCOMPARE(edit->toPlainText().size(), 0);

    auto cut = setupShortcut(edit, QLatin1String("first"), QKeySequence(Qt::CTRL + Qt::Key_A));
    connect(cut, &QShortcut::activated, edit, [this, edit] () {
        this->sendKeyEvents(edit, Qt::CTRL + Qt::Key_B, 0);
        this->currentResult = tst_QShortcut::SentKeyEvent;
    });

    // Verify reentrancy when a non-shortcut is triggered as part
    // of shortcut processing.
    currentResult = NoResult;
    ambigResult = NoResult;
    sendKeyEvents(edit, Qt::CTRL + Qt::Key_A, 0);
    QCOMPARE(currentResult, SentKeyEvent);
    QCOMPARE(ambigResult, NoResult);
    QCOMPARE(edit->toPlainText(), QLatin1String("<Ctrl+B>"));
}

// ------------------------------------------------------------------
// Context Validation -----------------------------------------------
// ------------------------------------------------------------------
void tst_QShortcut::context()
{
    const QString name = QLatin1String(QTest::currentTestFunction());
    MainWindow mainW;
    mainW.setWindowTitle(name + QLatin1String("_Helper"));
    mainW.show();
    auto edit = mainW.testEdit();

    QWidget myBox;
    myBox.setWindowTitle(name);
    auto other1 = new TestEdit(&myBox, "test_edit_other1");
    auto other2 = new TestEdit(&myBox, "test_edit_other2");
    auto layout = new QHBoxLayout(&myBox);
    layout->addWidget(other1);
    layout->addWidget(other2);
    myBox.show();
    QVERIFY(QTest::qWaitForWindowExposed(&myBox));

    setupShortcut(other1, "ActiveWindow", TriggerSlot1, QKeySequence("Alt+1"), Qt::WindowShortcut);
    setupShortcut(other2, "Focus",        TriggerSlot2, QKeySequence("Alt+2"), Qt::WidgetShortcut);
    setupShortcut(edit,   "Application",  TriggerSlot3, QKeySequence("Alt+3"), Qt::ApplicationShortcut);

    currentResult = NoResult;
    ambigResult = NoResult;
    edit->clear();
    other1->clear();
    other2->clear();

    // edit doesn't have focus, so ActiveWindow context should work
    // ..but Focus context shouldn't..
    // Changing focus to edit should make focus context work
    // Application context should always work


    // Focus on 'other1' edit, so Active Window context should trigger
    other1->activateWindow(); // <---
    QApplication::setActiveWindow(other1);
    QCOMPARE(QApplication::activeWindow(), other1->window());
    QCOMPARE(QApplication::focusWidget(), static_cast<QWidget *>(other1));

    currentResult = NoResult;
    ambigResult = NoResult;
    edit->clear();
    other1->clear();
    other2->clear();

    QCOMPARE(QApplication::focusWidget(), static_cast<QWidget *>(other1));
    sendKeyEvents(other1, Qt::ALT+Qt::Key_1);
    QCOMPARE(currentResult, Slot1Triggered);
    QCOMPARE(ambigResult, NoResult);
    QCOMPARE(edit->toPlainText(), QString());
    QCOMPARE(other1->toPlainText(), QString());
    QCOMPARE(other2->toPlainText(), QString());

    // ..but not Focus context on 'other2'..
    currentResult = NoResult;
    ambigResult = NoResult;
    edit->clear();
    other1->clear();
    other2->clear();

    sendKeyEvents(other1, Qt::ALT+Qt::Key_2);
    QCOMPARE(currentResult, NoResult);
    QCOMPARE(ambigResult, NoResult);
    QCOMPARE(edit->toPlainText(), QString());
    QCOMPARE(other1->toPlainText(), QString("<Alt+2>"));
    QCOMPARE(other2->toPlainText(), QString());

    // ..however, application global context on 'edit' should..
    currentResult = NoResult;
    ambigResult = NoResult;
    edit->clear();
    other1->clear();
    other2->clear();

    sendKeyEvents(other1, Qt::ALT+Qt::Key_3);
    QCOMPARE(currentResult, Slot3Triggered);
    QCOMPARE(ambigResult, NoResult);
    QCOMPARE(edit->toPlainText(), QString());
    QCOMPARE(other1->toPlainText(), QString());
    QCOMPARE(other2->toPlainText(), QString());

    // Changing focus to 'other2' should make the Focus context there work
    other2->activateWindow();
    other2->setFocus(); // ###
    QTRY_COMPARE(QApplication::activeWindow(), other2->window());
    QCOMPARE(QApplication::focusWidget(), static_cast<QWidget *>(other2));

    currentResult = NoResult;
    ambigResult = NoResult;
    edit->clear();
    other1->clear();
    other2->clear();

    sendKeyEvents(other2, Qt::ALT+Qt::Key_2);
    QCOMPARE(currentResult, Slot2Triggered);
    QCOMPARE(ambigResult, NoResult);
    QCOMPARE(edit->toPlainText(), QString());
    QCOMPARE(other1->toPlainText(), QString());
    QCOMPARE(other2->toPlainText(), QString());
}

// QTBUG-38986, do not generate duplicated QEvent::ShortcutOverride in event processing.
class OverrideCountingWidget : public QWidget
{
public:
    using QWidget::QWidget;

    int overrideCount = 0;

    bool event(QEvent *e) override
    {
        if (e->type() == QEvent::ShortcutOverride)
            overrideCount++;
        return QWidget::event(e);
    }
};

void tst_QShortcut::duplicatedShortcutOverride()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    OverrideCountingWidget w;
    w.setWindowTitle(Q_FUNC_INFO);
    w.resize(200, 200);
    w.move(QGuiApplication::primaryScreen()->availableGeometry().center() - QPoint(100, 100));
    w.show();
    QApplication::setActiveWindow(&w);
    QVERIFY(QTest::qWaitForWindowActive(&w));
    QTest::keyPress(w.windowHandle(), Qt::Key_A);
    QCoreApplication::processEvents();
    QCOMPARE(w.overrideCount, 1);
}

QShortcut *tst_QShortcut::setupShortcut(QWidget *parent, const QString &name, const QKeySequence &ks, Qt::ShortcutContext context)
{
    // Set up shortcut for next test
    auto cut = new QShortcut(ks, parent, nullptr, nullptr, context);
    cut->setObjectName(name);
    return cut;
}

QShortcut *tst_QShortcut::setupShortcut(QWidget *parent, const QString &name, Widget testWidget,
                                        const QKeySequence &ks, Qt::ShortcutContext context)
{
    // Set up shortcut for next test
    auto cut = setupShortcut(parent, name, ks, context);

    switch (testWidget) {
    case TriggerSlot1:
        connect(cut, &QShortcut::activated, this, &tst_QShortcut::slotTrig1);
        connect(cut, &QShortcut::activatedAmbiguously, this, &tst_QShortcut::ambigSlot1);
        break;
    case TriggerSlot2:
        connect(cut, &QShortcut::activated, this, &tst_QShortcut::slotTrig2);
        connect(cut, &QShortcut::activatedAmbiguously, this, &tst_QShortcut::ambigSlot2);
        break;
    case TriggerSlot3:
        connect(cut, &QShortcut::activated, this, &tst_QShortcut::slotTrig3);
        connect(cut, &QShortcut::activatedAmbiguously, this, &tst_QShortcut::ambigSlot3);
        break;
    case TriggerSlot4:
        connect(cut, &QShortcut::activated, this, &tst_QShortcut::slotTrig4);
        connect(cut, &QShortcut::activatedAmbiguously, this, &tst_QShortcut::ambigSlot4);
        break;
    case TriggerSlot5:
        connect(cut, &QShortcut::activated, this, &tst_QShortcut::slotTrig5);
        connect(cut, &QShortcut::activatedAmbiguously, this, &tst_QShortcut::ambigSlot5);
        break;
    case TriggerSlot6:
        connect(cut, &QShortcut::activated, this, &tst_QShortcut::slotTrig6);
        connect(cut, &QShortcut::activatedAmbiguously, this, &tst_QShortcut::ambigSlot6);
        break;
    case TriggerSlot7:
        connect(cut, &QShortcut::activated, this, &tst_QShortcut::slotTrig7);
        connect(cut, &QShortcut::activatedAmbiguously, this, &tst_QShortcut::ambigSlot7);
        break;
    case NoWidget:
        break;
    }
    return cut;
}

void tst_QShortcut::sendKeyEvents(QWidget *w, int k1, QChar c1, int k2, QChar c2, int k3, QChar c3, int k4, QChar c4)
{
    Qt::KeyboardModifiers b1 = toButtons( k1 );
    Qt::KeyboardModifiers b2 = toButtons( k2 );
    Qt::KeyboardModifiers b3 = toButtons( k3 );
    Qt::KeyboardModifiers b4 = toButtons( k4 );
    k1 &= ~Qt::MODIFIER_MASK;
    k2 &= ~Qt::MODIFIER_MASK;
    k3 &= ~Qt::MODIFIER_MASK;
    k4 &= ~Qt::MODIFIER_MASK;


    if (k1 || c1.toLatin1()) {
        QString c(c1.unicode() == QChar::Null ? QString() : QString(c1));
        QTest::sendKeyEvent(QTest::Press, w, static_cast<Qt::Key>(k1), c, b1);
        QTest::sendKeyEvent(QTest::Release, w, static_cast<Qt::Key>(k1), c, b1);
    }

    if (k2 || c2.toLatin1()) {
        QString c(c2.unicode() == QChar::Null ? QString() : QString(c2));
        QTest::sendKeyEvent(QTest::Press, w, static_cast<Qt::Key>(k2), c, b2);
        QTest::sendKeyEvent(QTest::Release, w, static_cast<Qt::Key>(k2), c, b2);
    }

    if (k3 || c3.toLatin1()) {
        QString c(c3.unicode() == QChar::Null ? QString() : QString(c3));
        QTest::sendKeyEvent(QTest::Press, w, static_cast<Qt::Key>(k3), c, b3);
        QTest::sendKeyEvent(QTest::Release, w, static_cast<Qt::Key>(k3), c, b3);
    }

    if (k4 || c4.toLatin1()) {
        QString c(c4.unicode() == QChar::Null ? QString() : QString(c4));
        QTest::sendKeyEvent(QTest::Press, w, static_cast<Qt::Key>(k4), c, b4);
        QTest::sendKeyEvent(QTest::Release, w, static_cast<Qt::Key>(k4), c, b4);
    }
}

void tst_QShortcut::testElement()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    static QScopedPointer<MainWindow> mainW;

    currentResult = NoResult;
    QFETCH(tst_QShortcut::Action, action);
    QFETCH(tst_QShortcut::Widget, testWidget);
    QFETCH(QString, txt);
    QFETCH(int, k1);
    QFETCH(int, c1);
    QFETCH(int, k2);
    QFETCH(int, c2);
    QFETCH(int, k3);
    QFETCH(int, c3);
    QFETCH(int, k4);
    QFETCH(int, c4);
    QFETCH(tst_QShortcut::Result, result);

    if (mainW.isNull()) {
        mainW.reset(new MainWindow);
        mainW->setWindowTitle(QTest::currentTestFunction());
        mainW->show();
        mainW->activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(mainW.data()));
    }

    switch (action) {
    case ClearAll:
        qDeleteAll(mainW->findChildren<QShortcut *>());
        break;
    case SetupAccel:
        setupShortcut(mainW.data(), txt, testWidget, txt.isEmpty()
                      ? QKeySequence(k1, k2, k3, k4) : QKeySequence::fromString(txt));
        break;
    case TestAccel:
        sendKeyEvents(mainW.data(), k1, c1, k2, c2, k3, c3, k4, c4);
        QCOMPARE(currentResult, result);
        break;
    case TestEnd:
        mainW.reset();
        break;
    }
}

void tst_QShortcut::shortcutToFocusProxy()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QLineEdit le;
    QCompleter completer;
    QStringListModel *slm = new QStringListModel(QStringList() << "a0" << "a1" << "a2", &completer);
    completer.setModel(slm);
    completer.setCompletionMode(QCompleter::PopupCompletion);
    le.setCompleter(&completer);
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_S), &le);
    QObject::connect(shortcut, &QShortcut::activated, &le, &QLineEdit::clear);
    le.setFocus();
    le.show();

    QVERIFY(QTest::qWaitForWindowActive(&le));
    QCOMPARE(QApplication::focusWidget(), &le);
    QTest::keyEvent(QTest::Press, QApplication::focusWidget(), Qt::Key_A);

    QCOMPARE(le.text(), QString::fromLocal8Bit("a"));
    QTest::keyEvent(QTest::Press, QApplication::focusWidget(), Qt::Key_Alt);
    QTest::keyEvent(QTest::Press, QApplication::focusWidget(), Qt::Key_S, Qt::AltModifier);
    QCOMPARE(le.text(), QString());
}

void tst_QShortcut::deleteLater()
{
    QWidget w;
    QPointer<QShortcut> sc(new QShortcut(QKeySequence(Qt::Key_1), &w));
    sc->deleteLater();
    QTRY_VERIFY(!sc);
}


QTEST_MAIN(tst_QShortcut)
#include "tst_qshortcut.moc"
