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


#include <QtTest/QtTest>
#include <qlayout.h>
#include "qstyle.h"
#include <qevent.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qapplication.h>
#include <qwidget.h>
#include <qlabel.h>
#include <qstyleoption.h>
#include <qscrollbar.h>
#include <qprogressbar.h>
#include <qtoolbutton.h>
#include <qtoolbar.h>

#include <qcommonstyle.h>
#include <qproxystyle.h>
#include <qstylefactory.h>

#include <qimagereader.h>
#include <qimagewriter.h>
#include <qmenu.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qcombobox.h>
#include <qradiobutton.h>
#include <qlineedit.h>
#include <qmdiarea.h>
#include <qscrollarea.h>
#include <qwidget.h>

#include <algorithm>

#include <QtTest/private/qtesthelpers_p.h>

using namespace QTestPrivate;

class tst_QStyle : public QObject
{
    Q_OBJECT

private slots:
    void drawItemPixmap();
    void cleanup();
#ifndef QT_NO_STYLE_FUSION
    void testFusionStyle();
#endif
    void testWindowsStyle();
#if defined(Q_OS_WIN) && !defined(QT_NO_STYLE_WINDOWSVISTA) && !defined(Q_OS_WINRT)
    void testWindowsVistaStyle();
#endif
#ifdef Q_OS_MAC
    void testMacStyle();
#endif
    void testStyleFactory();
    void testProxyStyle();
    void pixelMetric();
#if !defined(QT_NO_STYLE_WINDOWS) && !defined(QT_NO_STYLE_FUSION)
    void progressBarChangeStyle();
#endif
    void defaultFont();
    void testDrawingShortcuts();
    void testFrameOnlyAroundContents();

    void testProxyCalled();
    void testStyleOptionInit();
private:
    bool testAllFunctions(QStyle *);
    bool testScrollBarSubControls(const QStyle *style);
    void testPainting(QStyle *style, const QString &platform);
    void lineUpLayoutTest(QStyle *);
};

class MyWidget : public QWidget
{
public:
    using QWidget::QWidget;

protected:
    void paintEvent(QPaintEvent *) override;
};

void tst_QStyle::cleanup()
{
    QVERIFY(QApplication::topLevelWidgets().isEmpty());
}

void tst_QStyle::testStyleFactory()
{
    const QStringList keys = QStyleFactory::keys();
#ifndef QT_NO_STYLE_FUSION
    QVERIFY(keys.contains("Fusion"));
#endif
#ifndef QT_NO_STYLE_WINDOWS
    QVERIFY(keys.contains("Windows"));
#endif

    for (const QString &styleName : keys) {
        QScopedPointer<QStyle> style(QStyleFactory::create(styleName));
        QVERIFY2(!style.isNull(),
                 qPrintable(QString::fromLatin1("Fail to load style '%1'").arg(styleName)));
    }
}

class CustomProxy : public QProxyStyle
{
    int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr,
                            const QWidget *widget = nullptr) const override
    {
        if (metric == QStyle::PM_ButtonIconSize)
            return 13;
        return QProxyStyle::pixelMetric(metric, option, widget);
    }
};

void tst_QStyle::testProxyStyle()
{
    QProxyStyle *proxyStyle = new QProxyStyle();
    QVERIFY(proxyStyle->baseStyle());
    QStyle *style = QStyleFactory::create("Windows");
    QCOMPARE(style->proxy(), style);

    proxyStyle->setBaseStyle(style);
    QCOMPARE(style->proxy(), proxyStyle);
    QCOMPARE(style->parent(), proxyStyle);
    QCOMPARE(proxyStyle->baseStyle(), style);

    QVERIFY(testAllFunctions(proxyStyle));
    proxyStyle->setBaseStyle(nullptr);
    QVERIFY(proxyStyle->baseStyle());
    QApplication::setStyle(proxyStyle);

    QProxyStyle* baseStyle = new QProxyStyle("Windows");
    QCOMPARE(baseStyle->baseStyle()->objectName(), style->objectName());

    QProxyStyle doubleProxy(baseStyle);
    QVERIFY(testAllFunctions(&doubleProxy));

    CustomProxy customStyle;
    QLineEdit edit;
    edit.setStyle(&customStyle);
    QVERIFY(!customStyle.parent());
    QCOMPARE(edit.style()->pixelMetric(QStyle::PM_ButtonIconSize), 13);
}

void tst_QStyle::drawItemPixmap()
{
    MyWidget testWidget;
    testWidget.setObjectName("testObject");
    testWidget.resize(300, 300);
    testWidget.showNormal();

    QImage image = testWidget.grab().toImage();
    const QRgb green = QColor(Qt::green).rgb();
    QVERIFY(image.reinterpretAsFormat(QImage::Format_RGB32));
    const QRgb *bits = reinterpret_cast<const QRgb *>(image.constBits());
    const QRgb *end = bits + image.sizeInBytes() / sizeof(QRgb);
#ifdef Q_OS_WINRT
    QEXPECT_FAIL("", "QWidget::resize does not work on WinRT", Continue);
#endif
    QVERIFY(std::all_of(bits, end, [green] (QRgb r) { return r == green; }));
}

bool tst_QStyle::testAllFunctions(QStyle *style)
{
    QStyleOption opt;
    QWidget testWidget;
    opt.init(&testWidget);

    testWidget.setStyle(style);

    //Tests styleHint with default arguments for potential crashes
    for ( int hint = 0 ; hint < int(QStyle::SH_Menu_Mask); ++hint) {
        style->styleHint(QStyle::StyleHint(hint));
        style->styleHint(QStyle::StyleHint(hint), &opt, &testWidget);
    }

    //Tests pixelMetric with default arguments for potential crashes
    for ( int pm = 0 ; pm < int(QStyle::PM_LayoutVerticalSpacing); ++pm) {
        style->pixelMetric(QStyle::PixelMetric(pm));
        style->pixelMetric(QStyle::PixelMetric(pm), &opt, &testWidget);
    }

    //Tests drawControl with default arguments for potential crashes
    for ( int control = 0 ; control < int(QStyle::CE_ColumnViewGrip); ++control) {
        QPixmap surface(QSize(200, 200));
        QPainter painter(&surface);
        style->drawControl(QStyle::ControlElement(control), &opt, &painter, nullptr);
    }

    //Tests drawComplexControl with default arguments for potential crashes
    {
        QPixmap surface(QSize(200, 200));
        QPainter painter(&surface);
        QStyleOptionComboBox copt1;
        copt1.init(&testWidget);

        QStyleOptionGroupBox copt2;
        copt2.init(&testWidget);
        QStyleOptionSizeGrip copt3;
        copt3.init(&testWidget);
        QStyleOptionSlider copt4;
        copt4.init(&testWidget);
        copt4.minimum = 0;
        copt4.maximum = 100;
        copt4.tickInterval = 25;
        copt4.sliderValue = 50;
        QStyleOptionSpinBox copt5;
        copt5.init(&testWidget);
        QStyleOptionTitleBar copt6;
        copt6.init(&testWidget);
        QStyleOptionToolButton copt7;
        copt7.init(&testWidget);
        QStyleOptionComplex copt9;
        copt9.initFrom(&testWidget);

        style->drawComplexControl(QStyle::CC_SpinBox, &copt5, &painter, nullptr);
        style->drawComplexControl(QStyle::CC_ComboBox, &copt1, &painter, nullptr);
        style->drawComplexControl(QStyle::CC_ScrollBar, &copt4, &painter, nullptr);
        style->drawComplexControl(QStyle::CC_Slider, &copt4, &painter, nullptr);
        style->drawComplexControl(QStyle::CC_ToolButton, &copt7, &painter, nullptr);
        style->drawComplexControl(QStyle::CC_TitleBar, &copt6, &painter, nullptr);
        style->drawComplexControl(QStyle::CC_GroupBox, &copt2, &painter, nullptr);
        style->drawComplexControl(QStyle::CC_Dial, &copt4, &painter, nullptr);
    }

    //Check standard pixmaps/icons
    for ( int i = 0 ; i < int(QStyle::SP_ToolBarVerticalExtensionButton); ++i) {
        QPixmap pixmap = style->standardPixmap(QStyle::StandardPixmap(i));
        if (pixmap.isNull()) {
            qWarning("missing StandardPixmap: %d", i);
        }
        QIcon icn = style->standardIcon(QStyle::StandardPixmap(i));
        if (icn.isNull()) {
            qWarning("missing StandardIcon: %d", i);
        }
    }

    style->itemPixmapRect(QRect(0, 0, 100, 100), Qt::AlignHCenter, QPixmap(200, 200));
    style->itemTextRect(QFontMetrics(QApplication::font()), QRect(0, 0, 100, 100),
                        Qt::AlignHCenter, true, QLatin1String("Test"));

    return testScrollBarSubControls(style);
}

bool tst_QStyle::testScrollBarSubControls(const QStyle *style)
{
    const bool isMacStyle = style->objectName().compare(QLatin1String("macintosh"),
                                                        Qt::CaseInsensitive) == 0;
    QScrollBar scrollBar;
    setFrameless(&scrollBar);
    scrollBar.show();
    const QStyleOptionSlider opt = qt_qscrollbarStyleOption(&scrollBar);
    for (int sc : {1, 2, 4, 8}) {
        const auto subControl = static_cast<QStyle::SubControl>(sc);
        const QRect sr = style->subControlRect(QStyle::CC_ScrollBar, &opt, subControl, &scrollBar);
        if (sr.isNull()) {
            // macOS scrollbars no longer have these, so there's no reason to fail
            if (!(isMacStyle && (subControl == QStyle::SC_ScrollBarAddLine ||
                                 subControl == QStyle::SC_ScrollBarSubLine))) {
                qWarning() << "Unexpected null rect for subcontrol" << subControl;
                return false;
            }
        }
    }
    return true;
}

#ifndef QT_NO_STYLE_FUSION
void tst_QStyle::testFusionStyle()
{
    QScopedPointer<QStyle> fstyle(QStyleFactory::create("Fusion"));
    QVERIFY(!fstyle.isNull());
    QVERIFY(testAllFunctions(fstyle.data()));
    lineUpLayoutTest(fstyle.data());
}
#endif

void tst_QStyle::testWindowsStyle()
{
    QScopedPointer<QStyle> wstyle(QStyleFactory::create("Windows"));
    QVERIFY(!wstyle.isNull());
    QVERIFY(testAllFunctions(wstyle.data()));
    lineUpLayoutTest(wstyle.data());

    // Tests drawing indeterminate progress with 0 size: QTBUG-15973
    QStyleOptionProgressBar pb;
    pb.rect = QRect(0,0,-9,0);
    QPixmap surface(QSize(200, 200));
    QPainter painter(&surface);
    wstyle->drawControl(QStyle::CE_ProgressBar, &pb, &painter, nullptr);
}

#if defined(Q_OS_WIN) && !defined(QT_NO_STYLE_WINDOWSVISTA) && !defined(Q_OS_WINRT)
void tst_QStyle::testWindowsVistaStyle()
{
    QScopedPointer<QStyle> vistastyle(QStyleFactory::create("WindowsVista"));
    QVERIFY(!vistastyle.isNull());
    QVERIFY(testAllFunctions(vistastyle.data()));
}
#endif

#ifdef Q_OS_MAC
void tst_QStyle::testMacStyle()
{
    QStyle *mstyle = QStyleFactory::create("Macintosh");
    QVERIFY(testAllFunctions(mstyle));
    delete mstyle;
}
#endif

// Helper class...
void MyWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    QPixmap big(400,400);
    big.fill(Qt::green);
    style()->drawItemPixmap(&p, rect(), Qt::AlignCenter, big);
}


class Qt42Style : public QCommonStyle
{
    Q_OBJECT
public:
    int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr,
                    const QWidget *widget = nullptr) const override;

    int margin_toplevel = 10;
    int margin = 5;
    int spacing = 0;
};

int Qt42Style::pixelMetric(PixelMetric metric, const QStyleOption * /* option = 0*/,
                                   const QWidget * /* widget = 0*/ ) const
{
    switch (metric) {
        case QStyle::PM_DefaultTopLevelMargin:
            return margin_toplevel;
        case QStyle::PM_DefaultChildMargin:
            return margin;
        case QStyle::PM_DefaultLayoutSpacing:
            return spacing;
        default:
            break;
    }
    return -1;
}


void tst_QStyle::pixelMetric()
{
    QScopedPointer<Qt42Style> style(new Qt42Style);
    QCOMPARE(style->pixelMetric(QStyle::PM_DefaultTopLevelMargin), 10);
    QCOMPARE(style->pixelMetric(QStyle::PM_DefaultChildMargin), 5);
    QCOMPARE(style->pixelMetric(QStyle::PM_DefaultLayoutSpacing), 0);

    style->margin_toplevel = 0;
    style->margin = 0;
    style->spacing = 0;
    QCOMPARE(style->pixelMetric(QStyle::PM_DefaultTopLevelMargin), 0);
    QCOMPARE(style->pixelMetric(QStyle::PM_DefaultChildMargin), 0);
    QCOMPARE(style->pixelMetric(QStyle::PM_DefaultLayoutSpacing), 0);

    style->margin_toplevel = -1;
    style->margin = -1;
    style->spacing = -1;
    QCOMPARE(style->pixelMetric(QStyle::PM_DefaultTopLevelMargin), -1);
    QCOMPARE(style->pixelMetric(QStyle::PM_DefaultChildMargin), -1);
    QCOMPARE(style->pixelMetric(QStyle::PM_DefaultLayoutSpacing), -1);
}

#if !defined(QT_NO_STYLE_WINDOWS) && !defined(QT_NO_STYLE_FUSION)
void tst_QStyle::progressBarChangeStyle()
{
    //test a crashing situation (task 143530)
    //where changing the styles and deleting a progressbar would crash

    QStyle *style1 = QStyleFactory::create("Windows");
    QStyle *style2 = QStyleFactory::create("Fusion");

    QProgressBar *progress=new QProgressBar;
    progress->setStyle(style1);

    progress->show();

    progress->setStyle(style2);

    QTest::qWait(100);
    delete progress;

    QTest::qWait(100);

    //before the correction, there would be a crash here
    delete style1;
    delete style2;
}
#endif

void tst_QStyle::lineUpLayoutTest(QStyle *style)
{
    QWidget widget;
    setFrameless(&widget);
    QHBoxLayout layout;
    QFont font;
    font.setPointSize(9); //Plastique is lined up for odd numbers...
    widget.setFont(font);
    QSpinBox spinbox(&widget);
    QLineEdit lineedit(&widget);
    QComboBox combo(&widget);
    combo.setEditable(true);
    layout.addWidget(&spinbox);
    layout.addWidget(&lineedit);
    layout.addWidget(&combo);
    widget.setLayout(&layout);
    widget.setStyle(style);
    // propagate the style.
    const auto children = widget.findChildren<QWidget *>();
    for (QWidget *w : children)
        w->setStyle(style);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

#ifdef Q_OS_WIN
    const int limit = 2; // Aero style has larger margins
#else
    const int limit = 1;
#endif
    const int slDiff = qAbs(spinbox.height() - lineedit.height());
    const int scDiff = qAbs(spinbox.height() - combo.height());
    QVERIFY2(slDiff <= limit,
             qPrintable(QString::fromLatin1("%1 exceeds %2 for %3")
                        .arg(slDiff).arg(limit).arg(style->objectName())));
    QVERIFY2(scDiff <= limit,
             qPrintable(QString::fromLatin1("%1 exceeds %2 for %3")
                        .arg(scDiff).arg(limit).arg(style->objectName())));
}

void tst_QStyle::defaultFont()
{
    QFont defaultFont = QApplication::font();
    QFont pointFont = defaultFont;
    pointFont.setPixelSize(9);
    QApplication::setFont(pointFont);
    QPushButton button;
    setFrameless(&button);
    button.show();
    QCoreApplication::processEvents();
    QApplication::setFont(defaultFont);
}

class DrawTextStyle : public QProxyStyle
{
    Q_OBJECT
public:
    using QProxyStyle::QProxyStyle;

    void drawItemText(QPainter *painter, const QRect &rect,
                      int flags, const QPalette &pal, bool enabled,
                      const QString &text, QPalette::ColorRole textRole = QPalette::NoRole) const override
    {
        alignment = flags;
        QProxyStyle::drawItemText(painter, rect, flags, pal, enabled, text, textRole);
    }

    mutable int alignment = 0;
};


void tst_QStyle::testDrawingShortcuts()
{
    {
        QWidget w;
        setFrameless(&w);
        QToolButton *tb = new QToolButton(&w);
        tb->setText("&abc");
        QScopedPointer<DrawTextStyle> dts(new DrawTextStyle);
        w.show();
        tb->setStyle(dts.data());
        tb->grab();
        QStyleOptionToolButton sotb;
        sotb.initFrom(tb);
        bool showMnemonic = dts->styleHint(QStyle::SH_UnderlineShortcut, &sotb, tb);
        QVERIFY(dts->alignment & (showMnemonic ? Qt::TextShowMnemonic : Qt::TextHideMnemonic));
    }
    {
        QToolBar w;
        setFrameless(&w);
        QToolButton *tb = new QToolButton(&w);
        tb->setText("&abc");
        QScopedPointer<DrawTextStyle> dts(new DrawTextStyle);
        w.addWidget(tb);
        w.show();
        tb->setStyle(dts.data());
        tb->grab();
        QStyleOptionToolButton sotb;
        sotb.initFrom(tb);
        bool showMnemonic = dts->styleHint(QStyle::SH_UnderlineShortcut, &sotb, tb);
        QVERIFY(dts->alignment & (showMnemonic ? Qt::TextShowMnemonic : Qt::TextHideMnemonic));
     }
}

static const int SCROLLBAR_SPACING = 33;

class FrameTestStyle : public QProxyStyle {
public:
    FrameTestStyle() : QProxyStyle("Windows") { }

    int styleHint(StyleHint hint, const QStyleOption *opt, const QWidget *widget,
                  QStyleHintReturn *returnData) const override
    {
        if (hint == QStyle::SH_ScrollView_FrameOnlyAroundContents)
            return 1;
        return QProxyStyle ::styleHint(hint, opt, widget, returnData);
    }

    int pixelMetric(PixelMetric pm, const QStyleOption *option, const QWidget *widget) const override
    {
        if (pm == QStyle::PM_ScrollView_ScrollBarSpacing)
            return SCROLLBAR_SPACING;
        return QProxyStyle ::pixelMetric(pm, option ,widget);
    }
};

void tst_QStyle::testFrameOnlyAroundContents()
{
    QScrollArea area;
    area.setGeometry(0, 0, 200, 200);
    QScopedPointer<QStyle> winStyle(QStyleFactory::create("Windows"));
    FrameTestStyle frameStyle;
    QWidget *widget = new QWidget(&area);
    widget->setGeometry(0, 0, 400, 400);
    area.setStyle(winStyle.data());
    area.verticalScrollBar()->setStyle(winStyle.data());
    area.setWidget(widget);
    area.setVisible(true);
    int viewPortWidth = area.viewport()->width();
    area.verticalScrollBar()->setStyle(&frameStyle);
    area.setStyle(&frameStyle);
    // Test that we reserve space for scrollbar spacing
#ifdef Q_OS_WINRT
    QEXPECT_FAIL("", "QWidget::setGeometry does not work on WinRT", Continue);
#endif
    QCOMPARE(viewPortWidth, area.viewport()->width() + SCROLLBAR_SPACING);
}


class ProxyTest: public QProxyStyle
{
    Q_OBJECT
public:
    using QProxyStyle::QProxyStyle;

    void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p,
                       const QWidget *w) const override
    {
        called = true;
        return QProxyStyle::drawPrimitive(pe, opt, p, w);
    }

    mutable bool called = false;
};


void tst_QStyle::testProxyCalled()
{
    QToolButton b;
    b.setArrowType(Qt::DownArrow);
    QStyleOptionToolButton opt;
    opt.init(&b);
    opt.features |= QStyleOptionToolButton::Arrow;
    QPixmap surface(QSize(200, 200));
    QPainter painter(&surface);

    const QStringList keys = QStyleFactory::keys();
    QVector<QStyle*> styles;
    styles.reserve(keys.size() + 1);

    styles << new QCommonStyle();

    for (const QString &key : keys)
        styles << QStyleFactory::create(key);

    for (QStyle *style : styles) {
        ProxyTest testStyle;
        testStyle.setBaseStyle(style);
        style->drawControl(QStyle::CE_ToolButtonLabel, &opt, &painter, &b);
        QVERIFY(testStyle.called);
        delete style;
    }
}


class TestStyleOptionInitProxy: public QProxyStyle
{
    Q_OBJECT
public:
    mutable bool invalidOptionsDetected = false;

    using QProxyStyle::QProxyStyle;

    void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p, const QWidget *w) const override {
        checkStyleEnum<QStyle::PrimitiveElement>(pe, opt);
        return QProxyStyle::drawPrimitive(pe, opt, p, w);
    }

    void drawControl(ControlElement element, const QStyleOption *opt, QPainter *p, const QWidget *w) const override {
        checkStyleEnum<QStyle::ControlElement>(element, opt);
        return QProxyStyle::drawControl(element, opt, p, w);
    }

    QRect subElementRect(SubElement subElement, const QStyleOption *option, const QWidget *widget) const override {
        checkStyleEnum<QStyle::SubElement>(subElement, option);
        return QProxyStyle::subElementRect(subElement, option, widget);
    }

    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p, const QWidget *widget) const override {
        checkStyleEnum<QStyle::ComplexControl>(cc, opt);
        return QProxyStyle::drawComplexControl(cc, opt, p, widget);
    }

    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc, const QWidget *widget) const override {
        checkStyleEnum<QStyle::ComplexControl>(cc, opt);
        return QProxyStyle::subControlRect(cc, opt, sc, widget);
    }

    int pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const override {
        checkStyleEnum<QStyle::PixelMetric>(metric, option);
        return QProxyStyle::pixelMetric(metric, option, widget);
    }

    QSize sizeFromContents(ContentsType ct, const QStyleOption *opt, const QSize &contentsSize, const QWidget *w) const override {
        checkStyleEnum<QStyle::ContentsType>(ct, opt);
        return QProxyStyle::sizeFromContents(ct, opt, contentsSize, w);
    }

    int styleHint(StyleHint stylehint, const QStyleOption *opt, const QWidget *widget, QStyleHintReturn *returnData) const override {
        checkStyleEnum<QStyle::StyleHint>(stylehint, opt);
        return QProxyStyle::styleHint(stylehint, opt, widget, returnData);
    }

    QPixmap standardPixmap(StandardPixmap standardPixmap, const QStyleOption *opt, const QWidget *widget) const  override {
        checkStyleEnum<QStyle::StandardPixmap>(standardPixmap, opt);
        return QProxyStyle::standardPixmap(standardPixmap, opt, widget);
    }

    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *option, const QWidget *widget) const override {
        checkStyleEnum<QStyle::StandardPixmap>(standardIcon, option);
        return QProxyStyle::standardIcon(standardIcon, option, widget);
    }

    QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap, const QStyleOption *opt) const  override {
        checkStyle(QString::asprintf("QIcon::Mode(%i)", iconMode).toLatin1(), opt);
        return QProxyStyle::generatedIconPixmap(iconMode, pixmap, opt);
    }

    int layoutSpacing(QSizePolicy::ControlType control1, QSizePolicy::ControlType control2, Qt::Orientation orientation, const QStyleOption *option, const QWidget *widget) const override {
        checkStyle(QString::asprintf("QSizePolicy::ControlType(%i), QSizePolicy::ControlType(%i)", control1, control2).toLatin1(), option);
        return QProxyStyle::layoutSpacing(control1, control2, orientation, option, widget);
    }

private:
    void checkStyle(const QByteArray &info, const QStyleOption *opt) const {
        if (opt && (opt->version == 0 || opt->styleObject == nullptr) ) {
            invalidOptionsDetected = true;
            qWarning() << baseStyle()->metaObject()->className()
                       << "Invalid QStyleOption found for"
                       << info;
            qWarning() << "Version:" << opt->version << "StyleObject:" << opt->styleObject;
        }
    }

    template<typename MEnum>
    void checkStyleEnum(MEnum element, const QStyleOption *opt) const {
        static QMetaEnum _enum = QMetaEnum::fromType<MEnum>();
        checkStyle(_enum.valueToKey(element), opt);
    }
};

void tst_QStyle::testStyleOptionInit()
{
    QStringList keys = QStyleFactory::keys();
    keys.prepend(QString()); // QCommonStyle marker

    for (const QString &key : qAsConst(keys)) {
        QStyle* style = key.isEmpty() ? new QCommonStyle : QStyleFactory::create(key);
        TestStyleOptionInitProxy testStyle;
        testStyle.setBaseStyle(style);
        testAllFunctions(style);
        QVERIFY(!testStyle.invalidOptionsDetected);
    }
}

QTEST_MAIN(tst_QStyle)
#include "tst_qstyle.moc"
