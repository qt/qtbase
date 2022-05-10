// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>

#include <qcoreapplication.h>
#include <qmetaobject.h>
#include <qdebug.h>
#include <qboxlayout.h>
#include <qmenubar.h>
#include <qdialog.h>
#include <qsizegrip.h>
#include <qlabel.h>
#include <QtWidgets/QFrame>
#include <QtWidgets/QStyleFactory>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QComboBox>
#include <QPushButton>
#include <QRadioButton>
#include <private/qlayoutengine_p.h>

#include <QtTest/private/qtesthelpers_p.h>

using namespace QTestPrivate;

class tst_QLayout : public QObject
{
Q_OBJECT

public:
    tst_QLayout();
    virtual ~tst_QLayout();

private slots:
    void cleanup() { QVERIFY(QApplication::topLevelWidgets().isEmpty()); }
    void getSetCheck();
    void geometry();
    void smartMaxSize();
    void setLayoutBugs();
    void setContentsMargins();
    void layoutItemRect();
    void warnIfWrongParent();
    void controlTypes();
    void controlTypes2();
    void adjustSizeShouldMakeSureLayoutIsActivated();
    void testRetainSizeWhenHidden();
    void removeWidget();
};

tst_QLayout::tst_QLayout()
{
}

tst_QLayout::~tst_QLayout()
{
}

// Testing get/set functions
void tst_QLayout::getSetCheck()
{
    QBoxLayout obj1(QBoxLayout::LeftToRight);
    // QWidget * QLayout::menuBar()
    // void QLayout::setMenuBar(QWidget *)
    QMenuBar *var1 = new QMenuBar();
    obj1.setMenuBar(var1);
    QCOMPARE(static_cast<QWidget *>(var1), obj1.menuBar());
    obj1.setMenuBar((QWidget *)0);
    QCOMPARE((QWidget *)0, obj1.menuBar());
    delete var1;
}

class SizeHinterFrame : public QFrame
{
public:
    SizeHinterFrame(const QSize &sh, const QSize &msh = QSize())
    : QFrame(0), sh(sh), msh(msh) {
        setFrameStyle(QFrame::Box | QFrame::Plain);
    }



    void setSizeHint(const QSize &s) { sh = s; }
    QSize sizeHint() const override { return sh; }
    QSize minimumSizeHint() const override { return msh; }

private:
    QSize sh;
    QSize msh;
};


void tst_QLayout::geometry()
{
    // For QWindowsStyle we know that QWidgetItem::geometry() and QWidget::geometry()
    // should be the same.
    QApplication::setStyle(QStyleFactory::create(QLatin1String("Windows")));
    QWidget topLevel;
    setFrameless(&topLevel);
    QWidget w(&topLevel);
    QVBoxLayout layout(&w);
    SizeHinterFrame widget(QSize(100,100));
    layout.addWidget(&widget);
    QLayoutItem *item = layout.itemAt(0);
    topLevel.show();
    QApplication::processEvents();
    QCOMPARE(item->geometry().size(), QSize(100,100));

    widget.setMinimumSize(QSize(110,110));
    QCOMPARE(item->geometry().size(), QSize(110,110));

    widget.setMinimumSize(QSize(0,0));
    widget.setMaximumSize(QSize(90,90));
    widget.setSizeHint(QSize(100,100));
    QCOMPARE(item->geometry().size(), QSize(90,90));
}

void tst_QLayout::smartMaxSize()
{
    QList<int> expectedWidths;

    QFile f(QFINDTESTDATA("baseline/smartmaxsize"));

    QCOMPARE(f.open(QIODevice::ReadOnly | QIODevice::Text), true);

    QTextStream stream(&f);

    while(!stream.atEnd()) {
        QString line = stream.readLine(200);
        expectedWidths.append(line.section(QLatin1Char(' '), 6, -1, QString::SectionSkipEmpty).toInt());
    }
    f.close();

    int sizeCombinations[] = { 0, 10, 20, QWIDGETSIZE_MAX};
    QSizePolicy::Policy policies[] = {  QSizePolicy::Fixed,
                                        QSizePolicy::Minimum,
                                        QSizePolicy::Maximum,
                                        QSizePolicy::Preferred,
                                        QSizePolicy::Expanding,
                                        QSizePolicy::MinimumExpanding,
                                        QSizePolicy::Ignored
                                        };
    Qt::Alignment alignments[] = {  Qt::Alignment{},
                                    Qt::AlignLeft,
                                    Qt::AlignRight,
                                    Qt::AlignHCenter
                                    };

    int expectedIndex = 0;
    int regressionCount = 0;
    for (size_t p = 0; p < sizeof(policies)/sizeof(QSizePolicy::Policy); ++p) {
        QSizePolicy sizePolicy;
        sizePolicy.setHorizontalPolicy(policies[p]);
        for (size_t min = 0; min < sizeof(sizeCombinations)/sizeof(int); ++min) {
            int minSize = sizeCombinations[min];
            for (size_t max = 0; max < sizeof(sizeCombinations)/sizeof(int); ++max) {
                int maxSize = sizeCombinations[max];
                for (size_t sh = 0; sh < sizeof(sizeCombinations)/sizeof(int); ++sh) {
                    int sizeHint = sizeCombinations[sh];
                    for (size_t a = 0; a < sizeof(alignments)/sizeof(int); ++a) {
                        Qt::Alignment align = alignments[a];
                        QSize sz = qSmartMaxSize(QSize(sizeHint, 1), QSize(minSize, 1), QSize(maxSize, 1), sizePolicy, align);
                        int width = sz.width();
                        int expectedWidth = expectedWidths[expectedIndex];
                        if (width != expectedWidth) {
                            qDebug() << "error at index" << expectedIndex << ':' << sizePolicy.horizontalPolicy() << align << minSize << sizeHint << maxSize << width;
                            ++regressionCount;
                        }
                        ++expectedIndex;
                    }
                }
            }
        }
    }
    QCOMPARE(regressionCount, 0);
}

void tst_QLayout::setLayoutBugs()
{
    QWidget widget(0);
    QHBoxLayout *hBoxLayout = new QHBoxLayout(&widget);

    for(int i = 0; i < 6; ++i) {
        QPushButton *pushButton = new QPushButton("Press me!", &widget);
        hBoxLayout->addWidget(pushButton);
    }

    widget.setLayout(hBoxLayout);
    QCOMPARE(widget.layout(), hBoxLayout);

    QWidget containerWidget(0);
    containerWidget.setLayout(widget.layout());
    QVERIFY(!widget.layout());
    QCOMPARE(containerWidget.layout(), hBoxLayout);
}

class MyLayout : public QLayout
{
    public:
        MyLayout() : invalidated(false) {}
        virtual void invalidate() override {invalidated = true;}
        bool invalidated;
        QSize sizeHint() const override {return QSize();}
        void addItem(QLayoutItem*) override {}
        QLayoutItem* itemAt(int) const override {return 0;}
        QLayoutItem* takeAt(int) override {return 0;}
        int count() const override {return 0;}
};

void tst_QLayout::setContentsMargins()
{
    MyLayout layout;
    layout.invalidated = false;
    int left, top, right, bottom;

    layout.setContentsMargins(52, 53, 54, 55);
    QVERIFY(layout.invalidated);
    layout.invalidated = false;

    layout.getContentsMargins(&left, &top, &right, &bottom);
    QCOMPARE(left, 52);
    QCOMPARE(top, 53);
    QCOMPARE(right, 54);
    QCOMPARE(bottom, 55);

    layout.setContentsMargins(52, 53, 54, 55);
    QVERIFY(!layout.invalidated);

    MyLayout otherLayout; // with default contents margins
    QVERIFY(layout.contentsMargins() != otherLayout.contentsMargins());
    layout.unsetContentsMargins();
    QCOMPARE(layout.contentsMargins(), otherLayout.contentsMargins());

    layout.setContentsMargins(10, 20, 30, 40);
    QVERIFY(layout.contentsMargins() != otherLayout.contentsMargins());

    int contentsMarginsPropertyIndex = QLayout::staticMetaObject.indexOfProperty("contentsMargins");
    QVERIFY(contentsMarginsPropertyIndex >= 0);
    QMetaProperty contentsMarginsProperty = QLayout::staticMetaObject.property(contentsMarginsPropertyIndex);
    QVERIFY(contentsMarginsProperty.isValid());
    QVERIFY(contentsMarginsProperty.isResettable());
    QVERIFY(contentsMarginsProperty.reset(&layout));
    QCOMPARE(layout.contentsMargins(), otherLayout.contentsMargins());
}

class EventReceiver : public QObject
{
public:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event->type() == QEvent::Show) {
            geom = static_cast<QWidget*>(watched)->geometry();
        }
        return false;
    }
    QRect geom;
};

void tst_QLayout::layoutItemRect()
{
#ifdef Q_OS_MAC
    if (QApplication::style()->inherits("QMacStyle")) {
        QWidget *window = new QWidget;
        QRadioButton *radio = new QRadioButton(window);
        QWidgetItem item(radio);
        EventReceiver eventReceiver;
        radio->installEventFilter(&eventReceiver);

        radio->show();
        QApplication::processEvents();
        QApplication::processEvents();
        QSize s = item.sizeHint();

        item.setAlignment(Qt::AlignVCenter);
        item.setGeometry(QRect(QPoint(0, 0), s));

        QCOMPARE(radio->geometry().size(), radio->sizeHint());
        delete radio;
    }
#endif
}

void tst_QLayout::warnIfWrongParent()
{
    QWidget root;
    QHBoxLayout lay;
    lay.setParent(&root);
    QTest::ignoreMessage(QtWarningMsg, "QLayout::parentWidget: A layout can only have another layout as a parent.");
    QCOMPARE(lay.parentWidget(), nullptr);
}

void tst_QLayout::controlTypes()
{
    QVBoxLayout layout;
    QCOMPARE(layout.controlTypes(), QSizePolicy::DefaultType);
    QSizePolicy p;
    QCOMPARE(p.controlType(),QSizePolicy::DefaultType);
}

void tst_QLayout::controlTypes2()
{
    QWidget main;
    QVBoxLayout *const layout = new QVBoxLayout(&main);
    layout->setContentsMargins(0, 0, 0, 0);
    QComboBox *combo = new QComboBox(&main);
    layout->addWidget(combo);
    QCOMPARE(layout->controlTypes(), QSizePolicy::ComboBox);
}

void tst_QLayout::adjustSizeShouldMakeSureLayoutIsActivated()
{
    QWidget main;

    QVBoxLayout *const layout = new QVBoxLayout(&main);
    layout->setContentsMargins(0, 0, 0, 0);
    SizeHinterFrame *frame = new SizeHinterFrame(QSize(200, 10), QSize(200, 8));
    frame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    layout->addWidget(frame);

    SizeHinterFrame *frame2 = new SizeHinterFrame(QSize(200, 10), QSize(200, 8));
    frame2->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    layout->addWidget(frame2);

    main.show();

    frame2->hide();
    main.adjustSize();
    QCOMPARE(main.size(), QSize(200, 10));
}

void tst_QLayout::testRetainSizeWhenHidden()
{
#ifdef Q_OS_ANDROID
    QSKIP("Test does not work on platforms which default to showMaximized()");
#endif

    QWidget widget;
    QBoxLayout layout(QBoxLayout::TopToBottom, &widget);

    QLabel *label1 = new QLabel("label1 text", &widget);
    layout.addWidget(label1);
    QLabel *label2 = new QLabel("label2 text", &widget);
    layout.addWidget(label2);

    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    int normalHeight = widget.height();

    // a. Verify that a removed visible will mean lesser size after adjust
    label1->hide();
    widget.adjustSize();
    int heightWithoutLabel1 = widget.height();
    QVERIFY(heightWithoutLabel1 < normalHeight);

    // b restore with verify that the size is the same
    label1->show();
    QCOMPARE(widget.sizeHint().height(), normalHeight);

    // c verify that a policy with retainSizeWhenHidden is respected
    QSizePolicy sp_remove = label1->sizePolicy();
    QSizePolicy sp_retain = label1->sizePolicy();
    sp_retain.setRetainSizeWhenHidden(true);

    label1->setSizePolicy(sp_retain);
    label1->hide();
    QCOMPARE(widget.sizeHint().height(), normalHeight);

    // d check that changing the policy to not wanting size will result in lesser size
    label1->setSizePolicy(sp_remove);
    QCOMPARE(widget.sizeHint().height(), heightWithoutLabel1);

    // e verify that changing back the hidden widget to want the hidden size will ensure that it gets more size
    label1->setSizePolicy(sp_retain);
    QCOMPARE(widget.sizeHint().height(), normalHeight);
}

void tst_QLayout::removeWidget()
{
    QHBoxLayout layout;
    QCOMPARE(layout.count(), 0);
    QWidget w;
    layout.addWidget(&w);
    QCOMPARE(layout.count(), 1);
    layout.removeWidget(&w);
    QCOMPARE(layout.count(), 0);

    QPointer<QLayout> childLayout(new QHBoxLayout);
    layout.addLayout(childLayout);
    QCOMPARE(layout.count(), 1);

    layout.removeWidget(nullptr);
    QCOMPARE(layout.count(), 1);

    layout.removeItem(childLayout);
    QCOMPARE(layout.count(), 0);

    QVERIFY(!childLayout.isNull());
}

QTEST_MAIN(tst_QLayout)
#include "tst_qlayout.moc"
