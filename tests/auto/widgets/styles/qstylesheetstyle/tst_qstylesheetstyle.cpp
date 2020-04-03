/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDateEdit>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleFactory>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QToolTip>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QVBoxLayout>

#include <QtGui/QPainter>
#include <QtGui/QScreen>

#include <QtTest/QtTest>

#include <QtCore/QDebug>
#include <QtCore/QMetaObject>
#include <QtCore/QScopedPointer>

#include <private/qstylesheetstyle_p.h>
#include <private/qhighdpiscaling_p.h>
#include <QtTest/private/qtesthelpers_p.h>

using namespace QTestPrivate;

class tst_QStyleSheetStyle : public QObject
{
    Q_OBJECT
public:
    tst_QStyleSheetStyle();

    static void initMain();

private slots:
    void init();
    void cleanup();
    void repolish();
    void repolish_without_crashing();
    void numinstances();
    void widgetsBeforeAppStyleSheet();
    void widgetsAfterAppStyleSheet();
    void applicationStyleSheet();
    void windowStyleSheet();
    void widgetStyleSheet();
    void reparentWithNoChildStyleSheet();
    void reparentWithChildStyleSheet();
    void dynamicProperty();
    // NB! Invoking this slot after layoutSpacing crashes on Mac.
    void namespaces();
#ifdef Q_OS_MAC
    void layoutSpacing();
#endif
    void qproperty();
    void palettePropagation_data();
    void palettePropagation();
    void fontPropagation_data();
    void fontPropagation();
    void widgetStylePropagation_data();
    void widgetStylePropagation();
    void onWidgetDestroyed();
    void fontPrecedence();
    void focusColors();
#ifndef QT_NO_CURSOR
    void hoverColors();
#endif
    void background();
    void tabAlignment();
    void attributesList();
    void minmaxSizes();
    void task206238_twice();
    void transparent();
    void proxyStyle();
    void dialogButtonBox();
    void emptyStyleSheet();
    void toolTip();
    void embeddedFonts();
    void opaquePaintEvent_data();
    void opaquePaintEvent();
    void complexWidgetFocus();
    void task188195_baseBackground();
    void task232085_spinBoxLineEditBg();
    void changeStyleInChangeEvent();
    void QTBUG15910_crashNullWidget();
    void QTBUG36933_brokenPseudoClassLookup();
    void styleSheetChangeBeforePolish();
    //at the end because it mess with the style.
    void widgetStyle();
    void appStyle();
    void QTBUG11658_cachecrash();
    void styleSheetTargetAttribute();
    void unpolish();

    void highdpiImages_data();
    void highdpiImages();

private:
    static QColor COLOR(const QWidget &w)
    {
        w.ensurePolished();
        return w.palette().color(w.foregroundRole());
    }

    static QColor APPCOLOR(const QWidget &w)
    {
        w.ensurePolished();
        return QApplication::palette(&w).color(w.foregroundRole());
    }

    static QColor BACKGROUND(const QWidget &w)
    {
        w.ensurePolished();
        return w.palette().color(w.backgroundRole());
    }

    static QColor APPBACKGROUND(const QWidget &w)
    {
        w.ensurePolished();
        return QApplication::palette(&w).color(w.backgroundRole());
    }

    static int FONTSIZE(const QWidget &w)
    {
        w.ensurePolished();
        return w.font().pointSize();
    }

    static int APPFONTSIZE(const QWidget &w) { return QApplication::font(&w).pointSize(); }

    const QRect m_availableGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    QSize m_testSize;
};

// highdpiImages() tests HighDPI scaling; disable initially.
void tst_QStyleSheetStyle::initMain()
{
    QCoreApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
}

tst_QStyleSheetStyle::tst_QStyleSheetStyle()
{
    const int testSize = qMax(200, m_availableGeometry.width() / 10);
    m_testSize.setWidth(testSize);
    m_testSize.setHeight(testSize);
}

void tst_QStyleSheetStyle::init()
{
    qApp->setStyleSheet(QString());
    QCoreApplication::setAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles, false);
}

void tst_QStyleSheetStyle::cleanup()
{
    QTRY_VERIFY(QApplication::topLevelWidgets().isEmpty());
}

void tst_QStyleSheetStyle::numinstances()
{
    QWidget w;
    w.setWindowTitle(QTest::currentTestFunction());
    w.resize(m_testSize);
    centerOnScreen(&w);
    QCommonStyle *style = new QCommonStyle;
    style->setParent(&w);
    QWidget c(&w);
    w.show();

    // set and unset application stylesheet
    QCOMPARE(QStyleSheetStyle::numinstances, 0);
    qApp->setStyleSheet("* { color: red; }");
    QCOMPARE(QStyleSheetStyle::numinstances, 1);
    qApp->setStyleSheet(QString());
    QCOMPARE(QStyleSheetStyle::numinstances, 0);

    // set and unset application stylesheet+widget
    qApp->setStyleSheet("* { color: red; }");
    w.setStyleSheet("color: red;");
    QCOMPARE(QStyleSheetStyle::numinstances, 2);
    w.setStyle(style);
    QCOMPARE(QStyleSheetStyle::numinstances, 2);
    qApp->setStyleSheet(QString());
    QCOMPARE(QStyleSheetStyle::numinstances, 1);
    w.setStyleSheet(QString());
    QCOMPARE(QStyleSheetStyle::numinstances, 0);

    // set and unset widget stylesheet
    w.setStyle(nullptr);
    w.setStyleSheet("color: red");
    QCOMPARE(QStyleSheetStyle::numinstances, 1);
    c.setStyle(style);
    QCOMPARE(QStyleSheetStyle::numinstances, 2);
    w.setStyleSheet(QString());
    QCOMPARE(QStyleSheetStyle::numinstances, 0);
}

void tst_QStyleSheetStyle::widgetsBeforeAppStyleSheet()
{
    QPushButton w1; // widget with no stylesheet
    const QColor red(Qt::red);
    const QColor white(Qt::white);
    qApp->setStyleSheet("* { color: red; }");
    QCOMPARE(COLOR(w1), red);
    w1.setStyleSheet("color: white");
    QCOMPARE(COLOR(w1), white);
    qApp->setStyleSheet(QString());
    QCOMPARE(COLOR(w1), white);
    w1.setStyleSheet(QString());
    QCOMPARE(COLOR(w1), APPCOLOR(w1));
}

class FriendlySpinBox : public QSpinBox { friend class tst_QStyleSheetStyle; };

void tst_QStyleSheetStyle::widgetsAfterAppStyleSheet()
{
    const QColor red(Qt::red);
    const QColor white(Qt::white);
    qApp->setStyleSheet("* { color: red; font-size: 32pt; }");
    QPushButton w1;
    FriendlySpinBox spin;
    QCOMPARE(COLOR(w1), red);
    QCOMPARE(COLOR(spin), red);
    QCOMPARE(COLOR(*spin.lineEdit()), red);
    QCOMPARE(FONTSIZE(w1), 32);
    QCOMPARE(FONTSIZE(spin), 32);
    QCOMPARE(FONTSIZE(*spin.lineEdit()), 32);
    w1.setStyleSheet("color: white");
    QCOMPARE(COLOR(w1), white);
    QCOMPARE(COLOR(spin), red);
    QCOMPARE(COLOR(*spin.lineEdit()), red);
    w1.setStyleSheet(QString());
    QCOMPARE(COLOR(w1), red);
    QCOMPARE(COLOR(spin), red);
    QCOMPARE(COLOR(*spin.lineEdit()), red);
    w1.setStyleSheet("color: white");
    QCOMPARE(COLOR(w1), white);
    qApp->setStyleSheet(QString());
    QCOMPARE(COLOR(w1), white);
    QCOMPARE(COLOR(spin), APPCOLOR(spin));
    QCOMPARE(COLOR(*spin.lineEdit()), APPCOLOR(*spin.lineEdit()));
    w1.setStyleSheet(QString());
    QCOMPARE(COLOR(w1), APPCOLOR(w1));
    // QCOMPARE(FONTSIZE(w1), APPFONTSIZE(w1));  //### task 244261
    QCOMPARE(FONTSIZE(spin), APPFONTSIZE(spin));
    //QCOMPARE(FONTSIZE(*spin.lineEdit()), APPFONTSIZE(*spin.lineEdit())); //### task 244261
}

void tst_QStyleSheetStyle::applicationStyleSheet()
{
    const QColor red(Qt::red);
    const QColor white(Qt::white);
    QPushButton w1;
    qApp->setStyleSheet("* { color: red; }");
    QCOMPARE(COLOR(w1), red);
    qApp->setStyleSheet("* { color: white; }");
    QCOMPARE(COLOR(w1), white);
    qApp->setStyleSheet(QString());
    QCOMPARE(COLOR(w1), APPCOLOR(w1));
    qApp->setStyleSheet("* { color: red }");
    QCOMPARE(COLOR(w1), red);
}

void tst_QStyleSheetStyle::windowStyleSheet()
{
    const QColor red(Qt::red);
    const QColor white(Qt::white);
    QPushButton w1;
    qApp->setStyleSheet(QString());
    w1.setStyleSheet("* { color: red; }");
    QCOMPARE(COLOR(w1), red);
    w1.setStyleSheet("* { color: white; }");
    QCOMPARE(COLOR(w1), white);
    w1.setStyleSheet(QString());
    QCOMPARE(COLOR(w1), APPCOLOR(w1));
    w1.setStyleSheet("* { color: red }");
    QCOMPARE(COLOR(w1), red);

    qApp->setStyleSheet("* { color: green }");
    QCOMPARE(COLOR(w1), red);
    w1.setStyleSheet(QString());
    QCOMPARE(COLOR(w1), QColor("green"));
    qApp->setStyleSheet(QString());
    QCOMPARE(COLOR(w1), APPCOLOR(w1));
}

void tst_QStyleSheetStyle::widgetStyleSheet()
{
    const QColor blue(Qt::blue);
    const QColor red(Qt::red);
    const QColor white(Qt::white);
    QPushButton w1;
    QPushButton *pb = new QPushButton(&w1);
    QPushButton &w2 = *pb;

    qApp->setStyleSheet(QString());
    w1.setStyleSheet("* { color: red }");
    QCOMPARE(COLOR(w1), red);
    QCOMPARE(COLOR(w2), red);

    w2.setStyleSheet("* { color: white }");
    QCOMPARE(COLOR(w2), white);

    w1.setStyleSheet("* { color: blue }");
    QCOMPARE(COLOR(w1), blue);
    QCOMPARE(COLOR(w2), white);

    w1.setStyleSheet(QString());
    QCOMPARE(COLOR(w1), APPCOLOR(w1));
    QCOMPARE(COLOR(w2), white);

    w2.setStyleSheet(QString());
    QCOMPARE(COLOR(w1), APPCOLOR(w1));
    QCOMPARE(COLOR(w2), APPCOLOR(w2));
}

void tst_QStyleSheetStyle::reparentWithNoChildStyleSheet()
{
    const QColor blue(Qt::blue);
    const QColor red(Qt::red);
    const QColor white(Qt::white);
    QPushButton p1, p2;
    QPushButton *pb = new QPushButton(&p1);
    QPushButton &c1 = *pb; // child with no stylesheet

    qApp->setStyleSheet(QString());
    p1.setStyleSheet("* { color: red }");
    QCOMPARE(COLOR(c1), red);
    c1.setParent(&p2);
    QCOMPARE(COLOR(c1), APPCOLOR(c1));

    p2.setStyleSheet("* { color: white }");
    QCOMPARE(COLOR(c1), white);

    c1.setParent(&p1);
    QCOMPARE(COLOR(c1), red);

    qApp->setStyleSheet("* { color: blue }");
    c1.setParent(nullptr);
    QCOMPARE(COLOR(c1), blue);
    delete pb;
}

void tst_QStyleSheetStyle::reparentWithChildStyleSheet()
{
    const QColor gray("gray");
    const QColor white(Qt::white);
    qApp->setStyleSheet(QString());
    QPushButton p1, p2;
    QPushButton *pb = new QPushButton(&p1);
    QPushButton &c1 = *pb;

    c1.setStyleSheet("background: gray");
    QCOMPARE(BACKGROUND(c1), gray);
    c1.setParent(&p2);
    QCOMPARE(BACKGROUND(c1), gray);

    qApp->setStyleSheet("* { color: white }");
    c1.setParent(&p1);
    QCOMPARE(BACKGROUND(c1), gray);
    QCOMPARE(COLOR(c1), white);
}

void tst_QStyleSheetStyle::repolish()
{
    const QColor red(Qt::red);
    const QColor white(Qt::white);
    qApp->setStyleSheet(QString());
    QPushButton p1;
    p1.setStyleSheet("color: red; background: white");
    QCOMPARE(BACKGROUND(p1), white);
    p1.setStyleSheet("background: white");
    QCOMPARE(COLOR(p1), APPCOLOR(p1));
    p1.setStyleSheet("color: red");
    QCOMPARE(COLOR(p1), red);
    QCOMPARE(BACKGROUND(p1), APPBACKGROUND(p1));
    p1.setStyleSheet(QString());
    QCOMPARE(COLOR(p1), APPCOLOR(p1));
    QCOMPARE(BACKGROUND(p1), APPBACKGROUND(p1));
}

void tst_QStyleSheetStyle::repolish_without_crashing()
{
    // This used to crash, QTBUG-69204
    QMainWindow w;
    w.resize(m_testSize);
    w.setWindowTitle(QTest::currentTestFunction());
    QScopedPointer<QSplitter> splitter1(new QSplitter(w.centralWidget()));
    QScopedPointer<QSplitter> splitter2(new QSplitter);
    QScopedPointer<QSplitter> splitter3(new QSplitter);
    splitter2->addWidget(splitter3.data());

    splitter2->setStyleSheet("color: red");
    QScopedPointer<QLabel> label(new QLabel);
    label->setTextFormat(Qt::RichText);
    splitter3->addWidget(label.data());
    label->setText("hey");

    splitter1->addWidget(splitter2.data());
    w.show();
    QCOMPARE(COLOR(*label), QColor(Qt::red));
}

void tst_QStyleSheetStyle::widgetStyle()
{
    qApp->setStyleSheet(QString());

    QWidget *window1 = new QWidget;
    window1->setObjectName("window1");
    QWidget *widget1 = new QWidget(window1);
    widget1->setObjectName("widget1");
    QWidget *widget2 = new QWidget;
    widget2->setObjectName("widget2");
    QWidget *window2 = new QWidget;
    window2->setObjectName("window2");
    window1->ensurePolished();
    window2->ensurePolished();
    widget1->ensurePolished();
    widget2->ensurePolished();

    QPointer<QStyle> style1 = QStyleFactory::create("Windows");
    QPointer<QStyle> style2 = QStyleFactory::create("Windows");

    QStyle *appStyle = QApplication::style();

    // Sanity: By default, a window inherits the application style
    QCOMPARE(appStyle, window1->style());

    // Setting a custom style on a widget
    window1->setStyle(style1);
    QCOMPARE(style1.data(), window1->style());

    // Setting another style must not delete the older style
    window1->setStyle(style2);
    QCOMPARE(style2.data(), window1->style());
    QVERIFY(!style1.isNull()); // case we have not already crashed

    // Setting null style must make it follow the qApp style
    window1->setStyle(nullptr);
    QCOMPARE(window1->style(), appStyle);
    QVERIFY(!style2.isNull()); // case we have not already crashed
    QVERIFY(!style2.isNull()); // case we have not already crashed

    // Sanity: Set the stylesheet
    window1->setStyleSheet(":x { }");

    QPointer<QStyleSheetStyle> proxy = qobject_cast<QStyleSheetStyle *>(window1->style());
    QVERIFY(!proxy.isNull());
    QCOMPARE(proxy->base, nullptr); // and follows the application

    // Set the stylesheet
    window1->setStyle(style1);
    QVERIFY(proxy.isNull()); // we create a new one each time
    proxy = qobject_cast<QStyleSheetStyle *>(window1->style());
    QVERIFY(!proxy.isNull()); // it is a proxy
    QCOMPARE(proxy->baseStyle(), style1.data()); // must have been replaced with the new one

    // Update the stylesheet and check nothing changes
    window1->setStyleSheet(":y { }");
    QCOMPARE(window1->style()->metaObject()->className(), "QStyleSheetStyle"); // it is a proxy
    QCOMPARE(proxy->baseStyle(), style1.data()); // the same guy

    // Remove the stylesheet
    proxy = qobject_cast<QStyleSheetStyle *>(window1->style());
    window1->setStyleSheet(QString());
    QVERIFY(proxy.isNull()); // should have disappeared
    QCOMPARE(window1->style(), style1.data()); // its restored

    // Style Sheet existing children propagation
    window1->setStyleSheet(":z { }");
    proxy = qobject_cast<QStyleSheetStyle *>(window1->style());
    QVERIFY(!proxy.isNull()); // it is a proxy
    QCOMPARE(window1->style(), widget1->style()); // proxy must have propagated
    QCOMPARE(widget2->style(), appStyle); // widget2 is following the app style

    // Style Sheet automatic propagation to new children
    widget2->setParent(window1); // reparent in!
    QCOMPARE(window1->style(), widget2->style()); // proxy must have propagated

    // Style Sheet automatic removal from children who abandoned their parents
    window2->setStyle(style2);
    widget2->setParent(window2); // reparent
    QCOMPARE(widget2->style(), appStyle); // widget2 is following the app style

    // Style Sheet propagation on a child widget with a custom style
    widget2->setStyle(style1);
    window2->setStyleSheet(":x { }");
    proxy = qobject_cast<QStyleSheetStyle *>(widget2->style());
    QVERIFY(!proxy.isNull()); // it is a proxy
    QCOMPARE(proxy->baseStyle(), style1.data());

    // Style Sheet propagation on a child widget with a custom style already set
    window2->setStyleSheet(QString());
    QCOMPARE(window2->style(), style2.data());
    QCOMPARE(widget2->style(), style1.data());
    widget2->setStyle(nullptr);
    window2->setStyleSheet(":x { }");
    widget2->setStyle(style1);
    proxy = qobject_cast<QStyleSheetStyle *>(widget2->style());
    QVERIFY(!proxy.isNull()); // it is a proxy

    // QApplication, QWidget both having a style sheet

    // clean everything out
    window1->setStyle(nullptr);
    window1->setStyleSheet(QString());
    window2->setStyle(nullptr);
    window2->setStyleSheet(QString());
    QApplication::setStyle(nullptr);

    qApp->setStyleSheet("may_insanity_prevail { }"); // app has stylesheet
    QCOMPARE(window1->style(), QApplication::style());
    QCOMPARE(window1->style()->metaObject()->className(), "QStyleSheetStyle");
    QCOMPARE(widget1->style()->metaObject()->className(), "QStyleSheetStyle"); // check the child
    window1->setStyleSheet("may_more_insanity_prevail { }"); // window has stylesheet
    QCOMPARE(window1->style()->metaObject()->className(), "QStyleSheetStyle"); // a new one
    QCOMPARE(widget1->style(), window1->style()); // child follows...
    proxy = qobject_cast<QStyleSheetStyle *>(window1->style());
    QVERIFY(!proxy.isNull());
    QStyle *newStyle = QStyleFactory::create("Windows");
    QApplication::setStyle(newStyle); // set a custom style on app
    proxy = qobject_cast<QStyleSheetStyle *>(window1->style());
    QVERIFY(!proxy.isNull()); // it is a proxy
    QCOMPARE(proxy->baseStyle(), newStyle); // magic ;) the widget still follows the application
    QCOMPARE(static_cast<QStyle *>(proxy), widget1->style()); // child still follows...

    window1->setStyleSheet(QString()); // remove stylesheet
    QCOMPARE(window1->style(), QApplication::style()); // is this cool or what
    QCOMPARE(widget1->style(), QApplication::style()); // annoying child follows...
    QScopedPointer<QStyle> wndStyle(QStyleFactory::create("Windows"));
    window1->setStyle(wndStyle.data());
    QCOMPARE(window1->style()->metaObject()->className(), "QStyleSheetStyle"); // auto wraps it
    QCOMPARE(widget1->style(), window1->style()); // and auto propagates to child
    qApp->setStyleSheet(QString()); // remove the app stylesheet
    QCOMPARE(window1->style(), wndStyle.data()); // auto dewrap
    QCOMPARE(widget1->style(), QApplication::style()); // and child state is restored
    window1->setStyle(nullptr); // let sanity prevail
    QApplication::setStyle(nullptr);

    delete window1;
    delete widget2;
    delete window2;
    delete style1;
    delete style2;
}

void tst_QStyleSheetStyle::appStyle()
{
    qApp->setStyleSheet(QString());
    // qApp style can never be 0
    QVERIFY(QApplication::style() != nullptr);
    QPointer<QStyle> style1 = QStyleFactory::create("Windows");
    QPointer<QStyle> style2 = QStyleFactory::create("Windows");
    QApplication::setStyle(style1);
    // Basic sanity
    QCOMPARE(QApplication::style(), style1.data());
    QApplication::setStyle(style2);
    QVERIFY(style1.isNull()); // qApp must have taken ownership and deleted it
    // Setting null should not crash
    QApplication::setStyle(nullptr);
    QCOMPARE(QApplication::style(), style2.data());

    // Set the stylesheet
    qApp->setStyleSheet("whatever");
    QPointer<QStyleSheetStyle> sss = static_cast<QStyleSheetStyle *>(QApplication::style());
    QVERIFY(!sss.isNull());
    QCOMPARE(sss->metaObject()->className(), "QStyleSheetStyle"); // must be our proxy now
    QVERIFY(!style2.isNull()); // this should exist as it is the base of the proxy
    QCOMPARE(sss->baseStyle(), style2.data());
    style1 = QStyleFactory::create("Windows");
    QApplication::setStyle(style1);
    QVERIFY(style2.isNull()); // should disappear automatically
    QVERIFY(sss.isNull()); // should disappear automatically

    // Update the stylesheet and check nothing changes
    sss = static_cast<QStyleSheetStyle *>(QApplication::style());
    qApp->setStyleSheet("whatever2");
    QCOMPARE(QApplication::style(), sss.data());
    QCOMPARE(sss->baseStyle(), style1.data());

    // Revert the stylesheet
    qApp->setStyleSheet(QString());
    QVERIFY(sss.isNull()); // should have disappeared
    QCOMPARE(QApplication::style(), style1.data());

    qApp->setStyleSheet(QString());
    QCOMPARE(QApplication::style(), style1.data());
}

void tst_QStyleSheetStyle::dynamicProperty()
{
    qApp->setStyleSheet(QString());

    QString appStyle = QApplication::style()->metaObject()->className();
    QPushButton pb1(QStringLiteral("dynamicProperty_pb1"));
    pb1.setMinimumWidth(m_testSize.width());
    pb1.move(m_availableGeometry.topLeft() + QPoint(20, 100));

    QPushButton pb2(QStringLiteral("dynamicProperty_pb2"));
    pb2.setWindowTitle(QTest::currentTestFunction());
    pb2.setMinimumWidth(m_testSize.width());
    pb2.move(m_availableGeometry.topLeft() + QPoint(20, m_testSize.width() + 40));

    pb1.setProperty("type", "critical");
    qApp->setStyleSheet("*[class~=\"QPushButton\"] { color: red; } *[type=\"critical\"] { background: white; }");
    QVERIFY(COLOR(pb1) == Qt::red);
    QVERIFY(BACKGROUND(pb1) == Qt::white);

    pb2.setProperty("class", "critical"); // dynamic class
    pb2.setStyleSheet(QLatin1String(".critical[style~=\"") + appStyle + "\"] { color: blue }");
    pb2.show();

    QVERIFY(COLOR(pb2) == Qt::blue);
}

#ifdef Q_OS_MAC
void tst_QStyleSheetStyle::layoutSpacing()
{
    qApp->setStyleSheet("* { color: red }");
    QCheckBox ck1;
    QCheckBox ck2;
    QWidget window;
    int spacing_widgetstyle = window.style()->layoutSpacing(ck1.sizePolicy().controlType(), ck2.sizePolicy().controlType(), Qt::Vertical);
    int spacing_style = window.style()->layoutSpacing(ck1.sizePolicy().controlType(), ck2.sizePolicy().controlType(), Qt::Vertical);
    QCOMPARE(spacing_widgetstyle, spacing_style);
}
#endif

void tst_QStyleSheetStyle::qproperty()
{
    QPushButton pb;
    pb.setStyleSheet("QPushButton { qproperty-text: hello; qproperty-checkable: 1; qproperty-checked: false}");
    pb.ensurePolished();
    QCOMPARE(pb.text(), QString("hello"));
    QCOMPARE(pb.isCheckable(), true);
    QCOMPARE(pb.isChecked(), false);
}

namespace ns {
    class PushButton1 : public QPushButton {
        Q_OBJECT
    public:
        using QPushButton::QPushButton;
    };
    class PushButton2 : public PushButton1 {
        Q_OBJECT
    public:
        PushButton2() { setProperty("class", "PushButtonTwo PushButtonDuo"); }
    };
}

void tst_QStyleSheetStyle::namespaces()
{
    const QColor blue(Qt::blue);
    const QColor red(Qt::red);
    const QColor white(Qt::white);
    ns::PushButton1 pb1;
    qApp->setStyleSheet("ns--PushButton1 { background: white }");
    QCOMPARE(BACKGROUND(pb1), white);
    qApp->setStyleSheet(".ns--PushButton1 { background: red }");
    QCOMPARE(BACKGROUND(pb1), red);

    ns::PushButton2 pb2;
    qApp->setStyleSheet("ns--PushButton1 { background: blue}");
    QCOMPARE(BACKGROUND(pb2), blue);
    qApp->setStyleSheet("ns--PushButton2 { background: magenta }");
    QCOMPARE(BACKGROUND(pb2), QColor(Qt::magenta));
    qApp->setStyleSheet(".PushButtonTwo { background: white; }");
    QCOMPARE(BACKGROUND(pb2), white);
    qApp->setStyleSheet(".PushButtonDuo { background: red; }");
    QCOMPARE(BACKGROUND(pb2), red);
}

void tst_QStyleSheetStyle::palettePropagation_data()
{
    QTest::addColumn<QString>("applicationStyleSheet");
    QTest::addColumn<bool>("widgetStylePropagation");
    QTest::newRow("Widget style propagation") << " " << true;
    QTest::newRow("Widget style propagation, no application style sheet") << QString() << true;
    QTest::newRow("Default propagation") << " " << false;
    QTest::newRow("Default propagation, no application style sheet") << QString() << false;
}

void tst_QStyleSheetStyle::palettePropagation()
{
    QFETCH(QString, applicationStyleSheet);
    QFETCH(bool, widgetStylePropagation);

    qApp->setStyleSheet(applicationStyleSheet);
    QCoreApplication::setAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles, widgetStylePropagation);

    QGroupBox gb;
    QLabel *label = new QLabel(&gb);
    QLabel &lb = *label;
    label->setText("AsdF");

    gb.setStyleSheet("QGroupBox { color: red }");
    QCOMPARE(COLOR(gb), QColor(Qt::red));

    if (widgetStylePropagation) {
        QCOMPARE(COLOR(lb), QColor(Qt::red)); // palette should propagate in standard mode
    } else {
        QCOMPARE(COLOR(lb), APPCOLOR(lb)); // palette shouldn't propagate
    }

    QWidget window;
    lb.setParent(&window);
    if (widgetStylePropagation) {
        // In standard propagation mode, widgets that are not explicitly
        // targeted do not have their propagated palette unset when they are
        // unpolished by changing parents.  This is consistent with regular Qt
        // widgets, who also maintain their propagated palette when changing
        // parents
        QCOMPARE(COLOR(lb), QColor(Qt::red));
    } else {
        QCOMPARE(COLOR(lb), APPCOLOR(lb));
    }
    lb.setParent(&gb);

    gb.setStyleSheet("QGroupBox * { color: red }");

    QCOMPARE(COLOR(lb), QColor(Qt::red));
    QCOMPARE(COLOR(gb), APPCOLOR(gb));

    window.setStyleSheet("* { color: white; }");
    lb.setParent(&window);
    QCOMPARE(COLOR(lb), QColor(Qt::white));
}

void tst_QStyleSheetStyle::fontPropagation_data()
{
    QTest::addColumn<QString>("applicationStyleSheet");
    QTest::addColumn<bool>("widgetStylePropagation");
    QTest::newRow("Widget style propagation") << " " << true;
    QTest::newRow("Widget style propagation, no application style sheet") << QString() << true;
    QTest::newRow("Default propagation") << " " << false;
    QTest::newRow("Default propagation, no application style sheet") << QString() << false;
}

void tst_QStyleSheetStyle::fontPropagation()
{
    QFETCH(QString, applicationStyleSheet);
    QFETCH(bool, widgetStylePropagation);

    qApp->setStyleSheet(applicationStyleSheet);
    QCoreApplication::setAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles, widgetStylePropagation);

    QComboBox cb;
    cb.addItem("item1");
    cb.addItem("item2");

    QAbstractItemView *popup = cb.view();
    int viewFontSize = FONTSIZE(*popup);

    cb.setStyleSheet("QComboBox { font-size: 20pt; }");
    QCOMPARE(FONTSIZE(cb), 20);
    if (widgetStylePropagation) {
        QCOMPARE(FONTSIZE(*popup), 20);
    } else {
        QCOMPARE(FONTSIZE(*popup), viewFontSize);
    }
    QGroupBox gb;
    QPushButton *push = new QPushButton(&gb);
    QPushButton &pb = *push;
    int buttonFontSize = FONTSIZE(pb);
    int gbFontSize = FONTSIZE(gb);

    gb.setStyleSheet("QGroupBox { font-size: 20pt }");
    QCOMPARE(FONTSIZE(gb), 20);
    if (widgetStylePropagation) {
        QCOMPARE(FONTSIZE(pb), 20);
    } else {
        QCOMPARE(FONTSIZE(pb), buttonFontSize); // font does not propagate
    }
    gb.setStyleSheet("QGroupBox * { font-size: 20pt; }");
    QCOMPARE(FONTSIZE(gb), gbFontSize);
    QCOMPARE(FONTSIZE(pb), 20);

    QWidget window;
    window.setStyleSheet("* { font-size: 9pt }");
    pb.setParent(&window);
    QCOMPARE(FONTSIZE(pb), 9);
    window.setStyleSheet(QString());
    QCOMPARE(FONTSIZE(pb), buttonFontSize);

    QTabWidget tw;
    tw.setStyleSheet("QTabWidget { font-size: 20pt; }");
    QCOMPARE(FONTSIZE(tw), 20);
    QWidget *child = tw.findChild<QWidget *>("qt_tabwidget_tabbar");
    QVERIFY2(child, "QTabWidget did not contain a widget named \"qt_tabwidget_tabbar\"");
    QCOMPARE(FONTSIZE(*child), 20);
}

void tst_QStyleSheetStyle::onWidgetDestroyed()
{
    qApp->setStyleSheet(QString());
    QLabel *l = new QLabel;
    l->setStyleSheet("QLabel { color: red }");
    QPointer<QStyleSheetStyle> ss = static_cast<QStyleSheetStyle *>(l->style());
    delete l;
    QVERIFY(ss.isNull());
}

void tst_QStyleSheetStyle::fontPrecedence()
{
    QLineEdit edit;
    edit.setWindowTitle(QTest::currentTestFunction());
    edit.setMinimumWidth(m_testSize.width());
    centerOnScreen(&edit);
    edit.show();
    QFont font;
    QVERIFY(FONTSIZE(edit) != 22); // Sanity check to make sure this test makes sense.
    edit.setStyleSheet("QLineEdit { font-size: 22pt; }");
    QCOMPARE(FONTSIZE(edit), 22);
    font.setPointSize(16);
    edit.setFont(font);
    QCOMPARE(FONTSIZE(edit), 22);
    edit.setStyleSheet(QString());
    QCOMPARE(FONTSIZE(edit), 16);
    font.setPointSize(18);
    edit.setFont(font);
    QCOMPARE(FONTSIZE(edit), 18);
    edit.setStyleSheet("QLineEdit { font-size: 20pt; }");
    QCOMPARE(FONTSIZE(edit), 20);
    edit.setStyleSheet(QString());
    QCOMPARE(FONTSIZE(edit), 18);
    edit.hide();

    QLineEdit edit2;
    edit2.setReadOnly(true);
    edit2.setStyleSheet("QLineEdit { font-size: 24pt; } QLineEdit:read-only { font-size: 26pt; }");
    QCOMPARE(FONTSIZE(edit2), 26);
}

// Ensure primary will only return true if the color covers more than 50% of pixels
static bool testForColors(const QImage& image, const QColor &color, bool ensurePrimary = false)
{
    int count = 0;
    QRgb rgb = color.rgba();
    int totalCount = image.height() * image.width();
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            // Because of antialiasing we allow a certain range of errors here.
            QRgb pixel = image.pixel(x, y);

            if (qAbs(int(pixel & 0xff) - int(rgb & 0xff)) +
                    qAbs(int((pixel & 0xff00) >> 8) - int((rgb & 0xff00) >> 8)) +
                    qAbs(int((pixel & 0xff0000) >> 16) - int((rgb & 0xff0000) >> 16)) <= 50) {
                count++;
                if (!ensurePrimary && count >=10 )
                    return true;
                if (count > totalCount / 2)
                    return true;
            }
        }
    }

    return false;
}

class TestDialog : public QDialog
{
public:
    explicit TestDialog(const QString &styleSheet);

    QWidgetList widgets() const { return m_widgets; }
    QLineEdit *focusDummy() const { return m_focusDummy; }

private:
    void addWidget(QWidget *w)
    {
        w->setStyleSheet(m_styleSheet);
        m_layout->addWidget(w);
        m_widgets.append(w);
    }

    const QString m_styleSheet;
    QVBoxLayout* m_layout;
    QLineEdit *m_focusDummy;
    QWidgetList m_widgets;
};

TestDialog::TestDialog(const QString &styleSheet) :
    m_styleSheet(styleSheet),
    m_layout(new QVBoxLayout(this)),
    m_focusDummy(new QLineEdit)
{
    m_layout->addWidget(m_focusDummy); // Avoids initial focus.
    addWidget(new QPushButton("TESTING TESTING"));
    addWidget(new QLineEdit("TESTING TESTING"));
    addWidget(new QLabel("TESTING TESTING"));
    QSpinBox *spinbox = new QSpinBox;
    spinbox->setMaximum(1000000000);
    spinbox->setValue(123456789);
    addWidget(spinbox);
    QComboBox *combobox = new QComboBox;
    combobox->setEditable(true);
    combobox->addItems(QStringList{"TESTING TESTING"});
    addWidget(combobox);
    addWidget(new QLabel("<b>TESTING TESTING</b>"));
}

void tst_QStyleSheetStyle::focusColors()
{
    // Tests if colors can be changed by altering the focus of the widget.
    // To avoid messy pixel-by-pixel comparison, we assume that the goal
    // is reached if at least ten pixels of the right color can be found in
    // the image.
    // For this reason, we use unusual and extremely ugly colors! :-)
    // Note that in case of anti-aliased text, ensuring that we have at least
    // ten pixels of the right color requires quite a many characters, as the
    // majority of the pixels will have slightly different colors due to the
    // anti-aliasing effect.
#if !defined(Q_OS_WIN32) && !(defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(Q_CC_INTEL))
    QSKIP("This is a fragile test which fails on many esoteric platforms because of focus problems"
          " (for example, QTBUG-33959)."
          "That doesn't mean that the feature doesn't work in practice.");
#endif

    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    TestDialog frame(QStringLiteral("*:focus { border:none; background: #e8ff66; color: #ff0084 }"));
    frame.setWindowTitle(QTest::currentTestFunction());

    centerOnScreen(&frame);
    frame.show();

    QApplication::setActiveWindow(&frame);
    QVERIFY(QTest::qWaitForWindowActive(&frame));

    for (QWidget *widget : frame.widgets()) {
        widget->setFocus();
        QApplication::processEvents();

        QImage image(widget->width(), widget->height(), QImage::Format_ARGB32);
        widget->render(&image);
        if (image.depth() < 24)
            QSKIP("Test doesn't support color depth < 24");

        QVERIFY2(testForColors(image, QColor(0xe8, 0xff, 0x66)),
                (QString::fromLatin1(widget->metaObject()->className())
                + " did not contain background color #e8ff66, using style "
                + QString::fromLatin1(QApplication::style()->metaObject()->className()))
                .toLocal8Bit().constData());
        QVERIFY2(testForColors(image, QColor(0xff, 0x00, 0x84)),
                (QString::fromLatin1(widget->metaObject()->className())
                + " did not contain text color #ff0084, using style "
                + QString::fromLatin1(QApplication::style()->metaObject()->className()))
                .toLocal8Bit().constData());
    }
}

#ifndef QT_NO_CURSOR
void tst_QStyleSheetStyle::hoverColors()
{
#ifdef Q_OS_MACOS
    QSKIP("This test is fragile on Mac, most likely due to QTBUG-33959.");
#endif
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    TestDialog frame(QStringLiteral("*:hover { border:none; background: #e8ff66; color: #ff0084 }"));
    frame.setWindowTitle(QTest::currentTestFunction());

    centerOnScreen(&frame);
    // Move the mouse cursor out of the way to suppress spontaneous QEvent::Enter
    // events interfering with QApplicationPrivate::dispatchEnterLeave().
    // Mouse events can then be sent directly to the QWindow instead of the
    // QWidget, triggering the enter/leave handling within the dialog window,
    // speeding up the test.
    QCursor::setPos(frame.geometry().topLeft() - QPoint(100, 0));
    frame.show();

    QApplication::setActiveWindow(&frame);
    QVERIFY(QTest::qWaitForWindowActive(&frame));

    QWindow *frameWindow = frame.windowHandle();
    QVERIFY(frameWindow);

    const QPoint dummyPos = frame.focusDummy()->geometry().center();
    QTest::mouseMove(frameWindow, dummyPos);

    for (QWidget *widget : frame.widgets()) {
        //move the mouse inside the widget, it should be colored
        const QRect widgetGeometry = widget->geometry();
        QTest::mouseMove(frameWindow, widgetGeometry.center());
        QTRY_VERIFY2(widget->testAttribute(Qt::WA_UnderMouse), widget->metaObject()->className());

        QImage image(widgetGeometry.size(), QImage::Format_ARGB32);
        widget->render(&image);

        QVERIFY2(testForColors(image, QColor(0xe8, 0xff, 0x66)),
                  (QString::fromLatin1(widget->metaObject()->className())
                  + " did not contain background color #e8ff66").toLocal8Bit().constData());
        QVERIFY2(testForColors(image, QColor(0xff, 0x00, 0x84)),
                 (QString::fromLatin1(widget->metaObject()->className())
                  + " did not contain text color #ff0084").toLocal8Bit().constData());

        //move the mouse outside the widget, it should NOT be colored
        QTest::mouseMove(frameWindow, dummyPos);
        QTRY_VERIFY2(frame.focusDummy()->testAttribute(Qt::WA_UnderMouse), "FocusDummy");

        widget->render(&image);

        QVERIFY2(!testForColors(image, QColor(0xe8, 0xff, 0x66)),
                  (QString::fromLatin1(widget->metaObject()->className())
                  + " did contain background color #e8ff66").toLocal8Bit().constData());
        QVERIFY2(!testForColors(image, QColor(0xff, 0x00, 0x84)),
                 (QString::fromLatin1(widget->metaObject()->className())
                  + " did contain text color #ff0084").toLocal8Bit().constData());

        //move the mouse again inside the widget, it should be colored
        QTest::mouseMove (frameWindow, widgetGeometry.center());
        QTRY_VERIFY2(widget->testAttribute(Qt::WA_UnderMouse), widget->metaObject()->className());

        widget->render(&image);

        QVERIFY2(testForColors(image, QColor(0xe8, 0xff, 0x66)),
                 (QString::fromLatin1(widget->metaObject()->className())
                 + " did not contain background color #e8ff66").toLocal8Bit().constData());
        QVERIFY2(testForColors(image, QColor(0xff, 0x00, 0x84)),
                (QString::fromLatin1(widget->metaObject()->className())
                + " did not contain text color #ff0084").toLocal8Bit().constData());
    }
}
#endif

class SingleInheritanceDialog : public QDialog
{
    Q_OBJECT
public:
    using QDialog::QDialog;
};

class DoubleInheritanceDialog : public SingleInheritanceDialog
{
    Q_OBJECT
public:
    using SingleInheritanceDialog::SingleInheritanceDialog;
};

void tst_QStyleSheetStyle::background()
{
    typedef QSharedPointer<QWidget> WidgetPtr;

    const QString styleSheet = QStringLiteral("* { background-color: #e8ff66; }");
    QVector<WidgetPtr> widgets;
    const QPoint topLeft = m_availableGeometry.topLeft();
    // Testing inheritance styling of QDialog.
    WidgetPtr toplevel(new SingleInheritanceDialog);
    toplevel->resize(m_testSize);
    toplevel->move(topLeft + QPoint(20, 20));
    toplevel->setStyleSheet(styleSheet);
    widgets.append(toplevel);

    toplevel = WidgetPtr(new DoubleInheritanceDialog);
    toplevel->resize(m_testSize);
    toplevel->move(topLeft + QPoint(20, m_testSize.height() + 120));
    toplevel->setStyleSheet(styleSheet);
    widgets.append(toplevel);

    // Testing gradients in QComboBox.
    // First color
    toplevel = WidgetPtr(new QDialog);
    toplevel->resize(m_testSize);
    toplevel->move(topLeft + QPoint(m_testSize.width() + 120, 20));
    QGridLayout *layout = new QGridLayout(toplevel.data());
    QComboBox* cb = new QComboBox;
    cb->setMinimumWidth(160);
    cb->setStyleSheet("* { background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop:0 #e8ff66, stop:1 #000000); }");
    layout->addWidget(cb, 0, 0);
    widgets.append(toplevel);
    // Second color
    toplevel = WidgetPtr(new QDialog);
    toplevel->resize(m_testSize);
    toplevel->move(topLeft + QPoint(m_testSize.width() + 120, m_testSize.height() + 120));
    layout = new QGridLayout(toplevel.data());
    cb = new QComboBox;
    cb->setMinimumWidth(160);
    cb->setStyleSheet("* { background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop:0 #e8ff66, stop:1 #000000); }");
    layout->addWidget(cb, 0, 0);
    widgets.append(toplevel);

    for (int c = 0; c < widgets.size(); ++c) {
        QWidget *widget = widgets.at(c).data();
        widget->setWindowTitle(QStringLiteral("background ") + QString::number(c));
        widget->show();
        QVERIFY(QTest::qWaitForWindowExposed(widget));

        QImage image(widget->width(), widget->height(), QImage::Format_ARGB32);
        widget->render(&image);
        if (image.depth() < 24)
            QSKIP("Test doesn't support color depth < 24");

        if (c == 2 && !QApplication::style()->objectName().compare(QLatin1String("fusion"), Qt::CaseInsensitive))
            QEXPECT_FAIL("", "QTBUG-21468", Abort);

        QVERIFY2(testForColors(image, QColor(0xe8, 0xff, 0x66)),
                (QString::number(c) + QLatin1Char(' ') + QString::fromLatin1(widget->metaObject()->className())
                + " did not contain background image with color #e8ff66")
                .toLocal8Bit().constData());

        widget->hide();
    }
}

void tst_QStyleSheetStyle::tabAlignment()
{
    QWidget topLevel;
    topLevel.setWindowTitle(QTest::currentTestFunction());
    QTabWidget tabWidget(&topLevel);
    tabWidget.addTab(new QLabel("tab1"),"tab1");
    tabWidget.resize(QSize(400,400));
    centerOnScreen(&topLevel);
    topLevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    QTabBar *bar = tabWidget.findChild<QTabBar*>();
    QVERIFY(bar);
    //check the tab is on the right
    tabWidget.setStyleSheet("QTabWidget::tab-bar { alignment: right ; }");
    qApp->processEvents();
    QVERIFY(bar->geometry().right() > 380);
    QVERIFY(bar->geometry().left() > 200);
    //check the tab is on the middle
    tabWidget.setStyleSheet("QTabWidget::tab-bar { alignment: center ; }");
    QVERIFY(bar->geometry().right() < 300);
    QVERIFY(bar->geometry().left() > 100);
    //check the tab is on the left
    tabWidget.setStyleSheet("QTabWidget::tab-bar { alignment: left ; }");
    QVERIFY(bar->geometry().left() < 20);
    QVERIFY(bar->geometry().right() < 200);

    tabWidget.setTabPosition(QTabWidget::West);
    //check the tab is on the top
    QVERIFY(bar->geometry().top() < 20);
    QVERIFY(bar->geometry().bottom() < 200);
    //check the tab is on the bottom
    tabWidget.setStyleSheet("QTabWidget::tab-bar { alignment: right ; }");
    QVERIFY(bar->geometry().bottom() > 380);
    QVERIFY(bar->geometry().top() > 200);
    //check the tab is on the middle
    tabWidget.setStyleSheet("QTabWidget::tab-bar { alignment: center ; }");
    QVERIFY(bar->geometry().bottom() < 300);
    QVERIFY(bar->geometry().top() > 100);
}

void tst_QStyleSheetStyle::attributesList()
{
    const QColor blue(Qt::blue);
    const QColor red(Qt::red);
    QWidget w;
    QPushButton *p1=new QPushButton(&w);
    QPushButton *p2=new QPushButton(&w);
    QPushButton *p3=new QPushButton(&w);
    QPushButton *p4=new QPushButton(&w);
    p1->setProperty("prop", QStringList() << "red");
    p2->setProperty("prop", QStringList() << "foo" << "red");
    p3->setProperty("prop", QStringList() << "foo" << "bar");

    w.setStyleSheet(" QPushButton{ background-color:blue; }  QPushButton[prop~=red] { background-color:red; }");
    QCOMPARE(BACKGROUND(*p1) , red);
    QCOMPARE(BACKGROUND(*p2) , red);
    QCOMPARE(BACKGROUND(*p3) , blue);
    QCOMPARE(BACKGROUND(*p4) , blue);
}

void tst_QStyleSheetStyle::minmaxSizes()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QTabWidget tabWidget;
    tabWidget.resize(m_testSize);
    tabWidget.setWindowTitle(QTest::currentTestFunction());
    tabWidget.setObjectName("tabWidget");
    int index1 = tabWidget.addTab(new QLabel("Tab1"),"a");

    QWidget *page2=new QLabel("page2");
    page2->setObjectName("page2");
    page2->setStyleSheet("* {background-color: white; min-width: 250px; max-width:500px }");
    tabWidget.addTab(page2,"Tab2");
    QWidget *page3=new QLabel("plop");
    page3->setObjectName("Page3");
    page3->setStyleSheet("* {background-color: yellow; min-height: 250px; max-height:500px }");
    int index3 = tabWidget.addTab(page3,"very_long_long_long_long_caption");

    tabWidget.setStyleSheet("QTabBar::tab { min-width:100px; max-width:130px; }");

    centerOnScreen(&tabWidget);
    tabWidget.show();
    QVERIFY(QTest::qWaitForWindowActive(&tabWidget));
    //i allow 4px additional border from the native style (hence the -2, <=2)
    QVERIFY(qAbs(page2->maximumSize().width() - 500 - 2) <= 2);
    QVERIFY(qAbs(page2->minimumSize().width() - 250 - 2) <= 2);
    QVERIFY(qAbs(page3->maximumSize().height() - 500 - 2) <= 2);
    QVERIFY(qAbs(page3->minimumSize().height() - 250 - 2) <= 2);
    QVERIFY(qAbs(page3->minimumSize().height() - 250 - 2) <= 2);
    QTabBar *bar = tabWidget.findChild<QTabBar*>();
    QVERIFY(bar);
#ifdef Q_OS_MAC
    QEXPECT_FAIL("", "QTBUG-23686", Continue);
#endif
    QVERIFY(qAbs(bar->tabRect(index1).width() - 100 - 2) <= 2);
#ifdef Q_OS_MAC
    QEXPECT_FAIL("", "QTBUG-23686", Continue);
#endif
    QVERIFY(qAbs(bar->tabRect(index3).width() - 130 - 2) <= 2);
}

void tst_QStyleSheetStyle::task206238_twice()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    const QColor red(Qt::red);
    QMainWindow w;
    w.resize(m_testSize);
    w.setWindowTitle(QTest::currentTestFunction());
    QTabWidget* tw = new QTabWidget;
    tw->addTab(new QLabel("foo"), "test");
    w.setCentralWidget(tw);
    w.setStyleSheet("background: red;");
    centerOnScreen(&w);
    w.show();
    QVERIFY(QTest::qWaitForWindowActive(&w));
    QCOMPARE(BACKGROUND(w) , red);
    QCOMPARE(BACKGROUND(*tw), red);
    w.setStyleSheet("background: red;");
    QTest::qWait(20);
    QCOMPARE(BACKGROUND(w) , red);
    QCOMPARE(BACKGROUND(*tw), red);
}

void tst_QStyleSheetStyle::transparent()
{
    QWidget w;
    QPushButton *p1=new QPushButton(&w);
    QPushButton *p2=new QPushButton(&w);
    QPushButton *p3=new QPushButton(&w);
    p1->setStyleSheet("background:transparent");
    p2->setStyleSheet("background-color:transparent");
    p3->setStyleSheet("background:rgba(0,0,0,0)");
    QCOMPARE(BACKGROUND(*p1) , QColor(0,0,0,0));
    QCOMPARE(BACKGROUND(*p2) , QColor(0,0,0,0));
    QCOMPARE(BACKGROUND(*p3) , QColor(0,0,0,0));
}



class ProxyStyle : public QStyle
{
    Q_OBJECT

    public:
        ProxyStyle(QStyle *s)
        {
            style = s;
        }

        void drawControl(ControlElement ce, const QStyleOption *opt,
                         QPainter *painter, const QWidget *widget = nullptr) const override;

        void drawPrimitive(QStyle::PrimitiveElement pe,
                           const QStyleOption* opt,
                           QPainter *p,
                           const QWidget *w) const override
        {
            style->drawPrimitive(pe, opt, p, w);
        }

        QRect subElementRect(QStyle::SubElement se,
                             const QStyleOption *opt,
                             const QWidget *w) const override
        {
            Q_UNUSED(se);
            Q_UNUSED(opt);
            Q_UNUSED(w);
            return QRect(0, 0, 3, 3);
        }

        void drawComplexControl(QStyle::ComplexControl cc,
                                const QStyleOptionComplex *opt,
                                QPainter *p,
                                const QWidget *w) const override
        {
            style->drawComplexControl(cc, opt, p, w);
        }


        SubControl hitTestComplexControl(QStyle::ComplexControl cc,
                                         const QStyleOptionComplex *opt,
                                         const QPoint &pt,
                                         const QWidget *w) const override
        {
            return style->hitTestComplexControl(cc, opt, pt, w);
        }

        QRect subControlRect(QStyle::ComplexControl cc,
                             const QStyleOptionComplex *opt,
                             QStyle::SubControl sc,
                             const QWidget *w) const override
        {
            return style->subControlRect(cc, opt, sc, w);
        }

        int pixelMetric(QStyle::PixelMetric pm,
                        const QStyleOption *opt,
                        const QWidget *w) const override
        {
            return style->pixelMetric(pm, opt, w);
        }


        QSize sizeFromContents(QStyle::ContentsType ct,
                               const QStyleOption *opt,
                               const QSize &size,
                               const QWidget *w) const override
        {
            return style->sizeFromContents(ct, opt, size, w);
        }

        int styleHint(QStyle::StyleHint sh,
                      const QStyleOption *opt,
                      const QWidget *w,
                      QStyleHintReturn *shr) const override
        {
            return style->styleHint(sh, opt, w, shr);
        }

        QPixmap standardPixmap(QStyle::StandardPixmap spix,
                               const QStyleOption *opt,
                               const QWidget *w) const override
        {
            return style->standardPixmap(spix, opt, w);
        }

        QPixmap generatedIconPixmap(QIcon::Mode mode,
                                    const QPixmap &pix,
                                    const QStyleOption *opt) const override
        {
            return style->generatedIconPixmap(mode, pix, opt);
        }

        int layoutSpacing(QSizePolicy::ControlType c1,
                          QSizePolicy::ControlType c2,
                          Qt::Orientation ori,
                          const QStyleOption *opt,
                          const QWidget *w) const override
        {
            return style->layoutSpacing(c1, c2, ori, opt, w);
        }

        QIcon standardIcon(StandardPixmap si,
                           const QStyleOption *opt,
                           const QWidget *w) const override
        {
            return style->standardIcon(si, opt, w);
        }

    private:
        QStyle *style;
};

void ProxyStyle::drawControl(ControlElement ce, const QStyleOption *opt,
                             QPainter *painter, const QWidget *widget) const
{
    if(ce == CE_PushButton)
    {
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt))
        {
            QRect r = btn->rect;
            painter->fillRect(r, Qt::green);

            if(btn->state & QStyle::State_HasFocus)
                painter->fillRect(r.adjusted(5, 5, -5, -5), Qt::yellow);


            painter->drawText(r, Qt::AlignCenter, btn->text);
        }
    }
    else
    {
        style->drawControl(ce, opt, painter, widget);
    }
}

void tst_QStyleSheetStyle::proxyStyle()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    //Should not crash;   task 158984

    ProxyStyle *proxy = new ProxyStyle(QApplication::style());
    QString styleSheet("QPushButton {background-color: red; }");

    QWidget *w = new QWidget;
    w->setMinimumWidth(m_testSize.width());
    centerOnScreen(w);
    QVBoxLayout *layout = new QVBoxLayout(w);

    QPushButton *pb1 = new QPushButton(QApplication::style()->objectName(), w);
    layout->addWidget(pb1);

    QPushButton *pb2 = new QPushButton("ProxyStyle", w);
    pb2->setStyle(proxy);
    layout->addWidget(pb2);

    QPushButton *pb3 = new QPushButton("StyleSheet", w);
    pb3->setStyleSheet(styleSheet);
    layout->addWidget(pb3);

    QPushButton *pb4 = new QPushButton("StyleSheet then ProxyStyle ", w);
    pb4->setStyleSheet(styleSheet);

    // We are creating our Proxy based on current style...
    // In this case it would be the QStyleSheetStyle that is deleted
    // later on. We need to get access to the "real" QStyle to be able to
    // draw correctly.
    ProxyStyle *newProxy = new ProxyStyle(QApplication::style());
    pb4->setStyle(newProxy);

    layout->addWidget(pb4);

    QPushButton *pb5 = new QPushButton("ProxyStyle then StyleSheet ", w);
    pb5->setStyle(proxy);
    pb5->setStyleSheet(styleSheet);
    layout->addWidget(pb5);

    w->show();
    QVERIFY(QTest::qWaitForWindowActive(w));

    // Test for QTBUG-7198 - style sheet overrides custom element size
    QStyleOptionViewItem opt;
    opt.initFrom(w);
    opt.features |= QStyleOptionViewItem::HasCheckIndicator;
    QVERIFY(pb5->style()->subElementRect(QStyle::SE_ItemViewItemCheckIndicator,
            &opt, pb5).width() == 3);
    delete w;
    delete proxy;
    delete newProxy;
}

void tst_QStyleSheetStyle::dialogButtonBox()
{
    QDialogButtonBox box;
    box.addButton(QDialogButtonBox::Ok);
    box.addButton(QDialogButtonBox::Cancel);
    box.setStyleSheet("/** */ ");
    box.setStyleSheet(QString()); //should not crash
}

void tst_QStyleSheetStyle::emptyStyleSheet()
{
    //empty stylesheet should not change anything
    qApp->setStyleSheet(QString());
    QWidget w;
    w.setWindowTitle(QTest::currentTestFunction());
    QHBoxLayout layout(&w);
    w.setLayout(&layout);
    layout.addWidget(new QPushButton("push", &w));
    layout.addWidget(new QToolButton(&w));
    QLabel label("toto", &w);
    label.setFrameShape(QLabel::Panel);
    label.setFrameShadow(QLabel::Sunken);
    layout.addWidget(&label); //task 231137
    layout.addWidget(new QTableWidget(200,200, &w));
    layout.addWidget(new QProgressBar(&w));
    layout.addWidget(new QLineEdit(&w));
    layout.addWidget(new QSpinBox(&w));
    layout.addWidget(new QComboBox(&w));
    layout.addWidget(new QDateEdit(&w));
    layout.addWidget(new QGroupBox("some text", &w));

    centerOnScreen(&w);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    //workaround the fact that the label sizehint is one pixel different the first time.
    label.setIndent(0); //force to recompute the sizeHint:
    w.setFocus();
    QTest::qWait(100);

    QImage img1(w.size(), QImage::Format_ARGB32);
    w.render(&img1);

    w.setStyleSheet("/* */");
    QTest::qWait(100);

    QImage img2(w.size(), QImage::Format_ARGB32);
    w.render(&img2);

    if(img1 != img2) {
        img1.save("emptyStyleSheet_img1.png");
        img2.save("emptyStyleSheet_img2.png");
    }

    QEXPECT_FAIL("", "QTBUG-21468", Abort);
    QCOMPARE(img1,img2);
}

class ApplicationStyleSetter
{
public:
    explicit inline ApplicationStyleSetter(QStyle *s) : m_oldStyleName(QApplication::style()->objectName())
     { QApplication::setStyle(s); }
    inline ~ApplicationStyleSetter()
     { QApplication::setStyle(QStyleFactory::create(m_oldStyleName)); }

private:
    const QString m_oldStyleName;
};

void tst_QStyleSheetStyle::toolTip()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    qApp->setStyleSheet(QString());
    QWidget w;
    w.resize(m_testSize);
    w.setWindowTitle(QTest::currentTestFunction());
    // Use "Fusion" to prevent the Vista style from clobbering the tooltip palette in polish().
    QStyle *fusionStyle = QStyleFactory::create(QLatin1String("Fusion"));
    QVERIFY(fusionStyle);
    ApplicationStyleSetter as(fusionStyle);
    QHBoxLayout layout(&w);
    w.setLayout(&layout);

    QWidget *wid1 = new QGroupBox(&w);
    layout.addWidget(wid1);
    wid1->setStyleSheet("QToolTip { background: #ae2; }   #wid3 > QToolTip { background: #0b8; } ");
    QVBoxLayout *layout1 = new QVBoxLayout(wid1);
    wid1->setLayout(layout1);
    wid1->setToolTip("this is wid1");
    wid1->setObjectName("wid1");

    QWidget *wid2 = new QPushButton("wid2", wid1);
    layout1->addWidget(wid2);
    wid2->setStyleSheet("QToolTip { background: #f81; } ");
    wid2->setToolTip("this is wid2");
    wid2->setObjectName("wid2");

    QWidget *wid3 = new QPushButton("wid3", wid1);
    layout1->addWidget(wid3);
    wid3->setToolTip("this is wid3");
    wid3->setObjectName("wid3");

    QWidget *wid4 = new QPushButton("wid4", &w);
    layout.addWidget(wid4);
    wid4->setToolTip("this is wid4");
    wid4->setObjectName("wid4");

    centerOnScreen(&w);
    w.show();
    QApplication::setActiveWindow(&w);
    QVERIFY(QTest::qWaitForWindowActive(&w));

    const QColor normalToolTip = QToolTip::palette().color(QPalette::Inactive, QPalette::ToolTipBase);
    // Tooltip on the widget without stylesheet, then to other widget,
    // including one without stylesheet (the tooltip will be reused,
    // but its color must change)
    const QWidgetList widgets{wid4, wid1, wid2, wid3, wid4};
    const QVector<QColor> colors{normalToolTip, QColor("#ae2"), QColor("#f81"),
                                 QColor("#0b8"), normalToolTip};

    QWidgetList topLevels;
    for (int i = 0; i < widgets.count() ; ++i) {
        QWidget *wid = widgets.at(i);
        QColor col = colors.at(i);

        QToolTip::showText( QPoint(0,0) , "This is " + wid->objectName(), wid);

        topLevels = QApplication::topLevelWidgets();
        QWidget *tooltip = nullptr;
        for (QWidget *widget : qAsConst(topLevels)) {
            if (widget->inherits("QTipLabel")) {
                tooltip = widget;
                break;
            }
        }
        QVERIFY(tooltip);
        QTRY_VERIFY(tooltip->isVisible()); // Wait until Roll-Effect is finished (Windows Vista)
        QCOMPARE(tooltip->palette().color(tooltip->backgroundRole()), col);
    }

    QToolTip::showText( QPoint(0,0) , "This is " + wid3->objectName(), wid3);
    QTest::qWait(100);
    delete wid3; //should not crash;
    QTest::qWait(10);
    topLevels = QApplication::topLevelWidgets();
    for (QWidget *widget : qAsConst(topLevels))
        widget->update(); //should not crash either
}

void tst_QStyleSheetStyle::embeddedFonts()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    //task 235622 and 210551
    QSpinBox spin;
    spin.setWindowTitle(QTest::currentTestFunction());
    spin.setMinimumWidth(m_testSize.width());
    spin.move(m_availableGeometry.topLeft() + QPoint(20, 20));
    spin.show();
    spin.setStyleSheet("QSpinBox { font-size: 32px; }");
    QTest::qWait(20);
    QLineEdit *embedded = spin.findChild<QLineEdit *>();
    QVERIFY(embedded);
    QCOMPARE(spin.font().pixelSize(), 32);
    QCOMPARE(embedded->font().pixelSize(), 32);

#ifndef QT_NO_CONTEXTMENU
    QMenu *menu = embedded->createStandardContextMenu();
    menu->show();
    QTest::qWait(20);
    QVERIFY(menu);
    QVERIFY(menu->font().pixelSize() != 32);
    QCOMPARE(menu->font().pixelSize(), qApp->font(menu).pixelSize());
#endif // QT_NO_CONTEXTMENU

    //task 242556
    QComboBox box;
    box.setMinimumWidth(160);
    box.move(m_availableGeometry.topLeft() + QPoint(20, 120));
    box.setEditable(true);
    box.addItems(QStringList() << "First" << "Second" << "Third");
    box.setStyleSheet("QComboBox { font-size: 32px; }");
    box.show();
    QVERIFY(QTest::qWaitForWindowActive(&box));
    embedded = box.findChild<QLineEdit *>();
    QVERIFY(embedded);
    QCOMPARE(box.font().pixelSize(), 32);
    QCOMPARE(embedded->font().pixelSize(), 32);
}

void tst_QStyleSheetStyle::opaquePaintEvent_data()
{
    QTest::addColumn<QString>("stylesheet");
    QTest::addColumn<bool>("transparent");
    QTest::addColumn<bool>("styled");

    QTest::newRow("none") << QString::fromLatin1("/* */") << false << false;
    QTest::newRow("background black ") << QString::fromLatin1("background: black;") << false << true;
    QTest::newRow("background qrgba") << QString::fromLatin1("background: rgba(125,0,0,125);") << true << true;
    QTest::newRow("background transparent") << QString::fromLatin1("background: transparent;") << true << true;
    QTest::newRow("border native") << QString::fromLatin1("border: native;") << false << false;
    QTest::newRow("border solid") << QString::fromLatin1("border: 2px solid black;") << true << true;
    QTest::newRow("border transparent") << QString::fromLatin1("background: black; border: 2px solid rgba(125,0,0,125);") << true << true;
    QTest::newRow("margin") << QString::fromLatin1("margin: 25px;") << true << true;
    QTest::newRow("focus") << QString::fromLatin1("*:focus { background: rgba(125,0,0,125) }") << true << true;
}

void tst_QStyleSheetStyle::opaquePaintEvent()
{
    QFETCH(QString, stylesheet);
    QFETCH(bool, transparent);
    QFETCH(bool, styled);

    QWidget tl;
    QWidget cl(&tl);
    cl.setAttribute(Qt::WA_OpaquePaintEvent, true);
    cl.setAutoFillBackground(true);
    cl.setStyleSheet(stylesheet);
    cl.ensurePolished();
    QCOMPARE(cl.testAttribute(Qt::WA_OpaquePaintEvent), !transparent);
    QCOMPARE(cl.testAttribute(Qt::WA_StyledBackground), styled);
    QCOMPARE(cl.autoFillBackground(), !styled );
}

void tst_QStyleSheetStyle::complexWidgetFocus()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    // This test is a simplified version of the focusColors() test above.

    // Tests if colors can be changed by altering the focus of the widget.
    // To avoid messy pixel-by-pixel comparison, we assume that the goal
    // is reached if at least ten pixels of the right color can be found in
    // the image.
    // For this reason, we use unusual and extremely ugly colors! :-)

    QDialog frame;
    frame.setWindowTitle(QTest::currentTestFunction());
    frame.setStyleSheet("*:focus { background: black; color: black } "
                        "QSpinBox::up-arrow:focus, QSpinBox::down-arrow:focus { width: 7px; height: 7px; background: #ff0084 } "
                        "QComboBox::down-arrow:focus { width: 7px; height: 7px; background: #ff0084 }"
                        "QSlider::handle:horizontal:focus { width: 7px; height: 7px; background: #ff0084 } ");

    const QWidgetList widgets{new QSpinBox, new QComboBox, new QSlider(Qt::Horizontal)};

    QLayout* layout = new QGridLayout;
    layout->addWidget(new QLineEdit); // Avoids initial focus.
    for (QWidget *widget : widgets)
        layout->addWidget(widget);
    frame.setLayout(layout);

    centerOnScreen(&frame);
    frame.show();
    QApplication::setActiveWindow(&frame);
    QVERIFY(QTest::qWaitForWindowActive(&frame));
    for (QWidget *widget : widgets) {
        widget->setFocus();
        QApplication::processEvents();

        QImage image(frame.width(), frame.height(), QImage::Format_ARGB32);
        frame.render(&image);
        if (image.depth() < 24)
            QSKIP("Test doesn't support color depth < 24");

        QVERIFY2(testForColors(image, QColor(0xff, 0x00, 0x84)),
                (QString::fromLatin1(widget->metaObject()->className())
                + " did not contain text color #ff0084, using style "
                + QString::fromLatin1(QApplication::style()->metaObject()->className()))
                .toLocal8Bit().constData());
    }
}

void tst_QStyleSheetStyle::task188195_baseBackground()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    QTreeView tree;
    tree.setWindowTitle(QTest::currentTestFunction());
    tree.setStyleSheet( "QTreeView:disabled { background-color:#ab1251; }" );
    tree.setGeometry(QRect(m_availableGeometry.topLeft() + QPoint(20, 100), m_testSize));
    tree.show();
    QVERIFY(QTest::qWaitForWindowActive(&tree));
    QImage image(tree.width(), tree.height(), QImage::Format_ARGB32);

    tree.render(&image);
    QVERIFY(testForColors(image, tree.palette().base().color()));
    QVERIFY(!testForColors(image, QColor(0xab, 0x12, 0x51)));

    tree.setEnabled(false);
    tree.render(&image);
    QVERIFY(testForColors(image, QColor(0xab, 0x12, 0x51)));

    tree.setEnabled(true);
    tree.render(&image);
    QVERIFY(testForColors(image, tree.palette().base().color()));
    QVERIFY(!testForColors(image, QColor(0xab, 0x12, 0x51)));

    QTableWidget table(12, 12);
    table.setItem(0, 0, new QTableWidgetItem());
    table.setStyleSheet( "QTableView {background-color: #ff0000}" );
    // This needs to be large so that >50% (excluding header rows/columns) are red.
    table.setGeometry(QRect(m_availableGeometry.topLeft() + QPoint(300, 100), m_testSize * 2));
    table.show();
    QVERIFY(QTest::qWaitForWindowActive(&table));
    image = QImage(table.width(), table.height(), QImage::Format_ARGB32);
    table.render(&image);
    QVERIFY(testForColors(image, Qt::red, true));
}

void tst_QStyleSheetStyle::task232085_spinBoxLineEditBg()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    // This test is a simplified version of the focusColors() test above.

    // Tests if colors can be changed by altering the focus of the widget.
    // To avoid messy pixel-by-pixel comparison, we assume that the goal
    // is reached if at least ten pixels of the right color can be found in
    // the image.
    // For this reason, we use unusual and extremely ugly colors! :-)

    QSpinBox *spinbox = new QSpinBox;
    spinbox->setValue(8888);

    QDialog frame;
    frame.setWindowTitle(QTest::currentTestFunction());
    QLayout* layout = new QGridLayout;

    QLineEdit* dummy = new QLineEdit; // Avoids initial focus.

    // We only want to test the line edit colors.
    spinbox->setStyleSheet("QSpinBox:focus { background: #e8ff66; color: #ff0084 } "
                           "QSpinBox::up-button, QSpinBox::down-button { background: black; }");

    layout->addWidget(dummy);
    layout->addWidget(spinbox);
    frame.setLayout(layout);

    centerOnScreen(&frame);
    frame.show();
    QApplication::setActiveWindow(&frame);
    spinbox->setFocus();
    QVERIFY(QTest::qWaitForWindowActive(&frame));

    QImage image(frame.width(), frame.height(), QImage::Format_ARGB32);
    frame.render(&image);
    if (image.depth() < 24)
        QSKIP("Test doesn't support color depth < 24");

    QVERIFY2(testForColors(image, QColor(0xe8, 0xff, 0x66)),
            (QString::fromLatin1(spinbox->metaObject()->className())
            + " did not contain background color #e8ff66, using style "
            + QString::fromLatin1(QApplication::style()->metaObject()->className()))
            .toLocal8Bit().constData());
    QVERIFY2(testForColors(image, QColor(0xff, 0x00, 0x84)),
            (QString::fromLatin1(spinbox->metaObject()->className())
            + " did not contain text color #ff0084, using style "
            + QString::fromLatin1(QApplication::style()->metaObject()->className()))
            .toLocal8Bit().constData());
}

class ChangeEventWidget : public QWidget
{
protected:
    void changeEvent(QEvent *event) override
    {
        if(event->type() == QEvent::StyleChange) {
            static bool recurse = false;
            if (!recurse) {
                recurse = true;
                QStyle *style = QStyleFactory::create(QLatin1String("Fusion"));
                style->setParent(this);
                setStyle(style);
                recurse = false;
            }
        }
        QWidget::changeEvent(event);
    }
};

void tst_QStyleSheetStyle::changeStyleInChangeEvent()
{   //must not crash;
    ChangeEventWidget wid;
    wid.ensurePolished();
    wid.setStyleSheet(" /* */ ");
    wid.ensurePolished();
    wid.setStyleSheet(" /* ** */ ");
    wid.ensurePolished();
}

void tst_QStyleSheetStyle::QTBUG11658_cachecrash()
{
    //should not crash
    class Widget : public QWidget
    {
    public:
        Widget(int minimumWidth, QWidget *parent = nullptr)
        : QWidget(parent)
        {
            setMinimumWidth(minimumWidth);
            QVBoxLayout* pLayout = new QVBoxLayout(this);
            QCheckBox* pCheckBox = new QCheckBox(this);
            pLayout->addWidget(pCheckBox);
            setLayout(pLayout);

            QString szStyleSheet = QLatin1String("* { color: red; }");
            qApp->setStyleSheet(szStyleSheet);
            QApplication::setStyle(QStyleFactory::create(QLatin1String("Windows")));
        }
    };

    Widget *w = new Widget(m_testSize.width());
    delete w;
    w = new Widget(m_testSize.width());
    w->setWindowTitle(QTest::currentTestFunction());
    centerOnScreen(w);
    w->show();

    QVERIFY(QTest::qWaitForWindowExposed(w));
    delete w;
    qApp->setStyleSheet(QString());
}

void tst_QStyleSheetStyle::QTBUG15910_crashNullWidget()
{
    struct Widget : QWidget {
        void paintEvent(QPaintEvent *) override
        {
            QStyleOption opt;
            opt.init(this);
            QPainter p(this);
            style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, nullptr);
            style()->drawPrimitive(QStyle::PE_Frame, &opt, &p, nullptr);
            style()->drawControl(QStyle::CE_PushButton, &opt, &p, nullptr);
        }
    } w;
    w.setWindowTitle(QTest::currentTestFunction());
    w.setStyleSheet("* { background-color: white; color:black; border 3px solid yellow }");
    w.setMinimumWidth(160);
    centerOnScreen(&w);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
}

void tst_QStyleSheetStyle::QTBUG36933_brokenPseudoClassLookup()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: This fails. Figure out why.");

    const int rowCount = 10;
    const int columnCount = 10;

    QTableWidget widget(rowCount, columnCount);
    widget.resize(m_testSize);
    widget.setWindowTitle(QTest::currentTestFunction());

    for (int row = 0; row < rowCount; ++row) {
        const QString rowNumber = QLatin1String("row ") + QString::number(row + 1);
        for (int column = 0; column < columnCount; ++column) {
            const QString t = rowNumber
                + QLatin1String(" column ") + QString::number(column + 1);
            widget.setItem(row, column, new QTableWidgetItem(t));
        }

        // put no visible text for the vertical headers, but still put some text or they will collapse
        widget.setVerticalHeaderItem(row, new QTableWidgetItem(QStringLiteral("    ")));
    }

    // parsing of this stylesheet must not crash, and it must be correctly applied
    widget.setStyleSheet(QStringLiteral("QHeaderView::section:vertical { background-color: #FF0000 }"));

    centerOnScreen(&widget);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    widget.activateWindow();
    QApplication::setActiveWindow(&widget);
    QVERIFY(QTest::qWaitForWindowActive(&widget));

    QHeaderView *verticalHeader = widget.verticalHeader();
    QImage image(verticalHeader->size(), QImage::Format_ARGB32);
    verticalHeader->render(&image);
    if (!QApplication::style()->objectName().compare(QLatin1String("fusion"), Qt::CaseInsensitive))
        QEXPECT_FAIL("", "QTBUG-21468", Abort);
    QVERIFY(testForColors(image, QColor(0xFF, 0x00, 0x00)));
}

void tst_QStyleSheetStyle::styleSheetChangeBeforePolish()
{
    QWidget widget;
    widget.setWindowTitle(QTest::currentTestFunction());
    QVBoxLayout *vbox = new QVBoxLayout(&widget);
    QFrame *frame = new QFrame(&widget);
    frame->setFixedSize(m_testSize);
    frame->setStyleSheet("background-color: #FF0000;");
    frame->setStyleSheet("background-color: #00FF00;");
    vbox->addWidget(frame);
    QFrame *frame2 = new QFrame(&widget);
    frame2->setFixedSize(m_testSize);
    frame2->setStyleSheet("background-color: #FF0000;");
    frame2->setStyleSheet("background-color: #00FF00;");
    vbox->addWidget(frame);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QImage image(frame->size(), QImage::Format_ARGB32);
    frame->render(&image);
    QVERIFY(testForColors(image, QColor(0x00, 0xFF, 0x00)));
    QImage image2(frame2->size(), QImage::Format_ARGB32);
    frame2->render(&image2);
    QVERIFY(testForColors(image2, QColor(0x00, 0xFF, 0x00)));
}

void tst_QStyleSheetStyle::widgetStylePropagation_data()
{
    QTest::addColumn<QString>("applicationStyleSheet");
    QTest::addColumn<QString>("parentStyleSheet");
    QTest::addColumn<QString>("childStyleSheet");
    QTest::addColumn<QFont>("parentFont");
    QTest::addColumn<QFont>("childFont");
    QTest::addColumn<QPalette>("parentPalette");
    QTest::addColumn<QPalette>("childPalette");
    QTest::addColumn<int>("parentExpectedSize");
    QTest::addColumn<int>("childExpectedSize");
    QTest::addColumn<QColor>("parentExpectedColor");
    QTest::addColumn<QColor>("childExpectedColor");

    QFont noFont;
    QFont font45; font45.setPointSize(45);
    QFont font32; font32.setPointSize(32);

    QPalette noPalette;
    QPalette redPalette; redPalette.setColor(QPalette::WindowText, QColor("red"));
    QPalette greenPalette; greenPalette.setColor(QPalette::WindowText, QColor("green"));

    QLabel defaultLabel;

    int defaultSize = defaultLabel.font().pointSize();
    QColor defaultColor = defaultLabel.palette().color(defaultLabel.foregroundRole());
    QColor redColor("red");
    QColor greenColor("green");

    // Check regular Qt propagation works as expected, with and without a
    // non-interfering application stylesheet
    QTest::newRow("defaults")
        << QString() << QString() << QString()
        << noFont << noFont << noPalette << noPalette
        << defaultSize << defaultSize << defaultColor << defaultColor;
    QTest::newRow("parent font propagation, no application style sheet")
        << QString() << QString() << QString()
        << font45 << noFont << noPalette << noPalette
        << 45 << 45 << defaultColor << defaultColor;
    QTest::newRow("parent font propagation, dummy application style sheet")
        << "QGroupBox { font-size: 64pt }" << QString() << QString()
        << font45 << noFont << noPalette << noPalette
        << 45 << 45 << defaultColor << defaultColor;
    QTest::newRow("parent color propagation, no application style sheet")
        << QString() << QString() << QString()
        << noFont << noFont << redPalette << noPalette
        << defaultSize << defaultSize << redColor << redColor;
    QTest::newRow("parent color propagation, dummy application style sheet")
        << "QGroupBox { color: blue }" << QString() << QString()
        << noFont << noFont << redPalette << noPalette
        << defaultSize << defaultSize << redColor << redColor;

    // Parent style sheet propagates to child if child has not explicitly
    // set a value
    QTest::newRow("parent style sheet color propagation")
        << "#parentLabel { color: red }" << QString() << QString()
        << noFont << noFont << noPalette << noPalette
        << defaultSize << defaultSize << redColor << redColor;
    QTest::newRow("parent style sheet font propagation")
        << "#parentLabel { font-size: 45pt }" << QString() << QString()
        << noFont << noFont << noPalette << noPalette
        << 45 << 45 << defaultColor << defaultColor;

    // Parent style sheet does not propagate to child if child has explicitly
    // set a value
    QTest::newRow("parent style sheet color propagation, child explicitly set")
        << "#parentLabel { color: red }" << QString() << QString()
        << noFont << noFont << noPalette << greenPalette
        << defaultSize << defaultSize << redColor << greenColor;
    QTest::newRow("parent style sheet font propagation, child explicitly set")
        << "#parentLabel { font-size: 45pt }" << QString() << QString()
        << noFont << font32 << noPalette << noPalette
        << 45 << 32 << defaultColor << defaultColor;

    // Parent does not propagate to child when child is target of style sheet
    QTest::newRow("parent style sheet font propagation, child application style sheet")
        << "#childLabel { font-size: 32pt }" << QString() << QString()
        << font45 << noFont << noPalette << noPalette
        << 45 << 32 << defaultColor << defaultColor;
    QTest::newRow("parent style sheet color propagation, child application style sheet")
        << "#childLabel { color: green }" << QString() << QString()
        << noFont << noFont << redPalette << noPalette
        << defaultSize << defaultSize << redColor << greenColor;
}

void tst_QStyleSheetStyle::widgetStylePropagation()
{
    QFETCH(QString, applicationStyleSheet);
    QFETCH(QString, parentStyleSheet);
    QFETCH(QString, childStyleSheet);

    QFETCH(QFont, parentFont);
    QFETCH(QFont, childFont);
    QFETCH(QPalette, parentPalette);
    QFETCH(QPalette, childPalette);

    QFETCH(int, parentExpectedSize);
    QFETCH(int, childExpectedSize);
    QFETCH(QColor, parentExpectedColor);
    QFETCH(QColor, childExpectedColor);

    QCoreApplication::setAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles, true);

    qApp->setStyleSheet(applicationStyleSheet);

    QLabel parentLabel;
    parentLabel.setObjectName("parentLabel");
    QLabel childLabel(&parentLabel);
    childLabel.setObjectName("childLabel");

    if (parentFont.resolve())
        parentLabel.setFont(parentFont);
    if (childFont.resolve())
        childLabel.setFont(childFont);
    if (parentPalette.resolve())
        parentLabel.setPalette(parentPalette);
    if (childPalette.resolve())
        childLabel.setPalette(childPalette);
    if (!parentStyleSheet.isEmpty())
        parentLabel.setStyleSheet(parentStyleSheet);
    if (!childStyleSheet.isEmpty())
        childLabel.setStyleSheet(childStyleSheet);

    parentLabel.ensurePolished();
    childLabel.ensurePolished();

    QCOMPARE(FONTSIZE(parentLabel), parentExpectedSize);
    QCOMPARE(FONTSIZE(childLabel), childExpectedSize);
    QCOMPARE(COLOR(parentLabel), parentExpectedColor);
    QCOMPARE(COLOR(childLabel), childExpectedColor);
}

void tst_QStyleSheetStyle::styleSheetTargetAttribute()
{
    QGroupBox gb;
    QLabel lb(&gb);
    QPushButton pb(&lb);

    gb.ensurePolished(); lb.ensurePolished(); pb.ensurePolished();
    QCOMPARE(gb.testAttribute(Qt::WA_StyleSheetTarget), false);
    QCOMPARE(lb.testAttribute(Qt::WA_StyleSheetTarget), false);
    QCOMPARE(pb.testAttribute(Qt::WA_StyleSheetTarget), false);

    qApp->setStyleSheet("QPushButton { background-color: blue; }");

    gb.ensurePolished(); lb.ensurePolished(); pb.ensurePolished();
    QCOMPARE(gb.testAttribute(Qt::WA_StyleSheetTarget), false);
    QCOMPARE(lb.testAttribute(Qt::WA_StyleSheetTarget), false);
    QCOMPARE(pb.testAttribute(Qt::WA_StyleSheetTarget), true);

    qApp->setStyleSheet("QGroupBox { background-color: blue; }");

    gb.ensurePolished(); lb.ensurePolished(); pb.ensurePolished();
    QCOMPARE(gb.testAttribute(Qt::WA_StyleSheetTarget), true);
    QCOMPARE(lb.testAttribute(Qt::WA_StyleSheetTarget), false);
    QCOMPARE(pb.testAttribute(Qt::WA_StyleSheetTarget), false);

    qApp->setStyleSheet("QGroupBox * { background-color: blue; }");

    gb.ensurePolished(); lb.ensurePolished(); pb.ensurePolished();
    QCOMPARE(gb.testAttribute(Qt::WA_StyleSheetTarget), false);
    QCOMPARE(lb.testAttribute(Qt::WA_StyleSheetTarget), true);
    QCOMPARE(pb.testAttribute(Qt::WA_StyleSheetTarget), true);

    qApp->setStyleSheet("* { background-color: blue; }");
    gb.ensurePolished(); lb.ensurePolished(); pb.ensurePolished();
    QCOMPARE(gb.testAttribute(Qt::WA_StyleSheetTarget), true);
    QCOMPARE(lb.testAttribute(Qt::WA_StyleSheetTarget), true);
    QCOMPARE(pb.testAttribute(Qt::WA_StyleSheetTarget), true);

    qApp->setStyleSheet("QLabel { font-size: 32pt; }");

    gb.ensurePolished(); lb.ensurePolished(); pb.ensurePolished();
    QCOMPARE(gb.testAttribute(Qt::WA_StyleSheetTarget), false);
    QCOMPARE(lb.testAttribute(Qt::WA_StyleSheetTarget), true);
    QCOMPARE(pb.testAttribute(Qt::WA_StyleSheetTarget), false);

    qApp->setStyleSheet(QString());

    gb.ensurePolished(); lb.ensurePolished(); pb.ensurePolished();
    QCOMPARE(gb.testAttribute(Qt::WA_StyleSheetTarget), false);
    QCOMPARE(lb.testAttribute(Qt::WA_StyleSheetTarget), false);
    QCOMPARE(pb.testAttribute(Qt::WA_StyleSheetTarget), false);
}

void tst_QStyleSheetStyle::unpolish()
{
    QWidget w;
    QCOMPARE(w.minimumWidth(), 0);
    w.setStyleSheet("QWidget { min-width: 100; }");
    w.ensurePolished();
    QCOMPARE(w.minimumWidth(), 100);
    w.setStyleSheet(QString());
    QCOMPARE(w.minimumWidth(), 0);
}

void tst_QStyleSheetStyle::highdpiImages_data()
{
    QTest::addColumn<qreal>("screenFactor");
    QTest::addColumn<QColor>("color");

    QTest::newRow("highdpi") << 2.0 << QColor(0x00, 0xFF, 0x00);
    QTest::newRow("lowdpi")  << 1.0 << QColor(0xFF, 0x00, 0x00);
}

void tst_QStyleSheetStyle::highdpiImages()
{
    QFETCH(qreal, screenFactor);
    QFETCH(QColor, color);

    QWidget w;
    w.setWindowTitle(QLatin1String(QTest::currentTestFunction()) + QLatin1String("::")
                     + QLatin1String(QTest::currentDataTag()));
    QScreen *screen = QGuiApplication::primaryScreen();
    w.move(screen->availableGeometry().topLeft());
    QHighDpiScaling::setScreenFactor(screen, screenFactor);
    w.setStyleSheet("QWidget { background-image: url(\":/images/testimage.png\"); }");
    w.show();

    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QImage image(w.size(), QImage::Format_ARGB32);
    w.render(&image);
    QVERIFY(testForColors(image, color));

    QHighDpiScaling::setScreenFactor(screen, 1.0);
    QHighDpiScaling::updateHighDpiScaling(); // reset to normal
}

QTEST_MAIN(tst_QStyleSheetStyle)
#include "tst_qstylesheetstyle.moc"

