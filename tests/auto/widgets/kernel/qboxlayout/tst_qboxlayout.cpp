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
#include <QtGui>
#include <QtWidgets>

#include <QtTest/private/qtesthelpers_p.h>

using namespace QTestPrivate;

class tst_QBoxLayout : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();
    void insertSpacerItem();
    void insertLayout();
    void sizeHint();
    void sizeConstraints();
    void setGeometry();
    void setStyleShouldChangeSpacing();

    void testLayoutEngine_data();
    void testLayoutEngine();

    void taskQTBUG_7103_minMaxWidthNotRespected();
    void taskQTBUG_27420_takeAtShouldUnparentLayout();
    void taskQTBUG_40609_addingWidgetToItsOwnLayout();
    void taskQTBUG_40609_addingLayoutToItself();
    void replaceWidget();
};

class CustomLayoutStyle : public QProxyStyle
{
    Q_OBJECT
public:
    CustomLayoutStyle() : QProxyStyle(QStyleFactory::create("windows"))
    {
        hspacing = 5;
        vspacing = 10;
    }

    virtual int pixelMetric(PixelMetric metric, const QStyleOption * option = 0,
                            const QWidget * widget = 0 ) const;

    int hspacing;
    int vspacing;
};

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

void tst_QBoxLayout::cleanup()
{
    QVERIFY(QApplication::topLevelWidgets().isEmpty());
}

void tst_QBoxLayout::insertSpacerItem()
{
    QWidget window;
    window.setWindowTitle(QTest::currentTestFunction());

    QSpacerItem *spacer1 = new QSpacerItem(20, 10, QSizePolicy::Expanding, QSizePolicy::Expanding);
    QSpacerItem *spacer2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Expanding);

    QBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(new QLineEdit("Foooooooooooooooooooooooooo"));
    layout->addSpacerItem(spacer1);
    layout->addWidget(new QLineEdit("Baaaaaaaaaaaaaaaaaaaaaaaaar"));
    layout->insertSpacerItem(0, spacer2);
    window.setLayout(layout);

    QCOMPARE(layout->itemAt(0), spacer2);
    QCOMPARE(layout->itemAt(2), spacer1);

    window.show();
}

void tst_QBoxLayout::insertLayout()
{
    QWidget window;
    QVBoxLayout *vbox = new QVBoxLayout(&window);
    QScopedPointer<QVBoxLayout> dummyParentLayout(new QVBoxLayout);
    QHBoxLayout *subLayout = new QHBoxLayout;
    dummyParentLayout->addLayout(subLayout);
    QCOMPARE(subLayout->parent(), dummyParentLayout.data());
    QCOMPARE(dummyParentLayout->count(), 1);

    // add subLayout to another layout
    QTest::ignoreMessage(QtWarningMsg, "QLayout::addChildLayout: layout \"\" already has a parent");
    vbox->addLayout(subLayout);
    QCOMPARE((subLayout->parent() == vbox), (vbox->count() == 1));
}


void tst_QBoxLayout::sizeHint()
{
    QWidget window;
    window.setWindowTitle(QTest::currentTestFunction());
    QHBoxLayout *lay1 = new QHBoxLayout;
    QHBoxLayout *lay2 = new QHBoxLayout;
    QLabel *label = new QLabel("widget twooooooooooooooooooooooooooooooooooooooooooooooooooooooo");
    lay2->addWidget(label);
    lay1->addLayout(lay2);
    window.setLayout(lay1);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    label->setText("foooooooo baaaaaaar");
    QSize sh = lay1->sizeHint();
    QApplication::processEvents();
    // Note that this is not strictly required behaviour - actually
    // the preferred behaviour would be that sizeHint returns
    // the same value regardless of what's lying in the event queue.
    // (i.e. we would check for equality here instead)
    QVERIFY(lay1->sizeHint() != sh);
}

void tst_QBoxLayout::sizeConstraints()
{
    QWidget window;
    window.setWindowTitle(QTest::currentTestFunction());
    QHBoxLayout *lay = new QHBoxLayout;
    lay->addWidget(new QLabel("foooooooooooooooooooooooooooooooooooo"));
    lay->addWidget(new QLabel("baaaaaaaaaaaaaaaaaaaaaaaaaaaaaar"));
    lay->setSizeConstraint(QLayout::SetFixedSize);
    window.setLayout(lay);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QSize sh = window.sizeHint();
    delete lay->takeAt(1);
    QVERIFY(sh.width() >= window.sizeHint().width() &&
            sh.height() >= window.sizeHint().height());

}

void tst_QBoxLayout::setGeometry()
{
    QWidget toplevel;
    toplevel.setWindowTitle(QTest::currentTestFunction());
    setFrameless(&toplevel);
    QWidget w(&toplevel);
    QVBoxLayout *lay = new QVBoxLayout;
    lay->setMargin(0);
    lay->setSpacing(0);
    QHBoxLayout *lay2 = new QHBoxLayout;
    QDial *dial = new QDial;
    lay2->addWidget(dial);
    lay2->setAlignment(Qt::AlignTop);
    lay2->setAlignment(Qt::AlignRight);
    lay->addLayout(lay2);
    w.setLayout(lay);
    toplevel.show();

    QRect newGeom(0, 0, 70, 70);
    lay2->setGeometry(newGeom);
    QVERIFY2(newGeom.contains(dial->geometry()), "dial->geometry() should be smaller and within newGeom");
}

void tst_QBoxLayout::setStyleShouldChangeSpacing()
{

    QWidget window;
    window.setWindowTitle(QTest::currentTestFunction());
    QHBoxLayout *hbox = new QHBoxLayout(&window);
    QPushButton *pb1 = new QPushButton(tr("The spacing between this"));
    QPushButton *pb2 = new QPushButton(tr("and this button should depend on the style of the parent widget"));;
    pb1->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    pb2->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    hbox->addWidget(pb1);
    hbox->addWidget(pb2);
    QScopedPointer<CustomLayoutStyle> style1(new CustomLayoutStyle);
    style1->hspacing = 6;
    window.setStyle(style1.data());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    auto spacing = [&]() { return pb2->geometry().left() - pb1->geometry().right() - 1; };
    QCOMPARE(spacing(), 6);

    QScopedPointer<CustomLayoutStyle> style2(new CustomLayoutStyle());
    style2->hspacing = 10;
    window.setStyle(style2.data());
    QTRY_COMPARE(spacing(), 10);
}

void tst_QBoxLayout::taskQTBUG_7103_minMaxWidthNotRespected()
{
    QLabel *label = new QLabel("Qt uses standard C++, but makes extensive use of the C pre-processor to enrich the language. Qt can also be used in several other programming languages via language bindings. It runs on all major platforms, and has extensive internationalization support. Non-GUI features include SQL database access, XML parsing, thread management, network support and a unified cross-platform API for file handling.");
    label->setWordWrap(true);
    label->setFixedWidth(200);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding));

    QWidget widget;
    widget.setWindowTitle(QTest::currentTestFunction());
    widget.setLayout(layout);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    int height = label->height();

    QRect g = widget.geometry();
    g.setWidth(600);
    widget.setGeometry(g);

    QTest::qWait(50);

    QCOMPARE(label->height(), height);
}

void tst_QBoxLayout::taskQTBUG_27420_takeAtShouldUnparentLayout()
{
    QSharedPointer<QHBoxLayout> outer(new QHBoxLayout);
    QPointer<QVBoxLayout> inner = new QVBoxLayout;

    outer->addLayout(inner);
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

void tst_QBoxLayout::taskQTBUG_40609_addingWidgetToItsOwnLayout(){
    QWidget widget;
    widget.setWindowTitle(QTest::currentTestFunction());
    widget.setObjectName("347b469225a24a0ef05150a");
    QVBoxLayout layout(&widget);
    layout.setObjectName("ef9e2b42298e0e6420105bb");

    QTest::ignoreMessage(QtWarningMsg, "QLayout: Cannot add a null widget to QVBoxLayout/ef9e2b42298e0e6420105bb");
    layout.addWidget(nullptr);
    QCOMPARE(layout.count(), 0);

    QTest::ignoreMessage(QtWarningMsg, "QLayout: Cannot add parent widget QWidget/347b469225a24a0ef05150a to its child layout QVBoxLayout/ef9e2b42298e0e6420105bb");
    layout.addWidget(&widget);
    QCOMPARE(layout.count(), 0);
}

void tst_QBoxLayout::taskQTBUG_40609_addingLayoutToItself(){
    QWidget widget;
    widget.setWindowTitle(QTest::currentTestFunction());
    widget.setObjectName("fe44e5cb6c08006597126a");
    QVBoxLayout layout(&widget);
    layout.setObjectName("cc751dd0f50f62b05a62da");

    QTest::ignoreMessage(QtWarningMsg, "QLayout: Cannot add a null layout to QVBoxLayout/cc751dd0f50f62b05a62da");
    layout.addLayout(nullptr);
    QCOMPARE(layout.count(), 0);

    QTest::ignoreMessage(QtWarningMsg, "QLayout: Cannot add layout QVBoxLayout/cc751dd0f50f62b05a62da to itself");
    layout.addLayout(&layout);
    QCOMPARE(layout.count(), 0);
}

struct Descr
{
    Descr(int min, int sh, int max = -1, bool exp= false, int _stretch = 0, bool _empty = false)
        :minimumSize(min), sizeHint(sh), maximumSize(max < 0 ? QLAYOUTSIZE_MAX : max),
         expanding(exp), stretch(_stretch), empty(_empty)
        {}

    int minimumSize;
    int sizeHint;
    int maximumSize;
    bool expanding;

    int stretch;

    bool empty;
};


typedef QList<Descr> DescrList;
Q_DECLARE_METATYPE(DescrList);

typedef QList<int> SizeList;
typedef QList<int> PosList;



class LayoutItem : public QLayoutItem
{
public:
    LayoutItem(const Descr &descr) :m_descr(descr) {}

    QSize sizeHint() const { return QSize(m_descr.sizeHint, 100); }
    QSize minimumSize() const { return QSize(m_descr.minimumSize, 0); }
    QSize maximumSize() const { return QSize(m_descr.maximumSize, QLAYOUTSIZE_MAX); }
    Qt::Orientations expandingDirections() const
        { return m_descr.expanding ? Qt::Horizontal :  Qt::Orientations(0); }
    void setGeometry(const QRect &r) { m_pos = r.x(); m_size = r.width();}
    QRect geometry() const { return QRect(m_pos, 0, m_size, 100); }
    bool isEmpty() const { return m_descr.empty; }

private:
    Descr m_descr;
    int m_pos;
    int m_size;
};

void tst_QBoxLayout::testLayoutEngine_data()
{
    // (int min, int sh, int max = -1, bool exp= false, int _stretch = 0, bool _empty = false)
    QTest::addColumn<DescrList>("itemDescriptions");
    QTest::addColumn<int>("size");
    QTest::addColumn<int>("spacing");
    QTest::addColumn<PosList>("expectedPositions");
    QTest::addColumn<SizeList>("expectedSizes");

    QTest::newRow("Just one")
        << (DescrList() << Descr(0, 100))
        << 200
        << 0
        << (PosList() << 0)
        << (SizeList() << 200);

    QTest::newRow("Two non-exp")
        << (DescrList() << Descr(0, 100) << Descr(0,100))
        << 400
        << 0
        << (PosList() << 0 << 200)
        << (SizeList() << 200 << 200);

    QTest::newRow("Exp + non-exp")
        << (DescrList() << Descr(0, 100, -1, true) << Descr(0,100))
        << 400
        << 0
        << (PosList() << 0 << 300)
        << (SizeList() << 300 << 100);


    QTest::newRow("Stretch")
        << (DescrList() << Descr(0, 100, -1, false, 1) << Descr(0,100, -1, false, 2))
        << 300
        << 0
        << (PosList() << 0 << 100)
        << (SizeList() << 100 << 200);


    QTest::newRow("Spacing")
        << (DescrList() << Descr(0, 100) << Descr(0,100))
        << 400
        << 10
        << (PosList() << 0 << 205)
        << (SizeList() << 195 << 195);


    QTest::newRow("Less than minimum")
        << (DescrList() << Descr(100, 100, 100, false) << Descr(50, 100, 100, false))
        << 100
        << 0
        << (PosList() << 0 << 50)
        << (SizeList() << 50 << 50);


    QTest::newRow("Less than sizehint")
        << (DescrList() << Descr(100, 200, 100, false) << Descr(50, 200, 100, false))
        << 200
        << 0
        << (PosList() << 0 << 100)
        << (SizeList() << 100 << 100);

    QTest::newRow("Too much space")
        << (DescrList() << Descr(0, 100, 100, false) << Descr(0, 100, 100, false))
        << 500
        << 0
        << (PosList() << 100 << 300)
        << (SizeList() << 100 << 100);

    QTest::newRow("Empty")
        << (DescrList() << Descr(0, 100, 100) << Descr(0,0,-1, false, 0, true) << Descr(0, 100, 100) )
        << 500
        << 0
        << (PosList() << 100 << 300 << 300)
        << (SizeList() << 100 << 0 << 100);

    QTest::newRow("QTBUG-33104")
        << (DescrList() << Descr(11, 75, 75, true) << Descr(75, 75))
        << 200
        << 0
        << (PosList() << 0 << 75)
        << (SizeList() << 75 << 125);

    QTest::newRow("Expanding with maximumSize")
        << (DescrList() << Descr(11, 75, 100, true) << Descr(75, 75))
        << 200
        << 0
        << (PosList() << 0 << 100)
        << (SizeList() << 100 << 100);

    QTest::newRow("Stretch with maximumSize")
        << (DescrList() << Descr(11, 75, 100, false, 1) << Descr(75, 75))
        << 200
        << 0
        << (PosList() << 0 << 100)
        << (SizeList() << 100 << 100);

    QTest::newRow("Stretch with maximumSize last")
        << (DescrList()  << Descr(75, 75) << Descr(11, 75, 100, false, 1))
        << 200
        << 0
        << (PosList() << 0 << 100)
        << (SizeList() << 100 << 100);
}

void tst_QBoxLayout::testLayoutEngine()
{
    QFETCH(DescrList, itemDescriptions);
    QFETCH(int, size);
    QFETCH(int, spacing);
    QFETCH(PosList, expectedPositions);
    QFETCH(SizeList, expectedSizes);

    QHBoxLayout box;
    box.setSpacing(spacing);
    int i;
    for (i = 0; i < itemDescriptions.count(); ++i) {
         Descr descr = itemDescriptions.at(i);
         LayoutItem *li = new LayoutItem(descr);
         box.addItem(li);
         box.setStretch(i, descr.stretch);
    }
    box.setGeometry(QRect(0,0,size,100));
    for (i = 0; i < expectedSizes.count(); ++i) {
        int xSize = expectedSizes.at(i);
        int xPos = expectedPositions.at(i);
        QLayoutItem *item = box.itemAt(i);
        QCOMPARE(item->geometry().width(), xSize);
        QCOMPARE(item->geometry().x(), xPos);
    }
}

void tst_QBoxLayout::replaceWidget()
{
    QWidget w;
    QBoxLayout *boxLayout = new QVBoxLayout(&w);

    QLineEdit *replaceFrom = new QLineEdit;
    QLineEdit *replaceTo = new QLineEdit;
    boxLayout->addWidget(new QLineEdit());
    boxLayout->addWidget(replaceFrom);
    boxLayout->addWidget(new QLineEdit());

    QCOMPARE(boxLayout->indexOf(replaceFrom), 1);
    QCOMPARE(boxLayout->indexOf(replaceTo), -1);
    delete boxLayout->replaceWidget(replaceFrom, replaceTo);

    QCOMPARE(boxLayout->indexOf(replaceFrom), -1);
    QCOMPARE(boxLayout->indexOf(replaceTo), 1);
}

QTEST_MAIN(tst_QBoxLayout)
#include "tst_qboxlayout.moc"
