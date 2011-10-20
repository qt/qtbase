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

class tst_QInputContext : public QObject
{
Q_OBJECT

public:
    tst_QInputContext() : m_phoneIsQwerty(false) {}
    virtual ~tst_QInputContext() {}

public slots:
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

private:
    bool m_phoneIsQwerty;
};

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
    qApp->setInputContext(ic);
    QTest::mouseClick(&le, Qt::LeftButton);

    QVERIFY(ic->lastTypes.indexOf(QEvent::MouseButtonRelease) >= 0);
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

    QFilterInputContext *ic = new QFilterInputContext;
    qApp->setInputContext(ic);

    w.show();
    QApplication::setActiveWindow(&w);

    // Testing single click panel activation.
    newStyle->m_rsipBehavior = QStyle::RSIP_OnMouseClick;
    QTest::mouseClick(le2, Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));
    QVERIFY(ic->lastTypes.indexOf(QEvent::RequestSoftwareInputPanel) >= 0);
    ic->lastTypes.clear();

    // Testing double click panel activation.
    newStyle->m_rsipBehavior = QStyle::RSIP_OnMouseClickAndAlreadyFocused;
    QTest::mouseClick(le1, Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));
    QVERIFY(ic->lastTypes.indexOf(QEvent::RequestSoftwareInputPanel) < 0);
    QTest::mouseClick(le1, Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));
    QVERIFY(ic->lastTypes.indexOf(QEvent::RequestSoftwareInputPanel) >= 0);
    ic->lastTypes.clear();

    // Testing right mouse button
    QTest::mouseClick(le1, Qt::RightButton, Qt::NoModifier, QPoint(5, 5));
    QVERIFY(ic->lastTypes.indexOf(QEvent::RequestSoftwareInputPanel) < 0);

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

    QFilterInputContext *ic = new QFilterInputContext;
    qApp->setInputContext(ic);

    w.show();
    QApplication::setActiveWindow(&w);

    // Testing that panel doesn't close between two input methods aware widgets.
    QTest::mouseClick(le1, Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));
    QTest::mouseClick(le2, Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));
    QVERIFY(ic->lastTypes.indexOf(QEvent::CloseSoftwareInputPanel) < 0);

    // Testing that panel closes when focusing non-aware widget.
    QTest::mouseClick(rb, Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));
    QVERIFY(ic->lastTypes.indexOf(QEvent::CloseSoftwareInputPanel) >= 0);
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

QTEST_MAIN(tst_QInputContext)
#include "tst_qinputcontext.moc"
