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
#include <qapplication.h>
#include <qwidget.h>
#include <qproxystyle.h>
#include <qsizepolicy.h>
//#include <QtGui>

#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QRadioButton>
#include <QStyleFactory>
#include <QSharedPointer>

// Make a widget frameless to prevent size constraints of title bars
// from interfering (Windows).
static inline void setFrameless(QWidget *w)
{
    Qt::WindowFlags flags = w->windowFlags();
    flags |= Qt::FramelessWindowHint;
    flags &= ~(Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    w->setWindowFlags(flags);
}

class tst_QGridLayout : public QObject
{
Q_OBJECT

private slots:
    void cleanup();
    void getItemPosition();
    void itemAtPosition();
    void badDistributionBug();
    void setMinAndMaxSize();
    void spacingAndSpacers();

    void spacingsAndMargins();
    void spacingsAndMargins_data();
    void minMaxSize_data();
    void minMaxSize();

    void styleDependentSpacingsAndMargins_data();
    void styleDependentSpacingsAndMargins();
    void layoutSpacing_data();
    void layoutSpacing();
    void spacing();
    void spacerWithSpacing();
    void contentsRect();
    void distributeMultiCell();

    void taskQTBUG_27420_takeAtShouldUnparentLayout();
    void taskQTBUG_40609_addingWidgetToItsOwnLayout();
    void taskQTBUG_40609_addingLayoutToItself();
    void taskQTBUG_52357_spacingWhenItemIsHidden();
    void replaceWidget();
    void dontCrashWhenExtendsToEnd();
};

static inline int visibleTopLevelWidgetCount()
{
    int result= 0;
    foreach (const QWidget *topLevel, QApplication::topLevelWidgets()) {
        if (topLevel->isVisible())
            ++result;
    }
    return result;
}

void tst_QGridLayout::cleanup()
{
    // Verify that no visible top levels are leaked. Cannot check for
    // topLevelWidgets().isEmpty() here since the data driven test layoutSpacing()
    // will appear to "leak" top levels due to it creating widgets in the test data.
    QCOMPARE(visibleTopLevelWidgetCount(), 0);
}

class ItemTestWidget : public QWidget {
public:
    ItemTestWidget();

    QGridLayout *testLayout;
    QWidget *w1;
    QWidget *w2;
    QWidget *w3;
    QSpacerItem *sp;
};

ItemTestWidget::ItemTestWidget()
    : testLayout(new QGridLayout(this))
    , w1(new QWidget(this))
    , w2(new QWidget(this))
    , w3(new QWidget(this))
    , sp(new QSpacerItem(4, 4))
{
    setObjectName("testWidget");
    setWindowTitle(QTest::currentTestFunction());

    w1->setPalette(QPalette(Qt::red));
    testLayout->addWidget(w1, 0, 0);

    testLayout->addWidget(w2, 1, 1, 2, 2);
    w2->setPalette(QPalette(Qt::green));

    testLayout->addWidget(w3, 0, 1, 1, 2);
    w3->setPalette(QPalette(Qt::blue));

    testLayout->addItem(sp, 1, 3, 2, 1);
}

void tst_QGridLayout::getItemPosition()
{
    ItemTestWidget testWidget;
    testWidget.resize(200, 200);
    testWidget.show();

    QLayoutItem *item;
    int counter = 0;

    bool seenW1 = false;
    bool seenW2 = false;
    bool seenW3 = false;
    bool seenSpacer = false;

    while ((item = testWidget.testLayout->itemAt(counter))) {
        QWidget *w = item->widget();
        int r,c,rs,cs;
        testWidget.testLayout->getItemPosition(counter, &r, &c, &rs, &cs);

//        qDebug() << "item" << counter << "has" <<r << c << rs << cs;

        if (w == testWidget.w1) {
            QVERIFY(!seenW1);
            seenW1 = true;
            QCOMPARE(r, 0);
            QCOMPARE(c, 0);
            QCOMPARE(rs, 1);
            QCOMPARE(cs, 1);
        } else if (w == testWidget.w2) {
            QVERIFY(!seenW2);
            seenW2 = true;
            QCOMPARE(r, 1);
            QCOMPARE(c, 1);
            QCOMPARE(rs, 2);
            QCOMPARE(cs, 2);
        } else if (w == testWidget.w3) {
            QVERIFY(!seenW3);
            seenW3 = true;
            QCOMPARE(r, 0);
            QCOMPARE(c, 1);
            QCOMPARE(rs, 1);
            QCOMPARE(cs, 2);
        } else {
            QVERIFY(!w);
            QVERIFY(!seenSpacer);
            seenSpacer = true;
            QCOMPARE(r, 1);
            QCOMPARE(c, 3);
            QCOMPARE(rs, 2);
            QCOMPARE(cs, 1);
        }
        ++counter;
    }
    QCOMPARE(counter, 4);
    QVERIFY(seenW1);
    QVERIFY(seenW2);
    QVERIFY(seenW3);
    QVERIFY(seenSpacer);
}

void tst_QGridLayout::itemAtPosition()
{
    ItemTestWidget testWidget;
    testWidget.resize(200, 200);
    testWidget.show();

    void *table[4][5] = {
        { testWidget.w1, testWidget.w3,testWidget.w3, 0,  0 },
        { 0,  testWidget.w2, testWidget.w2, testWidget.sp, 0 },
        { 0,  testWidget.w2, testWidget.w2, testWidget.sp, 0 },
        { 0,  0,  0,  0,  0 }
    };

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 5; ++col) {
            QLayoutItem *item = testWidget.testLayout->itemAtPosition(row, col);
            QVERIFY(item == table[row][col]
                    || (item && item->widget() == table[row][col]));
        }
    }
}

#include "ui_sortdialog.h"

void tst_QGridLayout::badDistributionBug()
{
    QDialog dialog;
    Ui::SortDialog ui;
    ui.setupUi(&dialog);
    ui.gridLayout->setMargin(0);
    ui.gridLayout->setSpacing(0);
    ui.vboxLayout->setMargin(0);
    ui.vboxLayout->setSpacing(0);
    ui.okButton->setFixedHeight(20);
    ui.moreButton->setFixedHeight(20);
    ui.primaryGroupBox->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    ui.primaryGroupBox->setFixedHeight(200);

    QSize minSize = dialog.layout()->minimumSize();
    QCOMPARE(minSize.height(), 200);
}

void tst_QGridLayout::setMinAndMaxSize()
{
    QWidget widget;
    setFrameless(&widget);
    QGridLayout layout(&widget);
    layout.setMargin(0);
    layout.setSpacing(0);
    layout.setSizeConstraint(QLayout::SetMinAndMaxSize);
    widget.show();

    QWidget leftChild;
    leftChild.setPalette(QPalette(Qt::red));
    leftChild.setMinimumSize(100, 100);
    leftChild.setMaximumSize(200, 200);
    layout.addWidget(&leftChild, 0, 0);
    QApplication::sendPostedEvents(0, 0);
    QCOMPARE(widget.minimumSize(), leftChild.minimumSize());
    QCOMPARE(widget.maximumSize(), leftChild.maximumSize());

    QWidget rightChild;
    rightChild.setPalette(QPalette(Qt::green));
    rightChild.setMinimumSize(100, 100);
    rightChild.setMaximumSize(200, 200);
    layout.addWidget(&rightChild, 0, 2);
    QApplication::sendPostedEvents(0, 0);

    QCOMPARE(widget.minimumWidth(),
             leftChild.minimumWidth() + rightChild.minimumWidth());
    QCOMPARE(widget.minimumHeight(),
             qMax(leftChild.minimumHeight(), rightChild.minimumHeight()));
    QCOMPARE(widget.maximumWidth(),
             leftChild.maximumWidth() + rightChild.maximumWidth());
    QCOMPARE(widget.maximumHeight(),
             qMax(leftChild.maximumHeight(), rightChild.maximumHeight()));


    static const int colMin = 100;
    layout.setColumnMinimumWidth(1, colMin);
    QCOMPARE(layout.columnMinimumWidth(1), colMin);

    QApplication::sendPostedEvents(0, 0);
    QCOMPARE(widget.minimumWidth(),
             leftChild.minimumWidth() + rightChild.minimumWidth() + colMin);
    QCOMPARE(widget.maximumWidth(),
             leftChild.maximumWidth() + rightChild.maximumWidth() + colMin);
    QCOMPARE(widget.minimumHeight(),
             qMax(leftChild.minimumHeight(), rightChild.minimumHeight()));
    QCOMPARE(widget.maximumHeight(),
             qMax(leftChild.maximumHeight(), rightChild.maximumHeight()));



    layout.setColumnStretch(1,1);
    QApplication::sendPostedEvents(0, 0);
    QCOMPARE(widget.minimumWidth(),
             leftChild.minimumWidth() + rightChild.minimumWidth() + colMin);
    QCOMPARE(widget.maximumWidth(), QLAYOUTSIZE_MAX);
    QCOMPARE(widget.minimumHeight(),
             qMax(leftChild.minimumHeight(), rightChild.minimumHeight()));
    QCOMPARE(widget.maximumHeight(),
             qMax(leftChild.maximumHeight(), rightChild.maximumHeight()));



    layout.setColumnStretch(1,0);
    QApplication::sendPostedEvents(0, 0);
    QCOMPARE(widget.minimumWidth(),
             leftChild.minimumWidth() + rightChild.minimumWidth() + colMin);
    QCOMPARE(widget.maximumWidth(),
             leftChild.maximumWidth() + rightChild.maximumWidth() + colMin);
    QCOMPARE(widget.minimumHeight(),
             qMax(leftChild.minimumHeight(), rightChild.minimumHeight()));
    QCOMPARE(widget.maximumHeight(),
             qMax(leftChild.maximumHeight(), rightChild.maximumHeight()));



    layout.setColumnMinimumWidth(1, 0);

    static const int spacerS = 250;
    QSpacerItem *spacer = new QSpacerItem(spacerS, spacerS);
    layout.addItem(spacer, 0, 1);
    QApplication::sendPostedEvents(0, 0);

    QCOMPARE(widget.minimumWidth(),
             leftChild.minimumWidth() + rightChild.minimumWidth() + spacerS);
    QCOMPARE(widget.maximumWidth(), QLAYOUTSIZE_MAX);
    QCOMPARE(widget.minimumHeight(),
             qMax(qMax(leftChild.minimumHeight(), rightChild.minimumHeight()), spacerS));
    QCOMPARE(widget.maximumHeight(),
             qMax(leftChild.maximumHeight(), rightChild.maximumHeight()));


    spacer->changeSize(spacerS, spacerS, QSizePolicy::Fixed, QSizePolicy::Minimum);
    layout.invalidate();
    QApplication::sendPostedEvents(0, 0);
    QCOMPARE(widget.minimumWidth(),
             leftChild.minimumWidth() + rightChild.minimumWidth() + spacerS);
    QCOMPARE(widget.maximumWidth(),
             leftChild.maximumWidth() + rightChild.maximumWidth() + spacerS);


    layout.removeItem(spacer);
    delete spacer;
    spacer = Q_NULLPTR;

    rightChild.hide();
    QApplication::sendPostedEvents(0, 0);
    QCOMPARE(widget.minimumSize(), leftChild.minimumSize());
    QCOMPARE(widget.maximumSize(), leftChild.maximumSize());

    rightChild.show();
    layout.removeWidget(&rightChild);
    QApplication::sendPostedEvents(0, 0);
    QCOMPARE(widget.minimumSize(), leftChild.minimumSize());
    QCOMPARE(widget.maximumSize(), leftChild.maximumSize());

    QWidget bottomChild(&widget);
    bottomChild.setPalette(QPalette(Qt::green));
    bottomChild.setMinimumSize(100, 100);
    bottomChild.setMaximumSize(200, 200);
    layout.addWidget(&bottomChild, 1, 0);
    QApplication::sendPostedEvents(0, 0);

    QCOMPARE(widget.minimumHeight(),
             leftChild.minimumHeight() + bottomChild.minimumHeight());
    QCOMPARE(widget.minimumWidth(),
             qMax(leftChild.minimumWidth(), bottomChild.minimumWidth()));
    QCOMPARE(widget.maximumHeight(),
             leftChild.maximumHeight() + bottomChild.maximumHeight());
    QCOMPARE(widget.maximumWidth(),
             qMax(leftChild.maximumWidth(), bottomChild.maximumWidth()));

    bottomChild.hide();
    QApplication::sendPostedEvents(0, 0);
    QCOMPARE(widget.minimumSize(), leftChild.minimumSize());
    QCOMPARE(widget.maximumSize(), leftChild.maximumSize());

    bottomChild.show();
    layout.removeWidget(&bottomChild);
    QApplication::sendPostedEvents(0, 0);
    QCOMPARE(widget.minimumSize(), leftChild.minimumSize());
    QCOMPARE(widget.maximumSize(), leftChild.maximumSize());
}


class SizeHinter : public QWidget
{
public:
    SizeHinter(const QSize &s, QWidget *parent = 0)
        : QWidget(parent), sh(s) { }
    SizeHinter(int w, int h, QWidget *parent = 0)
        : QWidget(parent), sh(QSize(w,h)) {}
    void setSizeHint(QSize s) { sh = s; }
    QSize sizeHint() const { return sh; }
private:
    QSize sh;
};

void tst_QGridLayout::spacingAndSpacers()
{
    QWidget widget;
    setFrameless(&widget);
    QGridLayout layout(&widget);
    layout.setMargin(0);
    layout.setSpacing(0);
    widget.show();

    QSize expectedSizeHint;

    SizeHinter leftChild(100,100);
    leftChild.setPalette(QPalette(Qt::red));
    layout.addWidget(&leftChild, 0, 0);
    QApplication::sendPostedEvents(0, 0);
    expectedSizeHint = leftChild.sizeHint();
    QCOMPARE(widget.sizeHint(), expectedSizeHint);


    SizeHinter rightChild(200,100);
    rightChild.setPalette(QPalette(Qt::green));
    layout.addWidget(&rightChild, 0, 2);
    QApplication::sendPostedEvents(0, 0);
    QCOMPARE(rightChild.sizeHint(), QSize(200,100));

    expectedSizeHint += QSize(rightChild.sizeHint().width(), 0);
    QCOMPARE(widget.sizeHint(), expectedSizeHint);

    layout.setColumnMinimumWidth(1, 100);
    widget.adjustSize();
    expectedSizeHint += QSize(100,0);
    QApplication::sendPostedEvents(0, 0);
    QCOMPARE(widget.sizeHint(), expectedSizeHint);

    rightChild.hide();
    QApplication::sendPostedEvents(0, 0);
    expectedSizeHint -= QSize(rightChild.sizeHint().width(), 0);
    QCOMPARE(widget.sizeHint(), expectedSizeHint);


    layout.setColumnMinimumWidth(1, 0);
    expectedSizeHint -= QSize(100, 0);
    QCOMPARE(widget.sizeHint(), expectedSizeHint);

    rightChild.show();

    layout.removeWidget(&rightChild);
    QApplication::sendPostedEvents(0, 0);
    QCOMPARE(widget.sizeHint(), expectedSizeHint);
}


class Qt42Style : public QProxyStyle
{
    Q_OBJECT
public:
    Qt42Style() : QProxyStyle(QStyleFactory::create("windows"))
    {
        spacing = 6;
        margin = 9;
        margin_toplevel = 11;
    }

    virtual int pixelMetric(PixelMetric metric, const QStyleOption * option = 0,
                            const QWidget * widget = 0 ) const;

    int spacing;
    int margin;
    int margin_toplevel;

};

int Qt42Style::pixelMetric(PixelMetric metric, const QStyleOption * option /*= 0*/,
                                   const QWidget * widget /*= 0*/ ) const
{
    switch (metric) {
        case PM_DefaultLayoutSpacing:
            return spacing;
        break;
        case PM_DefaultTopLevelMargin:
            return margin_toplevel;
        break;
        case PM_DefaultChildMargin:
            return margin;
        break;
        default:
            break;
    }
    return QProxyStyle::pixelMetric(metric, option, widget);
}


typedef QList<QPoint> PointList;


class SizeHinterFrame : public QLabel
{
public:
    SizeHinterFrame(QWidget *parent = 0)
    : QLabel(parent)
    {
        init(-1);
    }

    SizeHinterFrame(const QSize &s, int numPixels = -1)
    : QLabel(0), sh(s) {
        init(numPixels);
    }


    SizeHinterFrame(int w, int h)
    : QLabel(0), sh(QSize(w,h))
    {
        init(-1);
    }

    void setSizeHint(const QSize &s) { sh = s; }
    QSize sizeHint() const { return sh; }
    void setMinimumSizeHint(const QSize &s) { msh = s; }
    QSize minimumSizeHint() const { return msh; }

    virtual int heightForWidth(int width) const;

    void setNumberOfPixels(int numPixels) {
        m_numPixels = numPixels;
        QSizePolicy sp = sizePolicy();
        sp.setHeightForWidth(m_numPixels != -1);
        setSizePolicy(sp);
    }
private:
    void init(int numPixels = -1){
        setText(QLatin1Char('(') + QString::number(sh.width())
                + QLatin1Char(',') + QString::number(sh.height()) + QLatin1Char(')'));
        setFrameStyle(QFrame::Box | QFrame::Plain);
        setNumberOfPixels(numPixels);
    }
private:
    QSize sh;
    QSize msh;
    int m_numPixels;
};

int SizeHinterFrame::heightForWidth(int width) const
{
    // Special hack if m_numPixels == -2 then we report that we are heightForWidth aware, but we
    // return sizeHint().width() so that it should be laid out as if we had't any hfw.
    // This enables us to run the tests twice and to see if we get the same results with hfw as without hfw.
    if (m_numPixels == -2) {
        return sizeHint().height();
    }
    if (m_numPixels == -1 || width == 0) return -1;
    // this widget should always cover the same amount of pixels (provided that we don't get any rounding errors)
    return (m_numPixels)/width;
}

void tst_QGridLayout::spacingsAndMargins_data()
{
    // input
    QTest::addColumn<int>("columns");
    QTest::addColumn<int>("rows");
    QTest::addColumn<QSize>("sizehint");
    // expected
    QTest::addColumn<PointList>("expectedpositions");

    int child_offset_y = 11 + 100 + 6 + 9 ;
    QTest::newRow("1x1 grid") << 1 << 1 << QSize(100, 100)
                       << (PointList()  // toplevel
                                        << QPoint( 11, 11)
                                        // children
                                        << QPoint( 20, child_offset_y)
                       );

    QTest::newRow("2x1 grid") << 2 << 1 << QSize(100, 100)
                       << (PointList()  // toplevel
                                        << QPoint( 11, 11)
                                        << QPoint( 11 + 100 + 6, 11)
                                        // children
                                        << QPoint( 20, child_offset_y)
                                        << QPoint( 20 + 100 + 6, child_offset_y)
                                        );

    QTest::newRow("3x1 grid") << 3 << 1 << QSize(100, 100)
                       << (PointList()  // toplevel
                                        << QPoint( 11, 11)
                                        << QPoint( 11 + 100 + 6, 11)
                                        << QPoint( 11 + 100 + 6 + 100 + 6, 11)
                                        // children
                                        << QPoint( 20, child_offset_y)
                                        << QPoint( 20 + 100 + 6, child_offset_y)
                                        << QPoint( 20 + 100 + 6 + 100 + 6, child_offset_y)
                                        );

    child_offset_y = 11 + 9 + 100 + 6 + 100 + 6;
    QTest::newRow("1x2 grid") << 1 << 2 << QSize(100, 100)
                       << (PointList()  // toplevel
                                        << QPoint( 11, 11)
                                        << QPoint( 11, 11 + 100 + 6)
                                        // children
                                        << QPoint( 20, child_offset_y)
                                        << QPoint( 20, child_offset_y + 100 + 6)
                                        );
    child_offset_y = 11 + 9 + 100 + 6 + 100 + 6 + 100 + 6;
    QTest::newRow("1x3 grid") << 1 << 3 << QSize(100, 100)
                       << (PointList()  // toplevel
                                        << QPoint( 11, 11)
                                        << QPoint( 11, 11 + 100 + 6)
                                        << QPoint( 11, 11 + 100 + 6 + 100 + 6)
                                        // children
                                        << QPoint( 20, child_offset_y)
                                        << QPoint( 20, child_offset_y + 100 + 6)
                                        << QPoint( 20, child_offset_y + 100 + 6 + 100 + 6)
                                        );

    child_offset_y = 11 + 9 + 100 + 6 + 100 + 6;
    QTest::newRow("2x2 grid") << 2 << 2 << QSize(100, 100)
                       << (PointList()  // toplevel
                                        << QPoint( 11, 11)
                                        << QPoint( 11 + 100 + 6, 11)
                                        << QPoint( 11, 11 + 100 + 6)
                                        << QPoint( 11 + 100 + 6, 11 + 100 + 6)
                                        // children
                                        << QPoint( 20, child_offset_y)
                                        << QPoint( 20 + 100 + 6, child_offset_y)
                                        << QPoint( 20, child_offset_y + 100 + 6)
                                        << QPoint( 20 + 100 + 6, child_offset_y + 100 + 6)
                                        );
}

void tst_QGridLayout::spacingsAndMargins()
{
/*
    The test tests a gridlayout as a child of a top-level widget,
    and then a gridlayout as a child of a non-toplevel widget.

    The expectedpositions should then contain the list of widget positions in the
    first gridlayout, then followed by a list of widget positions in the second gridlayout.
*/
    QFETCH(int, columns);
    QFETCH(int, rows);
    QFETCH(QSize, sizehint);
    QFETCH(PointList, expectedpositions);


    QApplication::setStyle(new Qt42Style);
    QWidget toplevel;
    setFrameless(&toplevel);

    QVBoxLayout vbox(&toplevel);
    QGridLayout grid1;
    vbox.addLayout(&grid1);

    // a layout with a top-level parent widget
    QList<QPointer<SizeHinterFrame> > sizehinters;
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < columns; ++j) {
            SizeHinterFrame *sh = new SizeHinterFrame(sizehint);
            sh->setMinimumSizeHint(sizehint);
            sizehinters.append(sh);
            grid1.addWidget(sh, i, j);
        }
    }

    // Add the child widget
    QWidget widget;
    vbox.addWidget(&widget);
    QGridLayout grid2;
    widget.setLayout(&grid2);
    // add a layout to the child widget
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < columns; ++j) {
            SizeHinterFrame *sh = new SizeHinterFrame(sizehint);
            sh->setMinimumSizeHint(sizehint);
            sizehinters.append(sh);
            grid2.addWidget(sh, i, j);
        }
    }

    grid1.setColumnStretch(columns-1, 1);
    grid1.setRowStretch(rows-1, 1);
    toplevel.showNormal();
    toplevel.adjustSize();
    QApplication::processEvents();
    QVERIFY(QTest::qWaitForWindowExposed(&toplevel));

    QSize topsize = toplevel.size();
    QSize minimumsize = vbox.totalMinimumSize();

    if (topsize.width() < minimumsize.width() || topsize.height() < minimumsize.height())
        QSKIP("The screen is too small to run this test case");

// We are relying on the order here...
    for (int pi = 0; pi < sizehinters.count(); ++pi) {
        QPoint pt = sizehinters.at(pi)->mapTo(&toplevel, QPoint(0, 0));
        QCOMPARE(pt, expectedpositions.at(pi));
    }
}




struct SizeInfo {
    SizeInfo(const QPoint &expected, const QSize &sh, const QSize &minimumSize = QSize(),
             const QSize &maximumSize = QSize(), int numPixelsToCover = -1)
    {
        expectedPos = expected;
        sizeHint = sh;
        minSize = minimumSize;
        maxSize = maximumSize;
        hfwNumPixels = numPixelsToCover;
    }

    SizeInfo(const QRect &expected, const QSize &sh, const QSize &minimumSize = QSize(),
             const QSize &maximumSize = QSize(), int numPixelsToCover = -1)
    {
        expectedPos = expected.topLeft();
        expectedSize = expected.size();
        sizeHint = sh;
        minSize = minimumSize;
        maxSize = maximumSize;
        hfwNumPixels = numPixelsToCover;
    }
    SizeInfo(const SizeInfo& other) {
        (*this)=other;
    }

    SizeInfo &operator=(const SizeInfo& other) {
        expectedPos = other.expectedPos;
        expectedSize = other.expectedSize;
        sizeHint = other.sizeHint;
        minSize = other.minSize;
        maxSize = other.maxSize;
        hfwNumPixels = other.hfwNumPixels;
        return (*this);
    }

    QPoint expectedPos;
    QSize expectedSize;
    QSize sizeHint;
    QSize minSize;
    QSize maxSize;
    int hfwNumPixels;
};


typedef QList<SizeInfo> SizeInfoList;
Q_DECLARE_METATYPE(SizeInfoList)


void tst_QGridLayout::minMaxSize_data()
{
    // input
    QTest::addColumn<QString>("stylename");
    QTest::addColumn<int>("columns");
    QTest::addColumn<int>("rows");
    QTest::addColumn<int>("sizePolicy");
    QTest::addColumn<QSize>("fixedSize");
    //input and expected output
    QTest::addColumn<SizeInfoList>("sizeinfos");

    QTest::newRow("3x1 grid, extend to minimumSize") << QString() << 3 << 1
                << int(QSizePolicy::Minimum) << QSize(152, 50) << (SizeInfoList()
                << SizeInfo(QRect(10, 10, 43, 30), QSize( 75, 75), QSize(0,0))
                << SizeInfo(QRect(10 + 45, 10, 43, 30), QSize(75, 75), QSize( 0, 0))
                << SizeInfo(QRect(10 + 45 + 44, 10, 42, 30), QSize(75, 75), QSize( 0, 0))
                );

    QTest::newRow("1x1 grid, extend to minimumSize") << QString() << 1 << 1
                << int(QSizePolicy::Preferred) << QSize() << (SizeInfoList()
                << SizeInfo(QPoint(10, 10), QSize( 90, 90), QSize(100,100))
                );
    QTest::newRow("2x1 grid, extend to minimumSize") << QString() << 2 << 1
                << int(QSizePolicy::Preferred) << QSize() << (SizeInfoList()
                << SizeInfo(QPoint(10, 10), QSize( 90, 90), QSize(100,100))
                << SizeInfo(QPoint(10 + 100 + 1, 10), QSize( 90, 90))
                );
    QTest::newRow("2x1 grid, extend to minimumSize, windows") << QString::fromLatin1("windows") << 2 << 1
                << int(QSizePolicy::Preferred) << QSize() << (SizeInfoList()
                << SizeInfo(QPoint(11, 11), QSize( 90, 90), QSize(100,100))
                << SizeInfo(QPoint(11 + 100 + 6, 11), QSize( 90, 90))
                );
    QTest::newRow("1x2 grid, extend to minimumSize") << QString() << 1 << 2
                << int(QSizePolicy::Preferred) << QSize() << (SizeInfoList()
                << SizeInfo(QPoint(10, 10), QSize( 90, 90), QSize(100,100))
                << SizeInfo(QPoint(10, 10 + 100 + 1), QSize( 90, 90))
                );
    QTest::newRow("2x1 grid, crop to maximumSize") << QString() << 2 << 1
                << int(QSizePolicy::Preferred) << QSize() << (SizeInfoList()
            << SizeInfo(QPoint(10, 10), QSize(110,110), QSize(), QSize(100, 100))
            << SizeInfo(QPoint(10 + 100 + 1, 10), QSize( 90, 90))
            );
    QTest::newRow("1x2 grid, crop to maximumSize") << QString() << 1 << 2
                << int(QSizePolicy::Preferred) << QSize() << (SizeInfoList()
            << SizeInfo(QPoint(10, 10), QSize(110,110), QSize(), QSize(100, 100))
            << SizeInfo(QPoint(10, 10 + 100 + 1), QSize( 90, 90))
            );
    QTest::newRow("1x3 grid, heightForWidth") << QString() << 1 << 3
                << int(QSizePolicy::Preferred) << QSize() << (SizeInfoList()
            << SizeInfo(QPoint(10, 10), QSize(), QSize(200,100), QSize())
            << SizeInfo(QPoint(10, 10 + 100 + 1), QSize(100,100), QSize(), QSize(), 100*100)
            << SizeInfo(QPoint(10, 10 + 100 + 1 + 50 + 1), QSize(100,100), QSize(), QSize(100, 100))
            );
}

void tst_QGridLayout::minMaxSize()
{
/*
    The test tests a gridlayout as a child of a top-level widget
*/
    // input
    QFETCH(QString, stylename);
    QFETCH(int, columns);
    QFETCH(int, rows);
    QFETCH(int, sizePolicy);
    QFETCH(QSize, fixedSize);
    //input and expected output
    QFETCH(SizeInfoList, sizeinfos);

    QStyle *style = 0;
    if (stylename.isEmpty()) {
        Qt42Style *s = new Qt42Style;
        s->margin_toplevel = 10;
        s->margin = 5;
        s->spacing = 1;
        style = static_cast<QStyle *>(s);
    }else{
        style = QStyleFactory::create(stylename);
        if (!style) {
            QSKIP( qPrintable(QString::fromLatin1("Qt has been compiled without style: %1").arg(stylename)));
        }
    }
    QApplication::setStyle(style);
    QWidget toplevel;
    toplevel.setWindowTitle(QLatin1String(QTest::currentTestFunction())
                            + QLatin1Char(' ') + QLatin1String(QTest::currentDataTag()));
    setFrameless(&toplevel);
    QGridLayout *grid = new QGridLayout;
    if (fixedSize.isValid()) {
        toplevel.setFixedSize(fixedSize);
    } else {
        toplevel.setMinimumSize(QSize(0,0));
        toplevel.setMaximumSize(QSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX));
    }
    // Do a two-pass one using the real testdata, the other pass enables heightForWidth
    // on the widget, but the heightForWidth() function just return sizeHint().width()
    for (int pass = 0; pass < 2; ++pass) {
        toplevel.hide();
        QApplication::processEvents();
        QTest::qWait(20);
        // Test if removeItem uninitializes data properly
        while (grid->count()) {
            QLayoutItem *item = grid->itemAt(0);
            grid->removeItem(item);
            delete item->widget();
            delete item;
        }
        toplevel.setLayout(grid);

        // a layout with a top-level parent widget
        QList<QPointer<SizeHinterFrame> > sizehinters;
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < columns; ++j) {
                SizeInfo si = sizeinfos.at(sizehinters.count());
                int numpixels = si.hfwNumPixels;
                if (pass == 1 && numpixels == -1)
                    numpixels = -2; //### yuk, (and don't fake it if it already tests sizehint)
                SizeHinterFrame *sh = new SizeHinterFrame(si.sizeHint, numpixels);
                QSizePolicy sp = sh->sizePolicy();
                sp.setHorizontalPolicy((QSizePolicy::Policy)sizePolicy);
                sh->setSizePolicy(sp);
                sh->setParent(&toplevel);
                if (si.minSize.isValid())
                    sh->setMinimumSize(si.minSize);
                if (si.maxSize.isValid())
                    sh->setMaximumSize(si.maxSize);
                sizehinters.append(sh);
                grid->addWidget(sh, i, j);
            }
        }

        toplevel.show();
        QVERIFY(QTest::qWaitForWindowExposed(&toplevel));
        toplevel.adjustSize();
        QTest::qWait(240);                              // wait for the implicit adjustSize
        // If the following fails we might have to wait longer.
        // If that does not help there is likely a problem with the implicit adjustSize in show()
        if (!fixedSize.isValid()) {
            // Note that this can fail if the desktop has large fonts on windows.
            QTRY_COMPARE(toplevel.size(), toplevel.sizeHint());
        }
        // We are relying on the order here...
        for (int pi = 0; pi < sizehinters.count(); ++pi) {
            QPoint pt = sizehinters.at(pi)->mapTo(&toplevel, QPoint(0, 0));
            QCOMPARE(pt, sizeinfos.at(pi).expectedPos);
        }
    }
}


class CustomLayoutStyle : public QProxyStyle
{
    Q_OBJECT
public:
    CustomLayoutStyle() : QProxyStyle(QStyleFactory::create("windows"))
    {
        hspacing = 5;
        vspacing = 10;
        reimplementSubelementRect = false;
    }

    virtual int pixelMetric(PixelMetric metric, const QStyleOption * option = 0,
                            const QWidget * widget = 0 ) const;
    virtual QRect subElementRect(SubElement sr, const QStyleOption *opt,
                                const QWidget *widget) const;

    int hspacing;
    int vspacing;
    bool reimplementSubelementRect;

    int layoutSpacing(QSizePolicy::ControlType control1,
                      QSizePolicy::ControlType control2,
                      Qt::Orientation orientation,
                      const QStyleOption *option = 0,
                      const QWidget *widget = 0) const;

};

QRect CustomLayoutStyle::subElementRect(SubElement sr, const QStyleOption *opt,
                                const QWidget *widget) const
{
    QRect rect;
    if (reimplementSubelementRect) {
        switch (sr) {
        case SE_FrameLayoutItem:
            rect = opt->rect;
            rect.adjust(+4, +9, -4, 0); // The hspacing=5 and vspacing=10, so we keep it safe.
            break;
        case SE_GroupBoxLayoutItem:
            rect = opt->rect.adjusted(0, +10, 0, 0);
            break;
        default:
            break;
        }
    }
    if (rect.isNull())
        rect = QProxyStyle::subElementRect(sr, opt, widget);
    return rect;
}

#define CT1(c) CT2(c, c)
#define CT2(c1, c2) ((uint)c1 << 16) | (uint)c2

int CustomLayoutStyle::layoutSpacing(QSizePolicy::ControlType control1,
                                QSizePolicy::ControlType control2,
                                Qt::Orientation orientation,
                                const QStyleOption * /*option = 0*/,
                                const QWidget * /*widget = 0*/) const
{
    if (orientation == Qt::Horizontal) {
        switch (CT2(control1, control2)) {
            case CT1(QSizePolicy::PushButton):
                return 2;
                break;
        }
        return 5;
    } else {
        switch (CT2(control1, control2)) {
            case CT1(QSizePolicy::RadioButton):
                return 2;
                break;

        }
        return 10;
    }
}

int CustomLayoutStyle::pixelMetric(PixelMetric metric, const QStyleOption * option /*= 0*/,
                                   const QWidget * widget /*= 0*/ ) const
{
    switch (metric) {
        case PM_LayoutLeftMargin:
            return 0;
        break;
        case PM_LayoutTopMargin:
            return 3;
        break;
        case PM_LayoutRightMargin:
            return 6;
        break;
        case PM_LayoutBottomMargin:
            return 9;
        break;
        case PM_LayoutHorizontalSpacing:
            return hspacing;
        case PM_LayoutVerticalSpacing:
            return vspacing;
        break;
        default:
            break;
    }
    return QProxyStyle::pixelMetric(metric, option, widget);
}

void tst_QGridLayout::styleDependentSpacingsAndMargins_data()
{
    // input
    QTest::addColumn<int>("columns");
    QTest::addColumn<int>("rows");
    QTest::addColumn<QSize>("sizehint");
    // expected
    QTest::addColumn<PointList>("expectedpositions");

    QTest::newRow("1x1 grid") << 1 << 1 << QSize(100, 100)
                       << (PointList() << QPoint(0, 3) );
    QTest::newRow("2x1 grid") << 2 << 1 << QSize(100, 100)
                       << (PointList() << QPoint(0, 3)
                                       << QPoint(0+100+5, 3));
    QTest::newRow("3x1 grid") << 3 << 1 << QSize(100, 100)
                       << (PointList() << QPoint(0, 3)
                                       << QPoint(0+100+5, 3)
                                       << QPoint(0 + 2*105, 3));
    QTest::newRow("1x2 grid") << 1 << 2 << QSize(100, 100)
                       << (PointList() << QPoint(0, 3)
                                       << QPoint(0, 3+100+10));
    QTest::newRow("1x3 grid") << 1 << 3 << QSize(100, 100)
                       << (PointList() << QPoint(0, 3)
                                       << QPoint(0, 3+100+10)
                                       << QPoint(0, 3+2*110));
    QTest::newRow("2x2 grid") << 2 << 2 << QSize(100, 100)
                       << (PointList() << QPoint(0, 3) << QPoint(0+100+5, 3)
                                       << QPoint(0, 3+100+10) << QPoint(0+100+5, 3+100+10));
}


void tst_QGridLayout::styleDependentSpacingsAndMargins()
{
    QFETCH(int, columns);
    QFETCH(int, rows);
    QFETCH(QSize, sizehint);
    QFETCH(PointList, expectedpositions);

    QApplication::setStyle(new CustomLayoutStyle());
    QWidget widget;
    setFrameless(&widget);
    QGridLayout layout(&widget);
    QList<QPointer<SizeHinterFrame> > sizehinters;
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < columns; ++j) {
            SizeHinterFrame *sh = new SizeHinterFrame(sizehint);
            sh->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
            sh->setParent(&widget);
            sizehinters.append(sh);
            layout.addWidget(sh, i, j);
        }
    }
    layout.setColumnStretch(columns, 1);
    layout.setRowStretch(rows, 1);
    widget.show();
    widget.adjustSize();
    QApplication::processEvents();

    for (int pi = 0; pi < expectedpositions.count(); ++pi) {
        QCOMPARE(sizehinters.at(pi)->pos(), expectedpositions.at(pi));
    }
}

void tst_QGridLayout::layoutSpacing_data()
{
    QTest::addColumn<QWidget*>("widget");
    // expected
    QTest::addColumn<PointList>("expectedpositions");
    QTest::addColumn<int>("hSpacing");
    QTest::addColumn<int>("vSpacing");
    QTest::addColumn<bool>("customSubElementRect");

    CustomLayoutStyle *style = new CustomLayoutStyle();
    {
        // If the layoutSpacing is negative, the layouting code will call
        // layoutSpacing()
        style->hspacing = -1;
        style->vspacing = -1;
        style->reimplementSubelementRect = false;
        QApplication::setStyle(style);
        QWidget *w = new QWidget();
        setFrameless(w);
        QVBoxLayout *layout = new QVBoxLayout();
        QRadioButton *rb1 = new QRadioButton(QLatin1String("Radio 1"), w);
        QRadioButton *rb2 = new QRadioButton(QLatin1String("Radio 2"), w);
        QRadioButton *rb3 = new QRadioButton(QLatin1String("Radio 3"), w);
        layout->addWidget(rb1, 0, Qt::AlignTop | Qt::AlignLeft);
        layout->addWidget(rb2, 0, Qt::AlignTop | Qt::AlignLeft);
        layout->addWidget(rb3, 0, Qt::AlignTop | Qt::AlignLeft);

        QPushButton *b1 = new QPushButton(QLatin1String("Push 1"), w);
        QPushButton *b2 = new QPushButton(QLatin1String("Push 2"), w);
        QPushButton *b3 = new QPushButton(QLatin1String("Push 3"), w);
        layout->addWidget(b1, 0, Qt::AlignTop | Qt::AlignLeft);
        layout->addWidget(b2, 0, Qt::AlignTop | Qt::AlignLeft);
        layout->addWidget(b3, 0, Qt::AlignTop | Qt::AlignLeft);

        layout->addStretch(1);
        w->setLayout(layout);
        int rh = rb1->sizeHint().height();
        int ph = b1->sizeHint().height();
        QTest::newRow("1x6, radio + push buttons")
                << w << (PointList()
                << QPoint(0, 3)
                << QPoint(0, 3 + rh + 2)
                << QPoint(0, 3 + 2*(rh + 2))
                << QPoint(0, 3 + 2*(rh + 2) +   (rh + 10))
                << QPoint(0, 3 + 2*(rh + 2) + (rh + 10 + ph + 10))
                << QPoint(0, 3 + 2*(rh + 2) +  rh + 10 + 2*(ph + 10)))
                << style->hspacing << style->vspacing << style->reimplementSubelementRect;
    }

    {
        style->hspacing = -1;
        style->vspacing = -1;
        style->reimplementSubelementRect = false;
        QApplication::setStyle(style);
        QWidget *w = new QWidget();
        setFrameless(w);
        QHBoxLayout *layout = new QHBoxLayout();
        QLineEdit *le1 = new QLineEdit(w);
        QLineEdit *le2 = new QLineEdit(w);
        QLineEdit *le3 = new QLineEdit(w);
        layout->addWidget(le1, 0, Qt::AlignTop | Qt::AlignLeft);
        layout->addWidget(le2, 0, Qt::AlignTop | Qt::AlignLeft);
        layout->addWidget(le3, 0, Qt::AlignTop | Qt::AlignLeft);

        QPushButton *b1 = new QPushButton(QLatin1String("Push 1"), w);
        QPushButton *b2 = new QPushButton(QLatin1String("Push 2"), w);
        QPushButton *b3 = new QPushButton(QLatin1String("Push 3"), w);
        layout->addWidget(b1, 0, Qt::AlignTop | Qt::AlignLeft);
        layout->addWidget(b2, 0, Qt::AlignTop | Qt::AlignLeft);
        layout->addWidget(b3, 0, Qt::AlignTop | Qt::AlignLeft);

        layout->addStretch(1);
        w->setLayout(layout);
        int lw = le1->sizeHint().width();
        int pw = b1->sizeHint().width();
        QTest::newRow("6x1, line edit + push buttons")
                << w << (PointList()
                << QPoint(0, 3)
                << QPoint(0 + lw + 5, 3)
                << QPoint(0 + 2*(lw + 5), 3)
                << QPoint(0 + 3*(lw + 5), 3)
                << QPoint(0 + 3*(lw + 5) + 1*(pw + 2), 3)
                << QPoint(0 + 3*(lw + 5) + 2*(pw + 2), 3))
                << style->hspacing << style->vspacing << style->reimplementSubelementRect;
    }


    {
        style->hspacing = 5;
        style->vspacing = 10;
        style->reimplementSubelementRect = true;
        QApplication::setStyle(style);
        QWidget *w = new QWidget();
        setFrameless(w);
        QVBoxLayout *layout = new QVBoxLayout();
        QPushButton *pb1 = new QPushButton(QLatin1String("Push 1"), w);

        QGroupBox *g1 = new QGroupBox(QLatin1String("GroupBox 1"), w);

        QRadioButton *rb = new QRadioButton(QLatin1String("Radio 1"), g1);
        QVBoxLayout *g1layout = new QVBoxLayout();
        g1layout->addWidget(rb);
        g1->setLayout(g1layout);

        QPushButton *pb3 = new QPushButton(QLatin1String("Push 3"), w);

        layout->addWidget(pb1);
        layout->addWidget(g1 );
        layout->addWidget(pb3);

        w->setLayout(layout);
        QSize psh = pb1->sizeHint();
        QSize gsh = g1->sizeHint();

        QTest::newRow("subElementRect1")
                << w << (PointList()
                << QPoint(0, 3)
                << QPoint(0, 3 + psh.height() + 10 - 10)
                << QPoint(0, 3 + psh.height() + 10 - 10 + gsh.height() + 10)
                )
                << style->hspacing << style->vspacing << style->reimplementSubelementRect;
    }


    {
        style->hspacing = 5;
        style->vspacing = 10;
        style->reimplementSubelementRect = true;
        QApplication::setStyle(style);
        QWidget *w = new QWidget();
        QFont font;
        font.setPixelSize(10);
        w->setFont(font);
        setFrameless(w);
        QGridLayout *layout = new QGridLayout();
        QPushButton *pb1 = new QPushButton(QLatin1String("Push 1"), w);
        QPushButton *pb2 = new QPushButton(QLatin1String("Push 2"), w);
        QPushButton *pb3 = new QPushButton(QLatin1String("Push 3"), w);
        QPushButton *pb4 = new QPushButton(QLatin1String("Push 4"), w);

        layout->addWidget(pb1, 0, 0);
        layout->addWidget(pb2, 0, 1);
        layout->addWidget(pb3, 0, 2);
        layout->addWidget(pb4, 1, 0, Qt::AlignTop);


        QFrame *f1 = new QFrame(w);
        f1->setFrameStyle(QFrame::Box | QFrame::Plain);
        f1->setMinimumSize(100, 20);
        f1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        layout->addWidget(f1, 1, 1, Qt::AlignTop);


        QPushButton *pb6 = new QPushButton(QLatin1String("Push 6"), w);
        QPushButton *pb7 = new QPushButton(QLatin1String("Push 7"), w);
        QPushButton *pb8 = new QPushButton(QLatin1String("Push 8"), w);
        QPushButton *pb9 = new QPushButton(QLatin1String("Push 9"), w);
        layout->addWidget(pb6, 1, 2, Qt::AlignTop);
        layout->addWidget(pb7, 2, 0, Qt::AlignTop);
        layout->addWidget(pb8, 2, 1, Qt::AlignTop);
        layout->addWidget(pb9, 2, 2, Qt::AlignTop);

        layout->setColumnStretch(2, 1);
        layout->setRowStretch(2, 1);
        w->setLayout(layout);
        int c[3];
        c[0] = pb1->sizeHint().width();
        c[1] = f1->minimumSize().width() - 2*4;
        c[2] = pb3->sizeHint().width();

        int r[3];
        r[0] = pb1->sizeHint().height();
        r[1] = pb4->sizeHint().height();
        r[2] = pb7->sizeHint().height();


        QTest::newRow("subElementRect2")
                << w << (PointList()
                << QPoint(0, 3)
                << QPoint(0 + c[0] + 5, 3)
                << QPoint(0 + c[0] + 5 + c[1] + 5, 3)

                << QPoint(0, 3 + r[0] + 10)
                << QPoint(0 + c[0] + 5 - 4, 3 + r[0] + 10 - 9)
                << QPoint(0 + c[0] + 5 + c[1] + 5, 3 + r[0] + 10)

                << QPoint(0, 3 + r[0] + 10 + r[1] + 10)
                << QPoint(0 + c[0] + 5, 3 + r[0] + 10 + r[1] + 10)
                << QPoint(0 + c[0] + 5 + c[1] + 5, 3 + r[0] + 10 + r[1] + 10)

                )
                << style->hspacing << style->vspacing << style->reimplementSubelementRect;
    }

    {
        style->hspacing = 5;
        style->vspacing = 10;
        style->reimplementSubelementRect = true;
        QApplication::setStyle(style);
        QWidget *w = new QWidget();
        setFrameless(w);
        QVBoxLayout *layout = new QVBoxLayout();
        QPushButton *pb1 = new QPushButton(QLatin1String("Push 1"), w);

        QGroupBox *g1 = new QGroupBox(QLatin1String("GroupBox 1"), w);

        QRadioButton *rb = new QRadioButton(QLatin1String("Radio 1"), g1);
        QVBoxLayout *g1layout = new QVBoxLayout();
        g1layout->addWidget(rb);
        g1->setLayout(g1layout);

        QPushButton *pb3 = new QPushButton(QLatin1String("Push 3"), w);

        pb1->setAttribute(Qt::WA_LayoutUsesWidgetRect, true);
        g1->setAttribute(Qt::WA_LayoutUsesWidgetRect, true);
        pb3->setAttribute(Qt::WA_LayoutUsesWidgetRect, true);
        layout->addWidget(pb1);
        layout->addWidget(g1 );
        layout->addWidget(pb3);

        w->setLayout(layout);
        QSize psh = pb1->sizeHint();
        QSize gsh = g1->sizeHint();

        QTest::newRow("subElementRect1, use widgetRect")
                << w << (PointList()
                << QPoint(0, 3)
                << QPoint(0, 3 + psh.height() + 10)
                << QPoint(0, 3 + psh.height() + 10 + gsh.height() + 10)
                )
                << style->hspacing << style->vspacing << style->reimplementSubelementRect;
    }


    {
        style->hspacing = 5;
        style->vspacing = 10;
        style->reimplementSubelementRect = true;
        QApplication::setStyle(style);
        QWidget *w = new QWidget();
        setFrameless(w);
        QVBoxLayout *layout = new QVBoxLayout();
        QPushButton *pb1 = new QPushButton(QLatin1String("Push 1"), w);

        QGroupBox *g1 = new QGroupBox(QLatin1String("GroupBox 1"), w);

        QRadioButton *rb = new QRadioButton(QLatin1String("Radio 1"), g1);
        QVBoxLayout *g1layout = new QVBoxLayout();
        g1layout->addWidget(rb);
        g1->setLayout(g1layout);

        QPushButton *pb3 = new QPushButton(QLatin1String("Push 3"), w);

        pb1->setAttribute(Qt::WA_LayoutUsesWidgetRect, false);
        g1->setAttribute(Qt::WA_LayoutUsesWidgetRect, false);
        pb3->setAttribute(Qt::WA_LayoutUsesWidgetRect, false);
        layout->addWidget(pb1);
        layout->addWidget(g1 );
        layout->addWidget(pb3);

        w->setLayout(layout);
        QSize psh = pb1->sizeHint();
        QSize gsh = g1->sizeHint();

        QTest::newRow("subElementRect1, use layoutItemRect")
                << w << (PointList()
                << QPoint(0, 3)
                << QPoint(0, 3 + psh.height() + 10 - 10)
                << QPoint(0, 3 + psh.height() + 10 - 10 + gsh.height() + 10)
                )
                << style->hspacing << style->vspacing << style->reimplementSubelementRect;
    }


    {
        /* A 3x4 gridlayout, modified arrowpad example:
         *      [PB]
         * [PB]      [PB]
         *      |PB|
         *      |  |
         *      |  |
         *
         * Here the bottom pushbutton has a span
         */
        style->hspacing = -1;
        style->vspacing = -1;
        style->reimplementSubelementRect = false;
        QApplication::setStyle(style);
        QWidget *w = new QWidget();
        setFrameless(w);
        QGridLayout *layout = new QGridLayout();
        QPushButton *left = new QPushButton(w);
        QPushButton *up = new QPushButton(w);
        QPushButton *right = new QPushButton(w);
        QPushButton *down = new QPushButton(w);

        layout->addWidget(up, 0, 1);
        layout->addWidget(left, 1, 0);
        layout->addWidget(right, 1, 2);
        layout->addWidget(down, 2, 1, 3, 1);

        w->setLayout(layout);
        int pw = up->sizeHint().width();
        int ph = up->sizeHint().height();
        QTest::newRow("arrowpad with span")
                << w << (PointList()
                << QPoint(0 + pw + 5, 3)
                << QPoint(0, 3 + ph + 10)
                << QPoint(0 + pw + 5 + pw + 5, 3 + ph + 10)
                << QPoint(0 + pw + 5, 3 + ph + 10 + ph + 10)
                )
                << style->hspacing << style->vspacing << style->reimplementSubelementRect;
    }


    for (int yoff = 0; yoff < 5; ++yoff)
    {
        for (int xoff = 0; xoff < 5; ++xoff) {
            /* A 3x4 gridlayout, modified arrowpad example:
             * [empty cells]
             *      [PB]
             * [PB]      [PB]
             *      [PB]
             *
             * It has 0-4 empty rows at the top and 0-4 empty columns to the left.
             */
            style->hspacing = -1;
            style->vspacing = -1;
            style->reimplementSubelementRect = false;
            QApplication::setStyle(style);
            QWidget *w = new QWidget();
            setFrameless(w);
            QGridLayout *layout = new QGridLayout();
            QPushButton *left = new QPushButton(w);
            QPushButton *up = new QPushButton(w);
            QPushButton *right = new QPushButton(w);
            QPushButton *down = new QPushButton(w);

            layout->addWidget(up, yoff + 0, xoff + 1);
            layout->addWidget(left, yoff + 1, xoff + 0);
            layout->addWidget(right, yoff + 1, xoff + 2);
            layout->addWidget(down, yoff + 2, xoff + 1, 3, 1);

            w->setLayout(layout);
            int pw = up->sizeHint().width();
            int ph = up->sizeHint().height();
            QByteArray testName = "arrowpad with " + QByteArray::number(yoff)
                + " empty rows, " + QByteArray::number(xoff) + " empty columns";
            QTest::newRow(testName.constData())
                    << w << (PointList()
                    << QPoint(0 + pw + 5, 3)
                    << QPoint(0, 3 + ph + 10)
                    << QPoint(0 + pw + 5 + pw + 5, 3 + ph + 10)
                    << QPoint(0 + pw + 5, 3 + ph + 10 + ph + 10)
                    )
                    << style->hspacing << style->vspacing << style->reimplementSubelementRect;
        }
    }

}

void tst_QGridLayout::layoutSpacing()
{
    QFETCH(QWidget *, widget);
    QFETCH(PointList, expectedpositions);
    QFETCH(int, hSpacing);
    QFETCH(int, vSpacing);
    QFETCH(bool, customSubElementRect);

    QWidget toplevel;
    setFrameless(&toplevel);

    CustomLayoutStyle *style = new CustomLayoutStyle();
    style->hspacing = hSpacing;
    style->vspacing = vSpacing;
    style->reimplementSubelementRect = customSubElementRect;
    QApplication::setStyle(style);
    widget->setParent(&toplevel);
    widget->resize(widget->sizeHint());
    toplevel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&toplevel));

    QLayout *layout = widget->layout();
    QVERIFY(layout);
    //QTest::qWait(2000);
    for (int pi = 0; pi < expectedpositions.count(); ++pi) {
        QLayoutItem *item = layout->itemAt(pi);
        //qDebug()  << item->widget()->pos();
        QCOMPARE(item->widget()->pos(), expectedpositions.at(pi));
    }
}

void tst_QGridLayout::spacing()
{
    QWidget w;
    setFrameless(&w);
    CustomLayoutStyle *style = new CustomLayoutStyle();
    style->hspacing = 5;
    style->vspacing = 10;
    w.setStyle(style);
    QGridLayout grid(&w);
    QCOMPARE(style->hspacing, grid.horizontalSpacing());
    QCOMPARE(style->vspacing, grid.verticalSpacing());

    QCOMPARE(grid.spacing(), -1);
    grid.setVerticalSpacing(5);
    QCOMPARE(5, grid.horizontalSpacing());
    QCOMPARE(5, grid.verticalSpacing());
    QCOMPARE(grid.spacing(), 5);
    grid.setVerticalSpacing(-1);
    QCOMPARE(style->hspacing, grid.horizontalSpacing());
    QCOMPARE(style->vspacing, grid.verticalSpacing());

    style->hspacing = 5;
    style->vspacing = 5;
    QCOMPARE(grid.spacing(), 5);


    grid.setHorizontalSpacing(20);
    QCOMPARE(grid.spacing(), -1);
    style->vspacing = 20;
    QCOMPARE(grid.horizontalSpacing(), 20);
    QCOMPARE(grid.verticalSpacing(), 20);
    QCOMPARE(grid.spacing(), 20);
    grid.setHorizontalSpacing(-1);
    QCOMPARE(grid.spacing(), -1);
    style->hspacing = 20;
    QCOMPARE(grid.spacing(), 20);


    delete style;
}

void populate(QGridLayout *layout, int row, int kind)
{
    if (kind == 0) {
        QWidget *widget = new QWidget;
        widget->setFixedSize(100, 100);
        layout->addWidget(widget, row, 0);
    } else if (kind == 1) {
        layout->addItem(new QSpacerItem(10, 10), row, 0);
    }
}

void tst_QGridLayout::spacerWithSpacing()
{
    // Tests all combinations of widget (w), spacer (s) and no layoutitem (-)
    // to see if they are laid out correctly.
    // Note that a combination of "s-" or "-s" should only give the height of "s"
    const int expectedHeight[] = {
        302,// www
        211,// wws
        201,// ww-
        211,// wsw
        120,// wss
        110,// ws-
        201,// w-w
        110,// w-s
        100,// w--
        211,// sww
        120,// sws
        110,// sw-
        120,// ssw
         30,// sss
         20,// ss-
        110,// s-w
         20,// s-s
         10,// s--
        201,// -ww
        110,// -ws
        100,// -w-
        110,// -sw
         20,// -ss
         10,// -s-
        100,// --w
         10,// --s
        000 // ---
        };
    int ii = 0;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            for (int k = 0; k < 3; ++k) {
                QWidget window;
                QGridLayout layout(&window);
                layout.setSpacing(1);
                layout.setMargin(0);
                populate(&layout, 0, i);
                populate(&layout, 1, j);
                populate(&layout, 2, k);
                QCOMPARE(window.sizeHint().height(), expectedHeight[ii]);
                ++ii;
            }
        }
    }
}

void tst_QGridLayout::contentsRect()
{
    QWidget w;
    setFrameless(&w);
    QGridLayout grid;
    w.setLayout(&grid);
    grid.addWidget(new QPushButton(&w));
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    int l, t, r, b;
    grid.getContentsMargins(&l, &t, &r, &b);
    QRect geom = grid.geometry();

    QCOMPARE(geom.adjusted(+l, +t, -r, -b), grid.contentsRect());

}

void tst_QGridLayout::distributeMultiCell()
{
    QWidget w;
    Qt42Style style;
    style.spacing = 9;

    w.setStyle(&style);
    QGridLayout grid;
    w.setLayout(&grid);

    SizeHinter le1(200, 20, &w);
    le1.setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    SizeHinter le2(200, 20, &w);
    le2.setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    SizeHinter box(80, 57, &w);
    box.setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));
    box.setMinimumSize(80, 57);

    grid.addWidget(&le1, 0, 0, 1, 1);
    grid.addWidget(&le2, 1, 0, 1, 1);
    grid.addWidget(&box, 0, 1, 2, 1);

    QCOMPARE(box.sizeHint().height(), 57);
    QCOMPARE(w.sizeHint().height(), 11 + 57 + 11);
}

void tst_QGridLayout::taskQTBUG_27420_takeAtShouldUnparentLayout()
{
    QSharedPointer<QGridLayout> outer(new QGridLayout);
    QPointer<QGridLayout> inner = new QGridLayout;

    outer->addLayout(inner, 0, 0);
    QCOMPARE(outer->count(), 1);
    QCOMPARE(inner->parent(), outer.data());

    QLayoutItem *item = outer->takeAt(0);
    QCOMPARE(item->layout(), inner.data());
    QVERIFY(!item->layout()->parent());

    outer.reset();

    if (inner)
        delete item; // success: a taken item/layout should not be deleted when the old parent is deleted
    else
        QVERIFY(!inner.isNull());
}

void tst_QGridLayout::taskQTBUG_40609_addingWidgetToItsOwnLayout(){
    QWidget widget;
    widget.setObjectName("9bb37ca762aeb7269b8");
    QGridLayout layout(&widget);
    layout.setObjectName("d631e91a35f2b66a6dff35");

    QTest::ignoreMessage(QtWarningMsg, "QLayout: Cannot add a null widget to QGridLayout/d631e91a35f2b66a6dff35");
    layout.addWidget(Q_NULLPTR, 0, 0);
    QCOMPARE(layout.count(), 0);

    QTest::ignoreMessage(QtWarningMsg, "QLayout: Cannot add parent widget QWidget/9bb37ca762aeb7269b8 to its child layout QGridLayout/d631e91a35f2b66a6dff35");
    layout.addWidget(&widget, 0, 0);
    QCOMPARE(layout.count(), 0);
}

void tst_QGridLayout::taskQTBUG_40609_addingLayoutToItself(){
    QWidget widget;
    widget.setObjectName("0373d417fffe2c59c6fe543");
    QGridLayout layout(&widget);
    layout.setObjectName("5d79e1b0aed83f100e3c2");

    QTest::ignoreMessage(QtWarningMsg, "QLayout: Cannot add a null layout to QGridLayout/5d79e1b0aed83f100e3c2");
    layout.addLayout(Q_NULLPTR, 0, 0);
    QCOMPARE(layout.count(), 0);

    QTest::ignoreMessage(QtWarningMsg, "QLayout: Cannot add layout QGridLayout/5d79e1b0aed83f100e3c2 to itself");
    layout.addLayout(&layout, 0, 0);
    QCOMPARE(layout.count(), 0);
}

void tst_QGridLayout::taskQTBUG_52357_spacingWhenItemIsHidden()
{
    QWidget widget;
    setFrameless(&widget);
    QGridLayout layout(&widget);
    layout.setMargin(0);
    layout.setSpacing(5);
    QPushButton button1;
    layout.addWidget(&button1, 0, 0);
    QPushButton button2;
    layout.addWidget(&button2, 0, 1);
    QPushButton button3;
    layout.addWidget(&button3, 0, 2);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    int tempWidth = button1.width() + button2.width() + button3.width() + 2 * layout.spacing();
    button2.hide();
    QTRY_COMPARE_WITH_TIMEOUT(tempWidth, button1.width() + button3.width() + layout.spacing(), 1000);
}

void tst_QGridLayout::replaceWidget()
{
    QWidget wdg;
    QGridLayout *l = new QGridLayout();
    const int itemCount = 9;
    QLabel *labels[itemCount];

    // setup layout
    for (int n = 0; n < itemCount; ++n) {
        int x = n % 3;
        int y = n / 3;
        labels[n] = new QLabel(QLatin1String("label ") + QString::number(n));
        Qt::Alignment align = (n % 3 ? Qt::AlignLeft : Qt::AlignRight);
        l->addWidget(labels[n], x * 3, y * 3, (n % 2) + 1, (n + 1) % 2 + 1, align);
    }
    wdg.setLayout(l);

    // iterate and replace
    for (int n = 0; n < itemCount; n += 2) {
        int i = l->indexOf(labels[n]);
        int fromRow, fromCol, fromRowSpan, fromColSpan;
        l->getItemPosition(i, &fromRow, &fromCol, &fromRowSpan, &fromColSpan);
        Qt::Alignment fromAlign = l->itemAt(i)->alignment();
        // do replace
        QPushButton *pb = new QPushButton("replaced");
        QLayoutItem *olditem = l->replaceWidget(labels[n], pb);
        // verify
        QCOMPARE(i, l->indexOf(pb));
        QVERIFY(olditem != 0);
        QCOMPARE(l->indexOf(labels[n]), -1);
        int toRow, toCol, toRowSpan, toColSpan;
        l->getItemPosition(i, &toRow, &toCol, &toRowSpan, &toColSpan);
        QCOMPARE(fromRow, toRow);
        QCOMPARE(fromCol, toCol);
        QCOMPARE(fromRowSpan, toRowSpan);
        QCOMPARE(fromColSpan, toColSpan);
        Qt::Alignment toAlign = l->itemAt(i)->alignment();
        QCOMPARE(fromAlign, toAlign);
        // clean up
        olditem->widget()->deleteLater();
        delete olditem;
    }
}

void tst_QGridLayout::dontCrashWhenExtendsToEnd()
{
    QWidget window;
    window.resize(320,200);
    QWidget parent(&window);
    QLabel *lbl0 = new QLabel(QLatin1String("lbl0:"));
    QLabel *lbl1 = new QLabel(QLatin1String("lbl1:"));
    QPushButton *pb = new QPushButton(QLatin1String("pb1"));
    QGridLayout *l = new QGridLayout(&parent);
    l->addWidget(lbl0, 0, 0);
    l->addWidget(lbl1, 1, 0);
    // adding an item in the bottom right corner than spans to the end (!)...
    l->addWidget(pb, 1, 1, -1, -1);
    // ...should not cause a crash when the items are distributed....
    l->setGeometry(QRect(0, 0, 200, 50));    // DONT CRASH HERE
}

QTEST_MAIN(tst_QGridLayout)
#include "tst_qgridlayout.moc"
