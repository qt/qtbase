/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include "../../shared/util.h"

#include <qinputcontext.h>
#include <qlineedit.h>
#include <qplaintextedit.h>
#include <qlayout.h>
#include <qradiobutton.h>
#include <qwindowsstyle.h>
#include <qdesktopwidget.h>
#include <qpushbutton.h>
#include <qgraphicsview.h>
#include <qgraphicsscene.h>

#ifdef QT_WEBKIT_LIB
#include <qwebview.h>
#include <qgraphicswebview.h>
#endif

#ifdef Q_OS_SYMBIAN
#include <private/qt_s60_p.h>
#include <private/qcoefepinputcontext_p.h>

#include <w32std.h>
#include <coecntrl.h>
#endif

class tst_QInputContext : public QObject
{
Q_OBJECT

public:
    tst_QInputContext() : m_phoneIsQwerty(false) {}
    virtual ~tst_QInputContext() {}

    template <class WidgetType> void symbianTestCoeFepInputContext_addData();

public slots:
    void initTestCase();
    void cleanupTestCase() {}
    void init() {}
    void cleanup() {}
private slots:
    void maximumTextLength();
    void filterMouseEvents();
    void requestSoftwareInputPanel();
    void closeSoftwareInputPanel();
    void selections();
    void focusProxy();
    void contextInheritance();
    void symbianTestCoeFepInputContext_data();
    void symbianTestCoeFepInputContext();
    void symbianTestCoeFepAutoCommit_data();
    void symbianTestCoeFepAutoCommit();

private:
    bool m_phoneIsQwerty;
};

#ifdef Q_OS_SYMBIAN
class KeyEvent : public TWsEvent
{
public:
    KeyEvent(QWidget *w, TInt type, TInt scanCode, TUint code, TUint modifiers, TInt repeats) {
        iHandle = w->effectiveWinId()->DrawableWindow()->WindowGroupId();
        iType = type;
        SetTimeNow();
        TKeyEvent *keyEvent = reinterpret_cast<TKeyEvent *>(iEventData);
        keyEvent->iScanCode = scanCode;
        keyEvent->iCode = code;
        keyEvent->iModifiers = modifiers;
        keyEvent->iRepeats = repeats;
    }
};

class FepReplayEvent
{
public:
    enum Type {
        Pause,
        Key,
        CompleteKey
    };

    FepReplayEvent(int msecsToPause)
        : m_type(Pause)
        , m_msecsToPause(msecsToPause)
    {
    }

    FepReplayEvent(TInt keyType, TInt scanCode, TUint code, TUint modifiers, TInt repeats)
        : m_type(Key)
        , m_keyType(keyType)
        , m_scanCode(scanCode)
        , m_code(code)
        , m_modifiers(modifiers)
        , m_repeats(repeats)
    {
    }

    FepReplayEvent(TInt scanCode, TUint code, TUint modifiers, TInt repeats)
        : m_type(CompleteKey)
        , m_scanCode(scanCode)
        , m_code(code)
        , m_modifiers(modifiers)
        , m_repeats(repeats)
    {
    }

    void sendEvent(QWidget *w, TInt type, TInt scanCode, TUint code, TUint modifiers, TInt repeats)
    {
        KeyEvent event(w, type, scanCode, code, modifiers, repeats);
        S60->wsSession().SendEventToWindowGroup(w->effectiveWinId()->DrawableWindow()->WindowGroupId(), event);
    }

    void pause(int msecs)
    {
        // Don't use qWait here. The polling nature of that function screws up the test.
        QTimer timer;
        QEventLoop loop;
        QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
        timer.setSingleShot(true);
        timer.start(msecs);
        loop.exec();
    }

    // For some reason, the test fails if using processEvents instead of an event loop
    // with a timer to quit it, so use the timer.
#define KEY_WAIT 10

    void replay(QWidget *w)
    {
        if (m_type == Pause) {
            pause(m_msecsToPause);
        } else if (m_type == Key) {
            sendEvent(w, m_keyType, m_scanCode, m_code, m_modifiers, m_repeats);
            if (m_keyType != EEventKeyDown)
                // EEventKeyDown events should have no pause before the EEventKey event.
                pause(KEY_WAIT);
        } else if (m_type == CompleteKey) {
            sendEvent(w, EEventKeyDown, m_scanCode, 0, m_modifiers, m_repeats);
            // EEventKeyDown events should have no pause before the EEventKey event.
            sendEvent(w, EEventKey, m_scanCode, m_code, m_modifiers, m_repeats);
            pause(KEY_WAIT);
            sendEvent(w, EEventKeyUp, m_scanCode, 0, m_modifiers, m_repeats);
            pause(KEY_WAIT);
        }
    }

private:
    Type m_type;
    int m_msecsToPause;
    TInt m_keyType;
    TInt m_scanCode;
    TUint m_code;
    TUint m_modifiers;
    TInt m_repeats;
};

Q_DECLARE_METATYPE(QList<FepReplayEvent>)
Q_DECLARE_METATYPE(Qt::InputMethodHints)
Q_DECLARE_METATYPE(QLineEdit::EchoMode);

#endif // Q_OS_SYMBIAN

void tst_QInputContext::initTestCase()
{
#ifdef Q_OS_SYMBIAN
    // Sanity test. Checks FEP for:
    // - T9 mode is default (it will attempt to fix this)
    // - Language is English (it cannot fix this; bail out if not correct)
    QWidget w;
    QLayout *layout = new QVBoxLayout;
    w.setLayout(layout);
    QLineEdit *lineedit = new QLineEdit;
    layout->addWidget(lineedit);
    lineedit->setFocus();
#ifdef QT_KEYPAD_NAVIGATION
    lineedit->setEditFocus(true);
#endif
    w.show();

    QDesktopWidget desktop;
    QRect screenSize = desktop.screenGeometry(&w);
    if (screenSize.width() > screenSize.height()) {
        // Crude way of finding out we are running on a qwerty phone.
        m_phoneIsQwerty = true;
        return;
    }

    for (int iterations = 0; iterations < 16; iterations++) {
        QTest::qWait(500);

        QList<FepReplayEvent> keyEvents;

        keyEvents << FepReplayEvent('9', '9', 0, 0);
        keyEvents << FepReplayEvent('6', '6', 0, 0);
        keyEvents << FepReplayEvent('8', '8', 0, 0);
        keyEvents << FepReplayEvent(EStdKeyRightArrow, EKeyRightArrow, 0, 0);

        foreach(FepReplayEvent event, keyEvents) {
            event.replay(lineedit);
        }

        QApplication::processEvents();

        if (lineedit->text().endsWith("you", Qt::CaseInsensitive)) {
            // Success!
            return;
        }

        // Try changing modes.
        // After 8 iterations, try to press the mode switch twice before typing.
        for (int c = 0; c <= iterations / 8; c++) {
            FepReplayEvent(EStdKeyHash, '#', 0, 0).replay(lineedit);
        }
    }

    QFAIL("FEP sanity test failed. Either the phone is not set to English, or the test was unable to enable T9");
#endif
}

void tst_QInputContext::maximumTextLength()
{
    QLineEdit le;

    le.setMaxLength(15);
    QVariant variant = le.inputMethodQuery(Qt::ImMaximumTextLength);
    QVERIFY(variant.isValid());
    QCOMPARE(variant.toInt(), 15);

    QPlainTextEdit pte;
    // For BC/historical reasons, QPlainTextEdit::inputMethodQuery is protected.
    variant = static_cast<QWidget *>(&pte)->inputMethodQuery(Qt::ImMaximumTextLength);
    QVERIFY(!variant.isValid());
}

class QFilterInputContext : public QInputContext
{
public:
    QFilterInputContext() {}
    ~QFilterInputContext() {}

    QString identifierName() { return QString(); }
    QString language() { return QString(); }

    void reset() {}

    bool isComposing() const { return false; }

    bool filterEvent( const QEvent *event )
    {
        lastTypes.append(event->type());
        return false;
    }

public:
    QList<QEvent::Type> lastTypes;
};

void tst_QInputContext::filterMouseEvents()
{
    QLineEdit le;
    le.show();
    QApplication::setActiveWindow(&le);

    QFilterInputContext *ic = new QFilterInputContext;
    le.setInputContext(ic);
    QTest::mouseClick(&le, Qt::LeftButton);

    QVERIFY(ic->lastTypes.indexOf(QEvent::MouseButtonRelease) >= 0);

    le.setInputContext(0);
}

class RequestSoftwareInputPanelStyle : public QWindowsStyle
{
public:
    RequestSoftwareInputPanelStyle()
        : m_rsipBehavior(RSIP_OnMouseClickAndAlreadyFocused)
    {
#ifdef Q_OS_WINCE
        qApp->setAutoSipEnabled(true);
#endif
    }
    ~RequestSoftwareInputPanelStyle()
    {
    }

    int styleHint(StyleHint hint, const QStyleOption *opt = 0,
                  const QWidget *widget = 0, QStyleHintReturn* returnData = 0) const
    {
        if (hint == SH_RequestSoftwareInputPanel) {
            return m_rsipBehavior;
        } else {
            return QWindowsStyle::styleHint(hint, opt, widget, returnData);
        }
    }

    RequestSoftwareInputPanel m_rsipBehavior;
};

void tst_QInputContext::requestSoftwareInputPanel()
{
    QStyle *oldStyle = qApp->style();
    oldStyle->setParent(this); // Prevent it being deleted.
    RequestSoftwareInputPanelStyle *newStyle = new RequestSoftwareInputPanelStyle;
    qApp->setStyle(newStyle);

    QWidget w;
    QLayout *layout = new QVBoxLayout;
    QLineEdit *le1, *le2;
    le1 = new QLineEdit;
    le2 = new QLineEdit;
    layout->addWidget(le1);
    layout->addWidget(le2);
    w.setLayout(layout);

    QFilterInputContext *ic1, *ic2;
    ic1 = new QFilterInputContext;
    ic2 = new QFilterInputContext;
    le1->setInputContext(ic1);
    le2->setInputContext(ic2);

    w.show();
    QApplication::setActiveWindow(&w);

    // Testing single click panel activation.
    newStyle->m_rsipBehavior = QStyle::RSIP_OnMouseClick;
    QTest::mouseClick(le2, Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));
    QVERIFY(ic2->lastTypes.indexOf(QEvent::RequestSoftwareInputPanel) >= 0);
    ic2->lastTypes.clear();

    // Testing double click panel activation.
    newStyle->m_rsipBehavior = QStyle::RSIP_OnMouseClickAndAlreadyFocused;
    QTest::mouseClick(le1, Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));
    QVERIFY(ic1->lastTypes.indexOf(QEvent::RequestSoftwareInputPanel) < 0);
    QTest::mouseClick(le1, Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));
    QVERIFY(ic1->lastTypes.indexOf(QEvent::RequestSoftwareInputPanel) >= 0);
    ic1->lastTypes.clear();

    // Testing right mouse button
    QTest::mouseClick(le1, Qt::RightButton, Qt::NoModifier, QPoint(5, 5));
    QVERIFY(ic1->lastTypes.indexOf(QEvent::RequestSoftwareInputPanel) < 0);

    qApp->setStyle(oldStyle);
    oldStyle->setParent(qApp);
}

void tst_QInputContext::closeSoftwareInputPanel()
{
    QWidget w;
    QLayout *layout = new QVBoxLayout;
    QLineEdit *le1, *le2;
    QRadioButton *rb;
    le1 = new QLineEdit;
    le2 = new QLineEdit;
    rb = new QRadioButton;
    layout->addWidget(le1);
    layout->addWidget(le2);
    layout->addWidget(rb);
    w.setLayout(layout);

    QFilterInputContext *ic1, *ic2;
    ic1 = new QFilterInputContext;
    ic2 = new QFilterInputContext;
    le1->setInputContext(ic1);
    le2->setInputContext(ic2);

    w.show();
    QApplication::setActiveWindow(&w);

    // Testing that panel doesn't close between two input methods aware widgets.
    QTest::mouseClick(le1, Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));
    QTest::mouseClick(le2, Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));
    QVERIFY(ic2->lastTypes.indexOf(QEvent::CloseSoftwareInputPanel) < 0);

    // Testing that panel closes when focusing non-aware widget.
    QTest::mouseClick(rb, Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));
    QVERIFY(ic2->lastTypes.indexOf(QEvent::CloseSoftwareInputPanel) >= 0);
}

void tst_QInputContext::selections()
{
    QLineEdit le;
    le.setText("Test text");
    le.setSelection(2, 2);
    QCOMPARE(le.inputMethodQuery(Qt::ImCursorPosition).toInt(), 4);
    QCOMPARE(le.inputMethodQuery(Qt::ImAnchorPosition).toInt(), 2);

    QList<QInputMethodEvent::Attribute> attributes;
    attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::Selection, 5, 3, QVariant()));
    QInputMethodEvent event("", attributes);
    QApplication::sendEvent(&le, &event);
    QCOMPARE(le.cursorPosition(), 8);
    QCOMPARE(le.selectionStart(), 5);
    QCOMPARE(le.inputMethodQuery(Qt::ImCursorPosition).toInt(), 8);
    QCOMPARE(le.inputMethodQuery(Qt::ImAnchorPosition).toInt(), 5);
}

void tst_QInputContext::focusProxy()
{
    QWidget toplevel(0, Qt::X11BypassWindowManagerHint); toplevel.setObjectName("toplevel");
    QWidget w(&toplevel); w.setObjectName("w");
    QWidget proxy(&w); proxy.setObjectName("proxy");
    QWidget proxy2(&w); proxy2.setObjectName("proxy2");
    w.setFocusProxy(&proxy);
    w.setAttribute(Qt::WA_InputMethodEnabled);
    toplevel.show();
    QApplication::setActiveWindow(&toplevel);
    QTest::qWaitForWindowShown(&toplevel);
    w.setFocus();
    w.setAttribute(Qt::WA_NativeWindow); // we shouldn't crash!

    proxy.setAttribute(Qt::WA_InputMethodEnabled);
    proxy2.setAttribute(Qt::WA_InputMethodEnabled);

    proxy2.setFocus();
    w.setFocus();

    QInputContext *gic = qApp->inputContext();
    QVERIFY(gic);
    QCOMPARE(gic->focusWidget(), &proxy);

    // then change the focus proxy and check that input context is valid
    QVERIFY(w.hasFocus());
    QVERIFY(proxy.hasFocus());
    QVERIFY(!proxy2.hasFocus());
    w.setFocusProxy(&proxy2);
    QVERIFY(!w.hasFocus());
    QVERIFY(proxy.hasFocus());
    QVERIFY(!proxy2.hasFocus());
    QCOMPARE(gic->focusWidget(), &proxy);
}

void tst_QInputContext::contextInheritance()
{
    QWidget parent;
    QWidget child(&parent);

    parent.setAttribute(Qt::WA_InputMethodEnabled, true);
    child.setAttribute(Qt::WA_InputMethodEnabled, true);

    QCOMPARE(parent.inputContext(), qApp->inputContext());
    QCOMPARE(child.inputContext(), qApp->inputContext());

    QInputContext *qic = new QFilterInputContext;
    parent.setInputContext(qic);
    QCOMPARE(parent.inputContext(), qic);
    QCOMPARE(child.inputContext(), qic);

    parent.setAttribute(Qt::WA_InputMethodEnabled, false);
    QVERIFY(!parent.inputContext());
    QCOMPARE(child.inputContext(), qic);
    parent.setAttribute(Qt::WA_InputMethodEnabled, true);

    parent.setInputContext(0);
    QCOMPARE(parent.inputContext(), qApp->inputContext());
    QCOMPARE(child.inputContext(), qApp->inputContext());

    qic = new QFilterInputContext;
    qApp->setInputContext(qic);
    QCOMPARE(parent.inputContext(), qic);
    QCOMPARE(child.inputContext(), qic);
}

#ifdef QT_WEBKIT_LIB
class AutoWebView : public QWebView
{
    Q_OBJECT

public:
    AutoWebView()
        : m_length(0)
        , m_mode(QLineEdit::Normal)
    {
        updatePage();
    }
    ~AutoWebView() {}

    void updatePage()
    {
        // The update might reset the input method parameters.
        bool imEnabled = testAttribute(Qt::WA_InputMethodEnabled);
        Qt::InputMethodHints hints = inputMethodHints();

        QString page = "<html><body onLoad=\"document.forms.testform.testinput.focus()\">"
                "<form name=\"testform\"><input name=\"testinput\" type=\"%1\" %2></form></body></html>";
        if (m_mode == QLineEdit::Password)
            page = page.arg("password");
        else
            page = page.arg("text");

        if (m_length == 0)
            page = page.arg("");
        else
            page = page.arg("maxlength=\"" + QString::number(m_length) + "\"");

        setHtml(page);

        setAttribute(Qt::WA_InputMethodEnabled, imEnabled);
        setInputMethodHints(hints);
    }
    void setMaxLength(int length)
    {
        m_length = length;
        updatePage();
    }
    void setEchoMode(QLineEdit::EchoMode mode)
    {
        m_mode = mode;
        updatePage();
    }

    int m_length;
    QLineEdit::EchoMode m_mode;
};

class AutoGraphicsWebView : public QGraphicsView
{
    Q_OBJECT

public:
    AutoGraphicsWebView()
        : m_length(0)
        , m_mode(QLineEdit::Normal)
    {
        m_scene.addItem(&m_view);
        setScene(&m_scene);
        m_view.setFocus();
        updatePage();
    }
    ~AutoGraphicsWebView() {}

    void updatePage()
    {
        // The update might reset the input method parameters.
        bool imEnabled = testAttribute(Qt::WA_InputMethodEnabled);
        Qt::InputMethodHints hints = inputMethodHints();

        QString page = "<html><body onLoad=\"document.forms.testform.testinput.focus()\">"
                "<form name=\"testform\"><input name=\"testinput\" type=\"%1\" %2></form></body></html>";
        if (m_mode == QLineEdit::Password)
            page = page.arg("password");
        else
            page = page.arg("text");

        if (m_length == 0)
            page = page.arg("");
        else
            page = page.arg("maxlength=\"" + QString::number(m_length) + "\"");

        m_view.setHtml(page);

        setAttribute(Qt::WA_InputMethodEnabled, imEnabled);
        setInputMethodHints(hints);
    }
    void setMaxLength(int length)
    {
        m_length = length;
        updatePage();
    }
    void setEchoMode(QLineEdit::EchoMode mode)
    {
        m_mode = mode;
        updatePage();
    }

    int m_length;
    QLineEdit::EchoMode m_mode;
    QGraphicsScene m_scene;
    QGraphicsWebView m_view;
};
#endif // QT_WEBKIT_LIB

void tst_QInputContext::symbianTestCoeFepInputContext_data()
{
#ifdef Q_OS_SYMBIAN
    QTest::addColumn<QWidget *>              ("editwidget");
    QTest::addColumn<bool>                   ("inputMethodEnabled");
    QTest::addColumn<Qt::InputMethodHints>   ("inputMethodHints");
    QTest::addColumn<int>                    ("maxLength"); // Zero for no limit
    QTest::addColumn<QLineEdit::EchoMode>    ("echoMode");
    QTest::addColumn<QList<FepReplayEvent> > ("keyEvents");
    QTest::addColumn<QString>                ("finalString");
    QTest::addColumn<QString>                ("preeditString");

    symbianTestCoeFepInputContext_addData<QLineEdit>();
    symbianTestCoeFepInputContext_addData<QPlainTextEdit>();
    symbianTestCoeFepInputContext_addData<QTextEdit>();
# ifdef QT_WEBKIT_LIB
    symbianTestCoeFepInputContext_addData<AutoWebView>();
    symbianTestCoeFepInputContext_addData<AutoGraphicsWebView>();
# endif
#endif
}

Q_DECLARE_METATYPE(QWidget *)
Q_DECLARE_METATYPE(QLineEdit *)
Q_DECLARE_METATYPE(QPlainTextEdit *)
Q_DECLARE_METATYPE(QTextEdit *)

template <class WidgetType>
void tst_QInputContext::symbianTestCoeFepInputContext_addData()
{
#ifdef Q_OS_SYMBIAN
    QList<FepReplayEvent> events;
    QWidget *editwidget;

    events << FepReplayEvent(EStdKeyBackspace, EKeyBackspace, 0, 0);
    events << FepReplayEvent(EStdKeyBackspace, EKeyBackspace, 0, 0);
    events << FepReplayEvent('5', '5', 0, 0);
    events << FepReplayEvent('4', '4', 0, 0);
    events << FepReplayEvent('6', '6', 0, 0);
    events << FepReplayEvent(EStdKeyBackspace, EKeyBackspace, 0, 0);
    events << FepReplayEvent(EStdKeyBackspace, EKeyBackspace, 0, 0);
    events << FepReplayEvent('1', '1', 0, 0);
    events << FepReplayEvent(EStdKeyBackspace, EKeyBackspace, 0, 0);
    events << FepReplayEvent('2', '2', 0, 0);
    events << FepReplayEvent('1', '1', 0, 0);
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Numbers (no FEP)").toLocal8Bit())
            << editwidget
            << false
            << Qt::InputMethodHints(Qt::ImhNone)
            << 0
            << QLineEdit::Normal
            << events
            << QString("521")
            << QString("");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Numbers and password mode (no FEP)").toLocal8Bit())
            << editwidget
            << false
            << Qt::InputMethodHints(Qt::ImhNone)
            << 0
            << QLineEdit::Password
            << events
            << QString("521")
            << QString("");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Numbers").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhDigitsOnly)
            << 0
            << QLineEdit::Normal
            << events
            << QString("521")
            << QString("");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Numbers max length (no FEP)").toLocal8Bit())
            << editwidget
            << false
            << Qt::InputMethodHints(Qt::ImhNone)
            << 2
            << QLineEdit::Normal
            << events
            << QString("21")
            << QString("");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Numbers max length").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhDigitsOnly)
            << 2
            << QLineEdit::Normal
            << events
            << QString("21")
            << QString("");
    events.clear();

    events << FepReplayEvent(EEventKeyDown, '5', 0, 0, 0);
    events << FepReplayEvent(EEventKey, '5', '5', 0, 0);
    events << FepReplayEvent(EEventKey, '5', '5', 0, 1);
    events << FepReplayEvent(EEventKey, '5', '5', 0, 1);
    events << FepReplayEvent(EEventKeyUp, '5', 0, 0, 0);
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Numbers and autorepeat (no FEP)").toLocal8Bit())
            << editwidget
            << false
            << Qt::InputMethodHints(Qt::ImhNone)
            << 0
            << QLineEdit::Normal
            << events
            << QString("555")
            << QString("");
    events.clear();

    events << FepReplayEvent(EStdKeyBackspace, EKeyBackspace, 0, 0);
    events << FepReplayEvent('2', '2', 0, 0);
    events << FepReplayEvent('3', '3', 0, 0);
    events << FepReplayEvent('4', '4', 0, 0);
    events << FepReplayEvent('4', '4', 0, 0);
    events << FepReplayEvent('5', '5', 0, 0);
    events << FepReplayEvent('5', '5', 0, 0);
    events << FepReplayEvent(EStdKeyBackspace, EKeyBackspace, 0, 0);
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Multitap").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhNoPredictiveText)
            << 0
            << QLineEdit::Normal
            << events
            << QString("Adh")
            << QString("");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Multitap with no auto uppercase").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhNoPredictiveText | Qt::ImhNoAutoUppercase)
            << 0
            << QLineEdit::Normal
            << events
            << QString("adh")
            << QString("");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Multitap with uppercase").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhNoPredictiveText | Qt::ImhPreferUppercase)
            << 0
            << QLineEdit::Normal
            << events
            << QString("ADH")
            << QString("");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Multitap with lowercase").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhNoPredictiveText | Qt::ImhPreferLowercase)
            << 0
            << QLineEdit::Normal
            << events
            << QString("adh")
            << QString("");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Multitap with forced uppercase").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhNoPredictiveText | Qt::ImhUppercaseOnly)
            << 0
            << QLineEdit::Normal
            << events
            << QString("ADH")
            << QString("");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Multitap with forced lowercase").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhNoPredictiveText | Qt::ImhLowercaseOnly)
            << 0
            << QLineEdit::Normal
            << events
            << QString("adh")
            << QString("");
    events.clear();

    events << FepReplayEvent(EStdKeyHash, '#', 0, 0);
    events << FepReplayEvent('2', '2', 0, 0);
    events << FepReplayEvent('2', '2', 0, 0);
    events << FepReplayEvent('3', '3', 0, 0);
    events << FepReplayEvent('4', '4', 0, 0);
    events << FepReplayEvent('4', '4', 0, 0);
    events << FepReplayEvent('5', '5', 0, 0);
    events << FepReplayEvent('5', '5', 0, 0);
    events << FepReplayEvent(EStdKeyBackspace, EKeyBackspace, 0, 0);
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Multitap with mode switch").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhNoPredictiveText)
            << 0
            << QLineEdit::Normal
            << events
            << QString("bdh")
            << QString("");
    events.clear();

    events << FepReplayEvent('7', '7', 0, 0);
    events << FepReplayEvent('7', '7', 0, 0);
    events << FepReplayEvent('8', '8', 0, 0);
    events << FepReplayEvent('9', '9', 0, 0);
    events << FepReplayEvent('9', '9', 0, 0);
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Multitap with unfinished text").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhNoPredictiveText)
            << 0
            << QLineEdit::Normal
            << events
            << QString("Qt")
            << QString("x");
    events << FepReplayEvent(2000);
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Multitap with committed text").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhNoPredictiveText)
            << 0
            << QLineEdit::Normal
            << events
            << QString("Qtx")
            << QString("");
    events.clear();

    events << FepReplayEvent('4', '4', 0, 0);
    events << FepReplayEvent('4', '4', 0, 0);
    // Simulate holding down hash key.
    events << FepReplayEvent(EEventKeyDown, EStdKeyHash, 0, 0, 0);
    events << FepReplayEvent(EEventKey, EStdKeyHash, '#', 0, 0);
    events << FepReplayEvent(500);
    events << FepReplayEvent(EEventKey, EStdKeyHash, '#', 0, 1);
    events << FepReplayEvent(EEventKey, EStdKeyHash, '#', 0, 1);
    events << FepReplayEvent(EEventKey, EStdKeyHash, '#', 0, 1);
    events << FepReplayEvent(EEventKeyUp, EStdKeyHash, 0, 0, 0);
    events << FepReplayEvent('7', '7', 0, 0);
    events << FepReplayEvent('7', '7', 0, 0);
    events << FepReplayEvent('8', '8', 0, 0);
    // QTBUG-9867: Switch back as well to make sure we don't get extra symbols
    events << FepReplayEvent(EEventKeyDown, EStdKeyHash, 0, 0, 0);
    events << FepReplayEvent(EEventKey, EStdKeyHash, '#', 0, 0);
    events << FepReplayEvent(500);
    events << FepReplayEvent(EEventKey, EStdKeyHash, '#', 0, 1);
    events << FepReplayEvent(EEventKey, EStdKeyHash, '#', 0, 1);
    events << FepReplayEvent(EEventKey, EStdKeyHash, '#', 0, 1);
    events << FepReplayEvent(EEventKeyUp, EStdKeyHash, 0, 0, 0);
    events << FepReplayEvent('9', '9', 0, 0);
    events << FepReplayEvent('6', '6', 0, 0);
    events << FepReplayEvent('8', '8', 0, 0);
    events << FepReplayEvent(2000);
    events << FepReplayEvent(EStdKeyDevice3, EKeyDevice3, 0, 0); // Select key
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Multitap and numbers").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhNoPredictiveText)
            << 0
            << QLineEdit::Normal
            << events
            << QString("H778wmt")
            << QString("");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": T9 and numbers").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhPreferLowercase)
            << 0
            << QLineEdit::Normal
            << events
            << QString("hi778you")
            << QString("");
    events.clear();

    events << FepReplayEvent('4', '4', 0, 0);
    events << FepReplayEvent('4', '4', 0, 0);
    events << FepReplayEvent(EStdKeyDevice3, EKeyDevice3, 0, 0); // Select key
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": T9").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhPreferLowercase)
            << 0
            << QLineEdit::Normal
            << events
            << QString("hi")
            << QString("");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": T9 with uppercase").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhPreferUppercase)
            << 0
            << QLineEdit::Normal
            << events
            << QString("HI")
            << QString("");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": T9 with forced lowercase").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhLowercaseOnly)
            << 0
            << QLineEdit::Normal
            << events
            << QString("hi")
            << QString("");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": T9 with forced uppercase").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhUppercaseOnly)
            << 0
            << QLineEdit::Normal
            << events
            << QString("HI")
            << QString("");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": T9 with maxlength").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhLowercaseOnly)
            << 1
            << QLineEdit::Normal
            << events
            << QString("i")
            << QString("");
    events.clear();

    events << FepReplayEvent('4', '4', 0, 0);
    events << FepReplayEvent('4', '4', 0, 0);
    events << FepReplayEvent(EStdKeyLeftArrow, EKeyLeftArrow, 0, 0);
    events << FepReplayEvent(EStdKeyLeftArrow, EKeyLeftArrow, 0, 0);
    events << FepReplayEvent('9', '9', 0, 0);
    events << FepReplayEvent('6', '6', 0, 0);
    events << FepReplayEvent('8', '8', 0, 0);
    events << FepReplayEvent('0', '0', 0, 0);
    events << FepReplayEvent(EStdKeyRightArrow, EKeyRightArrow, 0, 0);
    events << FepReplayEvent(EStdKeyRightArrow, EKeyRightArrow, 0, 0);
    events << FepReplayEvent('8', '8', 0, 0);
    events << FepReplayEvent('8', '8', 0, 0);
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": T9 with movement and unfinished text").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhPreferLowercase)
            << 0
            << QLineEdit::Normal
            << events
            << QString("you hi")
            << QString("tv");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": T9 with movement, password and unfinished text").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhPreferLowercase)
            << 0
            << QLineEdit::Password
            << events
            << QString("wmt h")
            << QString("u");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": T9 with movement, maxlength, password and unfinished text").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhPreferLowercase)
            << 2
            << QLineEdit::Password
            << events
            << QString("wh")
            << QString("");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": T9 with movement, maxlength and unfinished text").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhPreferLowercase)
            << 2
            << QLineEdit::Normal
            << events
            << QString("hi")
            << QString("");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Multitap with movement and unfinished text").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhNoPredictiveText | Qt::ImhPreferLowercase)
            << 0
            << QLineEdit::Normal
            << events
            << QString("wmt h")
            << QString("u");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Multitap with movement, maxlength and unfinished text").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhNoPredictiveText | Qt::ImhPreferLowercase)
            << 2
            << QLineEdit::Normal
            << events
            << QString("wh")
            << QString("");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Numbers with movement").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhDigitsOnly)
            << 0
            << QLineEdit::Normal
            << events
            << QString("96804488")
            << QString("");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Numbers with movement and maxlength").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhDigitsOnly)
            << 2
            << QLineEdit::Normal
            << events
            << QString("44")
            << QString("");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Numbers with movement, password and unfinished text").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhDigitsOnly)
            << 0
            << QLineEdit::Password
            << events
            << QString("9680448")
            << QString("8");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Numbers with movement, maxlength, password and unfinished text").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhDigitsOnly)
            << 2
            << QLineEdit::Password
            << events
            << QString("44")
            << QString("");
    events << FepReplayEvent(EStdKeyRightArrow, EKeyRightArrow, 0, 0);
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": T9 with movement").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhPreferLowercase)
            << 0
            << QLineEdit::Normal
            << events
            << QString("you htvi")
            << QString("");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": T9 with movement and password").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhPreferLowercase)
            << 0
            << QLineEdit::Password
            << events
            << QString("wmt hu")
            << QString("");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": T9 with movement, maxlength and password").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhPreferLowercase)
            << 2
            << QLineEdit::Password
            << events
            << QString("wh")
            << QString("");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Multitap with movement").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhNoPredictiveText | Qt::ImhPreferLowercase)
            << 0
            << QLineEdit::Normal
            << events
            << QString("wmt hu")
            << QString("");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Multitap with movement and maxlength").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhNoPredictiveText | Qt::ImhPreferLowercase)
            << 2
            << QLineEdit::Normal
            << events
            << QString("wh")
            << QString("");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Numbers with movement and password").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhDigitsOnly)
            << 0
            << QLineEdit::Password
            << events
            << QString("96804488")
            << QString("");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Numbers with movement, maxlength and password").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhDigitsOnly)
            << 2
            << QLineEdit::Password
            << events
            << QString("44")
            << QString("");
    events.clear();

    // Test that the symbol key successfully does nothing when in number-only mode.
    events << FepReplayEvent(EEventKeyDown, EStdKeyLeftFunc, 0, 0, 0);
    events << FepReplayEvent(EEventKeyUp, EStdKeyLeftFunc, 0, 0, 0);
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Dead symbols key").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhDigitsOnly)
            << 0
            << QLineEdit::Normal
            << events
            << QString("")
            << QString("");
    editwidget = new WidgetType;
    QTest::newRow(QString(QString::fromLatin1(editwidget->metaObject()->className())
            + ": Dead symbols key and password").toLocal8Bit())
            << editwidget
            << true
            << Qt::InputMethodHints(Qt::ImhDigitsOnly)
            << 0
            << QLineEdit::Password
            << events
            << QString("")
            << QString("");
    events.clear();
#endif
}

void tst_QInputContext::symbianTestCoeFepInputContext()
{
#ifndef Q_OS_SYMBIAN
    QSKIP("This is a Symbian-only test", SkipAll);
#else
    QCoeFepInputContext *ic = qobject_cast<QCoeFepInputContext *>(qApp->inputContext());
    if (!ic) {
        QSKIP("coefep is not the active input context; skipping test", SkipAll);
    }

    QFETCH(QWidget *,             editwidget);
    QFETCH(bool,                  inputMethodEnabled);
    QFETCH(Qt::InputMethodHints,  inputMethodHints);
    QFETCH(int,                   maxLength);
    QFETCH(QLineEdit::EchoMode,   echoMode);
    QFETCH(QList<FepReplayEvent>, keyEvents);
    QFETCH(QString,               finalString);
    QFETCH(QString,               preeditString);

    if (inputMethodEnabled && m_phoneIsQwerty) {
        QSKIP("Skipping advanced input method tests on QWERTY phones", SkipSingle);
    }

    QWidget w;
    QLayout *layout = new QVBoxLayout;
    w.setLayout(layout);

    layout->addWidget(editwidget);
    editwidget->setFocus();
#ifdef QT_KEYPAD_NAVIGATION
    editwidget->setEditFocus(true);
#endif
    w.show();

    editwidget->setAttribute(Qt::WA_InputMethodEnabled, inputMethodEnabled);
    editwidget->setInputMethodHints(inputMethodHints);
    if (QLineEdit *lineedit = qobject_cast<QLineEdit *>(editwidget)) {
        if (maxLength > 0)
            lineedit->setMaxLength(maxLength);
        lineedit->setEchoMode(echoMode);
#ifdef QT_WEBKIT_LIB
    } else if (AutoWebView *webView = qobject_cast<AutoWebView *>(editwidget)) {
        if (maxLength > 0)
            webView->setMaxLength(maxLength);
        webView->setEchoMode(echoMode);
        // WebKit disables T9 everywhere.
        if (inputMethodEnabled && !(inputMethodHints & Qt::ImhNoPredictiveText))
            return;
    } else if (AutoGraphicsWebView *webView = qobject_cast<AutoGraphicsWebView *>(editwidget)) {
        if (maxLength > 0)
            webView->setMaxLength(maxLength);
        webView->setEchoMode(echoMode);
        // WebKit disables T9 everywhere.
        if (inputMethodEnabled && !(inputMethodHints & Qt::ImhNoPredictiveText))
            return;
#endif
    } else if (maxLength > 0 || echoMode != QLineEdit::Normal) {
        // Only some widgets support these features so don't attempt any tests using those
        // on other widgets.
        return;
    }

    QTest::qWait(200);

    foreach(FepReplayEvent event, keyEvents) {
        event.replay(editwidget);
    }

    QApplication::processEvents();

    QCOMPARE(editwidget->inputMethodQuery(Qt::ImSurroundingText).toString(), finalString);
    QCOMPARE(ic->m_preeditString, preeditString);

    delete editwidget;
#endif
}

void tst_QInputContext::symbianTestCoeFepAutoCommit_data()
{
#ifdef Q_OS_SYMBIAN
    QTest::addColumn<Qt::InputMethodHints>   ("inputMethodHints");
    QTest::addColumn<QLineEdit::EchoMode>    ("echoMode");
    QTest::addColumn<QList<FepReplayEvent> > ("keyEvents");
    QTest::addColumn<QString>                ("finalString");

    QList<FepReplayEvent> events;

    events << FepReplayEvent('4', '4', 0, 0);
    events << FepReplayEvent('4', '4', 0, 0);
    events << FepReplayEvent('0', '0', 0, 0);
    events << FepReplayEvent('9', '9', 0, 0);
    events << FepReplayEvent('6', '6', 0, 0);
    events << FepReplayEvent('8', '8', 0, 0);
    QTest::newRow("Numbers")
            << Qt::InputMethodHints(Qt::ImhDigitsOnly)
            << QLineEdit::Normal
            << events
            << QString("440968");
    QTest::newRow("Numbers and password")
            << Qt::InputMethodHints(Qt::ImhDigitsOnly)
            << QLineEdit::Password
            << events
            << QString("440968");
    QTest::newRow("Multitap")
            << Qt::InputMethodHints(Qt::ImhPreferLowercase | Qt::ImhNoPredictiveText)
            << QLineEdit::Normal
            << events
            << QString("h wmt");
    QTest::newRow("T9")
            << Qt::InputMethodHints(Qt::ImhPreferLowercase)
            << QLineEdit::Normal
            << events
            << QString("hi you");
    QTest::newRow("Multitap with password")
            << Qt::InputMethodHints(Qt::ImhPreferLowercase | Qt::ImhNoPredictiveText)
            << QLineEdit::Password
            << events
            << QString("h wmt");
    QTest::newRow("T9 with password")
            << Qt::InputMethodHints(Qt::ImhPreferLowercase)
            << QLineEdit::Password
            << events
            << QString("h wmt");
#endif
}

void tst_QInputContext::symbianTestCoeFepAutoCommit()
{
#ifndef Q_OS_SYMBIAN
    QSKIP("This is a Symbian-only test", SkipAll);
#else
    QCoeFepInputContext *ic = qobject_cast<QCoeFepInputContext *>(qApp->inputContext());
    if (!ic) {
        QSKIP("coefep is not the active input context; skipping test", SkipAll);
    }

    QFETCH(Qt::InputMethodHints,  inputMethodHints);
    QFETCH(QLineEdit::EchoMode,   echoMode);
    QFETCH(QList<FepReplayEvent>, keyEvents);
    QFETCH(QString,               finalString);

    if (m_phoneIsQwerty) {
        QSKIP("Skipping advanced input method tests on QWERTY phones", SkipSingle);
    }

    QWidget w;
    QLayout *layout = new QVBoxLayout;
    w.setLayout(layout);
    QLineEdit *lineedit = new QLineEdit;
    layout->addWidget(lineedit);
    lineedit->setFocus();
#ifdef QT_KEYPAD_NAVIGATION
    lineedit->setEditFocus(true);
#endif
    QPushButton *pushButton = new QPushButton("Done");
    layout->addWidget(pushButton);
    QAction softkey("Done", &w);
    softkey.setSoftKeyRole(QAction::PositiveSoftKey);
    w.addAction(&softkey);
    w.show();

    lineedit->setInputMethodHints(inputMethodHints);
    lineedit->setEchoMode(echoMode);

    QTest::qWait(200);
    foreach(FepReplayEvent event, keyEvents) {
        event.replay(lineedit);
    }
    QApplication::processEvents();

    QTest::mouseClick(pushButton, Qt::LeftButton);

    QCOMPARE(lineedit->text(), finalString);
    QVERIFY(ic->m_preeditString.isEmpty());

#ifdef Q_WS_S60
    lineedit->inputContext()->reset();
    lineedit->clear();
    lineedit->setFocus();
#ifdef QT_KEYPAD_NAVIGATION
    lineedit->setEditFocus(true);
#endif

    QTest::qWait(200);
    foreach(FepReplayEvent event, keyEvents) {
        event.replay(lineedit);
    }
    QApplication::processEvents();

    FepReplayEvent(EStdKeyDevice0, EKeyDevice0, 0, 0).replay(lineedit); // Left softkey

    QCOMPARE(lineedit->text(), finalString);
    QVERIFY(ic->m_preeditString.isEmpty());

#endif // Q_WS_S60
#endif // Q_OS_SYMBIAN
}

QTEST_MAIN(tst_QInputContext)
#include "tst_qinputcontext.moc"
