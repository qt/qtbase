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
public:
    tst_QStyle();

private:
    bool testAllFunctions(QStyle *);
    bool testScrollBarSubControls();
    void testPainting(QStyle *style, const QString &platform);
private slots:
    void drawItemPixmap();
    void init();
    void cleanup();
#ifndef QT_NO_STYLE_FUSION
    void testFusionStyle();
#endif
    void testWindowsStyle();
#if defined(Q_OS_WIN) && !defined(QT_NO_STYLE_WINDOWSVISTA)
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
    void lineUpLayoutTest(QStyle *);
    QWidget *testWidget;
};


tst_QStyle::tst_QStyle()
{
    testWidget = 0;
}

class MyWidget : public QWidget
{
public:
    MyWidget( QWidget* QWidget=0, const char* name=0 );
protected:
    void paintEvent( QPaintEvent* );
};

void tst_QStyle::init()
{
    testWidget = new MyWidget( 0, "testObject");
}

void tst_QStyle::cleanup()
{
    delete testWidget;
    testWidget = 0;
}

void tst_QStyle::testStyleFactory()
{
    QStringList keys = QStyleFactory::keys();
#ifndef QT_NO_STYLE_FUSION
    QVERIFY(keys.contains("Fusion"));
#endif
#ifndef QT_NO_STYLE_WINDOWS
    QVERIFY(keys.contains("Windows"));
#endif

    foreach (QString styleName , keys) {
        QStyle *style = QStyleFactory::create(styleName);
        QVERIFY2(style != 0, qPrintable(QString::fromLatin1("Fail to load style '%1'").arg(styleName)));
        delete style;
    }
}

class CustomProxy : public QProxyStyle
{
    virtual int pixelMetric(PixelMetric metric, const QStyleOption *option = 0,
                            const QWidget *widget = 0) const
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
    proxyStyle->setBaseStyle(0);
    QVERIFY(proxyStyle->baseStyle());
    qApp->setStyle(proxyStyle);

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
    testWidget->resize(300, 300);
    testWidget->showNormal();

    QImage image = testWidget->grab().toImage();
    const QRgb green = QColor(Qt::green).rgb();
    QVERIFY(image.reinterpretAsFormat(QImage::Format_RGB32));
    const QRgb *bits = reinterpret_cast<const QRgb *>(image.constBits());
    const QRgb *end = bits + image.byteCount() / sizeof(QRgb);
    QVERIFY(std::all_of(bits, end, [green] (QRgb r) { return r == green; }));
    testWidget->hide();
}

bool tst_QStyle::testAllFunctions(QStyle *style)
{
    QStyleOption opt;
    opt.init(testWidget);

    testWidget->setStyle(style);

    //Tests styleHint with default arguments for potential crashes
    for ( int hint = 0 ; hint < int(QStyle::SH_Menu_Mask); ++hint) {
        style->styleHint(QStyle::StyleHint(hint));
        style->styleHint(QStyle::StyleHint(hint), &opt, testWidget);
    }

    //Tests pixelMetric with default arguments for potential crashes
    for ( int pm = 0 ; pm < int(QStyle::PM_LayoutVerticalSpacing); ++pm) {
        style->pixelMetric(QStyle::PixelMetric(pm));
        style->pixelMetric(QStyle::PixelMetric(pm), &opt, testWidget);
    }

    //Tests drawControl with default arguments for potential crashes
    for ( int control = 0 ; control < int(QStyle::CE_ColumnViewGrip); ++control) {
        QPixmap surface(QSize(200, 200));
        QPainter painter(&surface);
        style->drawControl(QStyle::ControlElement(control), &opt, &painter, 0);
    }

    //Tests drawComplexControl with default arguments for potential crashes
    {
        QPixmap surface(QSize(200, 200));
        QPainter painter(&surface);
        QStyleOptionComboBox copt1;
        copt1.init(testWidget);

        QStyleOptionGroupBox copt2;
        copt2.init(testWidget);
        QStyleOptionSizeGrip copt3;
        copt3.init(testWidget);
        QStyleOptionSlider copt4;
        copt4.init(testWidget);
        copt4.minimum = 0;
        copt4.maximum = 100;
        copt4.tickInterval = 25;
        copt4.sliderValue = 50;
        QStyleOptionSpinBox copt5;
        copt5.init(testWidget);
        QStyleOptionTitleBar copt6;
        copt6.init(testWidget);
        QStyleOptionToolButton copt7;
        copt7.init(testWidget);
        QStyleOptionComplex copt9;
        copt9.initFrom(testWidget);

        style->drawComplexControl(QStyle::CC_SpinBox, &copt5, &painter, 0);
        style->drawComplexControl(QStyle::CC_ComboBox, &copt1, &painter, 0);
        style->drawComplexControl(QStyle::CC_ScrollBar, &copt4, &painter, 0);
        style->drawComplexControl(QStyle::CC_Slider, &copt4, &painter, 0);
        style->drawComplexControl(QStyle::CC_ToolButton, &copt7, &painter, 0);
        style->drawComplexControl(QStyle::CC_TitleBar, &copt6, &painter, 0);
        style->drawComplexControl(QStyle::CC_GroupBox, &copt2, &painter, 0);
        style->drawComplexControl(QStyle::CC_Dial, &copt4, &painter, 0);
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
    style->itemTextRect(QFontMetrics(qApp->font()), QRect(0, 0, 100, 100), Qt::AlignHCenter, true, QString("Test"));

    return testScrollBarSubControls();
}

bool tst_QStyle::testScrollBarSubControls()
{
    const auto *style = testWidget->style();
    const bool isMacStyle = style->objectName().toLower() == "macintosh";
    QScrollBar scrollBar;
    setFrameless(&scrollBar);
    scrollBar.show();
    const QStyleOptionSlider opt = qt_qscrollbarStyleOption(&scrollBar);
    foreach (int sc, QList<int>() << 1 << 2 << 4 << 8) {
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
    QStyle *fstyle = QStyleFactory::create("Fusion");
    QVERIFY(testAllFunctions(fstyle));
    lineUpLayoutTest(fstyle);
    delete fstyle;
}
#endif

void tst_QStyle::testWindowsStyle()
{
    QStyle *wstyle = QStyleFactory::create("Windows");
    QVERIFY(testAllFunctions(wstyle));
    lineUpLayoutTest(wstyle);

    // Tests drawing indeterminate progress with 0 size: QTBUG-15973
    QStyleOptionProgressBar pb;
    pb.rect = QRect(0,0,-9,0);
    QPixmap surface(QSize(200, 200));
    QPainter painter(&surface);
    wstyle->drawControl(QStyle::CE_ProgressBar, &pb, &painter, 0);
    delete wstyle;
}

void writeImage(const QString &fileName, QImage image)
{
    QImageWriter imageWriter(fileName);
    imageWriter.setFormat("png");
    qDebug() << "result " << imageWriter.write(image);
}

QImage readImage(const QString &fileName)
{
    QImageReader reader(fileName);
    return reader.read();
}


#if defined(Q_OS_WIN) && !defined(QT_NO_STYLE_WINDOWSVISTA)
void tst_QStyle::testWindowsVistaStyle()
{
    QStyle *vistastyle = QStyleFactory::create("WindowsVista");
    QVERIFY(testAllFunctions(vistastyle));

    if (QSysInfo::WindowsVersion == QSysInfo::WV_VISTA)
        testPainting(vistastyle, "vista");
    delete vistastyle;
}
#endif

void comparePixmap(const QString &filename, const QPixmap &pixmap)
{
    QImage oldFile = readImage(filename);
    QPixmap oldPixmap = QPixmap::fromImage(oldFile);
    if (!oldFile.isNull())
        QCOMPARE(pixmap, oldPixmap);
    else
        writeImage(filename, pixmap.toImage());
}

void tst_QStyle::testPainting(QStyle *style, const QString &platform)
{
qDebug("TEST PAINTING");
    //Test Menu
    QString fileName = "images/" + platform + "/menu.png";
    QMenu menu;
    menu.setStyle(style);
    menu.show();
    menu.addAction(new QAction("Test 1", &menu));
    menu.addAction(new QAction("Test 2", &menu));
    QPixmap pixmap = menu.grab();
    comparePixmap(fileName, pixmap);

    //Push button
    fileName = "images/" + platform + "/button.png";
    QPushButton button("OK");
    button.setStyle(style);
    button.show();
    pixmap = button.grab();
    button.hide();
    comparePixmap(fileName, pixmap);

    //Push button
    fileName = "images/" + platform + "/radiobutton.png";
    QRadioButton radiobutton("Check");
    radiobutton.setStyle(style);
    radiobutton.show();
    pixmap = radiobutton.grab();
    radiobutton.hide();
    comparePixmap(fileName, pixmap);

    //Combo box
    fileName = "images/" + platform + "/combobox.png";
    QComboBox combobox;
    combobox.setStyle(style);
    combobox.addItem("Test 1");
    combobox.addItem("Test 2");
    combobox.show();
    pixmap = combobox.grab();
    combobox.hide();
    comparePixmap(fileName, pixmap);

    //Spin box
    fileName = "images/" + platform + "/spinbox.png";
    QDoubleSpinBox spinbox;
    spinbox.setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
    spinbox.setStyle(style);
    spinbox.show();
    pixmap = spinbox.grab();
    spinbox.hide();
    comparePixmap(fileName, pixmap);
    QLocale::setDefault(QLocale::system());

    //Slider
    fileName = "images/" + platform + "/slider.png";
    QSlider slider;
    slider.setStyle(style);
    slider.show();
    pixmap = slider.grab();
    slider.hide();
    comparePixmap(fileName, pixmap);

    //Line edit
    fileName = "images/" + platform + "/lineedit.png";
    QLineEdit lineedit("Test text");
    lineedit.setStyle(style);
    lineedit.show();
    pixmap = lineedit.grab();
    lineedit.hide();
    comparePixmap(fileName, pixmap);

    //MDI
    fileName = "images/" + platform + "/mdi.png";
    QMdiArea mdiArea;
    mdiArea.addSubWindow(new QWidget(&mdiArea));
    mdiArea.resize(200, 200);
    mdiArea.setStyle(style);
    mdiArea.show();
    pixmap = mdiArea.grab();
    mdiArea.hide();
    comparePixmap(fileName, pixmap);

    // QToolButton
    fileName = "images/" + platform + "/toolbutton.png";
    QToolButton tb;
    tb.setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    tb.setText("AaQqPpXx");
    tb.setIcon(style->standardPixmap(QStyle::SP_DirHomeIcon));
    tb.setStyle(style);
    tb.show();
    pixmap = tb.grab();
    tb.hide();
    comparePixmap(fileName, pixmap);

}

#ifdef Q_OS_MAC
void tst_QStyle::testMacStyle()
{
    QStyle *mstyle = QStyleFactory::create("Macintosh");
    QVERIFY(testAllFunctions(mstyle));
    delete mstyle;
}
#endif

// Helper class...

MyWidget::MyWidget( QWidget* parent, const char* name )
    : QWidget( parent )
{
    setObjectName(name);
}

void MyWidget::paintEvent( QPaintEvent* )
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
    Qt42Style() : QCommonStyle()
    {
        margin_toplevel = 10;
        margin = 5;
        spacing = 0;
    }

    virtual int pixelMetric(PixelMetric metric, const QStyleOption * option = 0,
                            const QWidget * widget = 0 ) const;

    int margin_toplevel;
    int margin;
    int spacing;

};

int Qt42Style::pixelMetric(PixelMetric metric, const QStyleOption * /* option = 0*/,
                                   const QWidget * /* widget = 0*/ ) const
{
    switch (metric) {
        case QStyle::PM_DefaultTopLevelMargin:
            return margin_toplevel;
        break;
        case QStyle::PM_DefaultChildMargin:
            return margin;
        break;
        case QStyle::PM_DefaultLayoutSpacing:
            return spacing;
        break;
        default:
            break;
    }
    return -1;
}


void tst_QStyle::pixelMetric()
{
    Qt42Style *style = new Qt42Style();
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

    delete style;
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
        foreach (QWidget *w, widget.findChildren<QWidget *>())
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
    QFont defaultFont = qApp->font();
    QFont pointFont = defaultFont;
    pointFont.setPixelSize(9);
    qApp->setFont(pointFont);
    QPushButton button;
    setFrameless(&button);
    button.show();
    qApp->processEvents();
    qApp->setFont(defaultFont);
}

class DrawTextStyle : public QProxyStyle
{
    Q_OBJECT
public:
    DrawTextStyle(QStyle *base = 0) : QProxyStyle(), alignment(0) { setBaseStyle(base); }
    void drawItemText(QPainter *painter, const QRect &rect,
                              int flags, const QPalette &pal, bool enabled,
                              const QString &text, QPalette::ColorRole textRole = QPalette::NoRole) const
    {
        DrawTextStyle *that = (DrawTextStyle *)this;
        that->alignment = flags;
        QProxyStyle::drawItemText(painter, rect, flags, pal, enabled, text, textRole);
    }
    int alignment;
};


void tst_QStyle::testDrawingShortcuts()
{
    {
        QWidget w;
        setFrameless(&w);
        QToolButton *tb = new QToolButton(&w);
        tb->setText("&abc");
        DrawTextStyle *dts = new DrawTextStyle;
        w.show();
        tb->setStyle(dts);
        tb->grab();
        QStyleOptionToolButton sotb;
        sotb.initFrom(tb);
        bool showMnemonic = dts->styleHint(QStyle::SH_UnderlineShortcut, &sotb, tb);
        QVERIFY(dts->alignment & (showMnemonic ? Qt::TextShowMnemonic : Qt::TextHideMnemonic));
        delete dts;
    }
    {
        QToolBar w;
        setFrameless(&w);
        QToolButton *tb = new QToolButton(&w);
        tb->setText("&abc");
        DrawTextStyle *dts = new DrawTextStyle;
        w.addWidget(tb);
        w.show();
        tb->setStyle(dts);
        tb->grab();
        QStyleOptionToolButton sotb;
        sotb.initFrom(tb);
        bool showMnemonic = dts->styleHint(QStyle::SH_UnderlineShortcut, &sotb, tb);
        QVERIFY(dts->alignment & (showMnemonic ? Qt::TextShowMnemonic : Qt::TextHideMnemonic));
        delete dts;
     }
}

#define SCROLLBAR_SPACING 33

class FrameTestStyle : public QProxyStyle {
public:
    FrameTestStyle() : QProxyStyle("Windows") { }
    int styleHint(StyleHint hint, const QStyleOption *opt, const QWidget *widget, QStyleHintReturn *returnData) const {
        if (hint == QStyle::SH_ScrollView_FrameOnlyAroundContents)
            return 1;
        return QProxyStyle ::styleHint(hint, opt, widget, returnData);
    }

    int pixelMetric(PixelMetric pm, const QStyleOption *option, const QWidget *widget) const {
        if (pm == QStyle::PM_ScrollView_ScrollBarSpacing)
            return SCROLLBAR_SPACING;
        return QProxyStyle ::pixelMetric(pm, option ,widget);
    }
};

void tst_QStyle::testFrameOnlyAroundContents()
{
    QScrollArea area;
    area.setGeometry(0, 0, 200, 200);
    QStyle *winStyle = QStyleFactory::create("Windows");
    FrameTestStyle frameStyle;
    QWidget *widget = new QWidget(&area);
    widget->setGeometry(0, 0, 400, 400);
    area.setStyle(winStyle);
    area.verticalScrollBar()->setStyle(winStyle);
    area.setWidget(widget);
    area.setVisible(true);
    int viewPortWidth = area.viewport()->width();
    area.verticalScrollBar()->setStyle(&frameStyle);
    area.setStyle(&frameStyle);
    // Test that we reserve space for scrollbar spacing
    QVERIFY(viewPortWidth == area.viewport()->width() + SCROLLBAR_SPACING);
    delete winStyle;
}


class ProxyTest: public QProxyStyle
{
    Q_OBJECT
public:
    ProxyTest(QStyle *style = 0)
        :QProxyStyle(style)
        , called(false)
    {}

    void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p, const QWidget *w) const override {
        called = true;
        return QProxyStyle::drawPrimitive(pe, opt, p, w);
    }
    mutable bool called;
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

    QStringList keys = QStyleFactory::keys();
    QVector<QStyle*> styles;
    styles.reserve(keys.size() + 1);

    styles << new QCommonStyle();

    Q_FOREACH (const QString &key, keys) {
        styles << QStyleFactory::create(key);
    }

    Q_FOREACH (QStyle *style, styles) {
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
    mutable bool invalidOptionsDetected;
    explicit TestStyleOptionInitProxy(QStyle *style = nullptr)
        : QProxyStyle(style),
          invalidOptionsDetected(false)
    {}

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

    Q_FOREACH (const QString &key, keys) {
        QStyle* style = key.isEmpty() ? new QCommonStyle : QStyleFactory::create(key);
        TestStyleOptionInitProxy testStyle;
        testStyle.setBaseStyle(style);
        testAllFunctions(style);
        QVERIFY(!testStyle.invalidOptionsDetected);
    }
}

QTEST_MAIN(tst_QStyle)
#include "tst_qstyle.moc"
