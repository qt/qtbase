// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


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

#include <QtCore/qscopeguard.h>

namespace {
    class LayoutWithContent : public QGridLayout
    {
    public:
        LayoutWithContent(QWidget *parentWdg) : QGridLayout(parentWdg)
        {
            QWidget *minMaxSizeWdg = new QWidget();
            minMaxSizeWdg->setMinimumSize(40, 40);
            minMaxSizeWdg->setMaximumSize(140, 140);
            QLabel *label1 = new QLabel(QStringLiteral("This is a qt label 1"));
            QLabel *label2 = new QLabel(QStringLiteral("This is a qt label 2"));
            addWidget(minMaxSizeWdg, 0, 0);
            label1->setMaximumHeight(100);
            label2->setMaximumHeight(100);
            label1->setMaximumWidth(100);
            label2->setMaximumWidth(100);
            addWidget(label1, 1, 0);
            addWidget(label2, 0, 1);
        }

        void applySizeConstraint(SizeConstraint sc)
        {
            setSizeConstraint(sc);
            activate();
        }

        void applyVerticalSizeConstraint(SizeConstraint sc)
        {
            setVerticalSizeConstraint(sc);
            activate();
        }

        void applyHorizontalSizeConstraint(SizeConstraint sc)
        {
            setHorizontalSizeConstraint(sc);
            activate();
        }

        void clearConstraintAndResize(QSize sz, Qt::Orientations o = Qt::Horizontal | Qt::Vertical)
        {
            if (o == (Qt::Horizontal | Qt::Vertical))
                setSizeConstraint(QLayout::SizeConstraint::SetNoConstraint);
            else if (o == Qt::Vertical)
                setVerticalSizeConstraint(QLayout::SizeConstraint::SetNoConstraint);
            else if (o == Qt::Horizontal)
                setHorizontalSizeConstraint(QLayout::SizeConstraint::SetNoConstraint);

            // Make sure we can always remove any set constraint as they are not removed by a change of the constraint.
            parentWidget()->setMinimumSize(0, 0);
            parentWidget()->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
            parentWidget()->resize(sz);
            activate();
        }
    };
}

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
    void sizeConstraints();

    void checkVerticalSizeConstraint_data();
    void checkVerticalSizeConstraint();
    void checkHorizontalSizeConstraint_data();
    void checkHorizontalSizeConstraint();
private:
    void setupDataSizeConstraint();
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
#ifdef Q_OS_MACOS
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
    std::unique_ptr<QWidget> w(new QWidget);
    layout.addWidget(w.get());
    QCOMPARE(layout.count(), 1);
    layout.removeWidget(w.get());
    QCOMPARE(layout.count(), 0);

    QPointer<QLayout> childLayout(new QHBoxLayout);
    layout.addLayout(childLayout);
    QCOMPARE(layout.count(), 1);

    layout.removeWidget(nullptr);
    QCOMPARE(layout.count(), 1);

    layout.removeItem(childLayout);
    const auto reaper = qScopeGuard([&] { delete childLayout; });
    QCOMPARE(layout.count(), 0);

    QVERIFY(!childLayout.isNull());

    // Test inactive layout consumes ChildRemoved event (QTBUG-124151)
    layout.addWidget(w.get());
    layout.setEnabled(false);
    w.reset();
    layout.setEnabled(true);
}

void tst_QLayout::sizeConstraints()
{
    QWidget windowWdg;
    auto *layout = new LayoutWithContent(&windowWdg);

    // Check fixed size
    layout->applySizeConstraint(QLayout::SizeConstraint::SetFixedSize);
    QCOMPARE(windowWdg.size(), layout->totalSizeHint());
    layout->clearConstraintAndResize(QSize(100, 100));

    const QSize verySmall(5, 5);
    const QSize veryBig(1500, 1500);

    // Check minimumSize changes
    layout->clearConstraintAndResize(verySmall);
    layout->applySizeConstraint(QLayout::SizeConstraint::SetMinimumSize);
    QCOMPARE(windowWdg.size(), layout->minimumSize());

    // Check no-max on minimumSize changes
    layout->clearConstraintAndResize(veryBig);
    layout->applySizeConstraint(QLayout::SizeConstraint::SetMinimumSize);
    QCOMPARE(windowWdg.size(), veryBig);

    // Check maximumSize
    layout->clearConstraintAndResize(veryBig);
    layout->applySizeConstraint(QLayout::SizeConstraint::SetMaximumSize);
    QCOMPARE(windowWdg.size(), layout->maximumSize());

    // Check no-max on minimumSize changes
    layout->clearConstraintAndResize(verySmall);
    layout->applySizeConstraint(QLayout::SizeConstraint::SetMaximumSize);
    QCOMPARE(windowWdg.size(), verySmall);

    // Check min on MinMax
    layout->clearConstraintAndResize(verySmall);
    layout->applySizeConstraint(QLayout::SizeConstraint::SetMinAndMaxSize);
    QCOMPARE(windowWdg.size(), layout->minimumSize());

    // Check max on MinMax
    layout->clearConstraintAndResize(veryBig);
    layout->applySizeConstraint(QLayout::SizeConstraint::SetMinAndMaxSize);
    QCOMPARE(windowWdg.size(), layout->maximumSize());

    // Default size constaint with window
    layout->clearConstraintAndResize(verySmall);
    layout->applySizeConstraint(QLayout::SizeConstraint::SetDefaultConstraint);
    QCOMPARE(windowWdg.size(), layout->totalMinimumSize());

    // Default size constaint without window.
    // It can appear weird that the default constraint removes earlier constraints in
    // non-window mode when nothing similar happens for other constraints.
    // So this part could *maybe* be a subject for a change.
    // However, we need to be extremely careful as about every widget application
    // uses layouts.
    QWidget windowWidget;
    QHBoxLayout *topLayout = new QHBoxLayout(&windowWidget);
    QWidget *innerWidget = new QWidget();
    topLayout->addWidget(innerWidget);
    auto *layout2 = new LayoutWithContent(innerWidget);
    // make sure we set a minimumSize (that shouldn't be explicit) here.
    layout2->applySizeConstraint(QLayout::SizeConstraint::SetMinimumSize);
    QCOMPARE(innerWidget->minimumSize(), layout2->totalMinimumSize());
    // Only set here as we actually like to keep the size we had from minimumSize
    layout2->setSizeConstraint(QLayout::SizeConstraint::SetDefaultConstraint);
    QCOMPARE(innerWidget->minimumSize(), layout2->totalMinimumSize());
    layout2->activate();
    QCOMPARE(innerWidget->minimumSize(), QSize(0, 0));
}

void tst_QLayout::setupDataSizeConstraint()
{
    QTest::addColumn<QLayout::SizeConstraint>("constraint");

    const auto constraints = {
        QLayout::SetFixedSize,
        QLayout::SetMinimumSize,
        QLayout::SetMaximumSize,
        QLayout::SetMinAndMaxSize,
        QLayout::SetDefaultConstraint,
        QLayout::SetNoConstraint
    };

    for (auto c : constraints) {
        const QString desc = QLatin1String("Constraint_") + QTest::toString(c);
        QTest::addRow("%s", qPrintable(desc)) << c;
    }
}

void tst_QLayout::checkVerticalSizeConstraint_data()
{
    setupDataSizeConstraint();
}

void tst_QLayout::checkVerticalSizeConstraint()
{
    QFETCH(QLayout::SizeConstraint, constraint);

    QWidget windowWdg;
    auto *layout = new LayoutWithContent(&windowWdg);
    layout->setHorizontalSizeConstraint(constraint);

    // Check fixed size
    layout->applyVerticalSizeConstraint(QLayout::SetFixedSize);
    QCOMPARE(windowWdg.height(), layout->totalSizeHint().height());
    layout->clearConstraintAndResize(QSize(100, 100), Qt::Vertical);

    const QSize verySmall(5, 5);
    const QSize veryBig(1500, 1500);

    layout->clearConstraintAndResize(verySmall, Qt::Vertical);
    layout->applyVerticalSizeConstraint(QLayout::SetMinimumSize);
    QCOMPARE(windowWdg.size().height(), layout->minimumSize().height());

    layout->clearConstraintAndResize(veryBig, Qt::Vertical);
    layout->applyVerticalSizeConstraint(QLayout::SetMinimumSize);
    QCOMPARE(windowWdg.size().height(), veryBig.height());

    layout->clearConstraintAndResize(veryBig, Qt::Vertical);
    layout->applyVerticalSizeConstraint(QLayout::SetMaximumSize);
    QCOMPARE(windowWdg.size().height(), layout->maximumSize().height());

    layout->clearConstraintAndResize(verySmall, Qt::Vertical);
    layout->applyVerticalSizeConstraint(QLayout::SetMaximumSize);
    QCOMPARE(windowWdg.size().height(), verySmall.height());

    layout->clearConstraintAndResize(verySmall, Qt::Vertical);
    layout->applyVerticalSizeConstraint(QLayout::SetMinAndMaxSize);
    QCOMPARE(windowWdg.size().height(), layout->minimumSize().height());

    layout->clearConstraintAndResize(veryBig, Qt::Vertical);
    layout->applyVerticalSizeConstraint(QLayout::SetMinAndMaxSize);
    QCOMPARE(windowWdg.size().height(), layout->maximumSize().height());

    layout->clearConstraintAndResize(verySmall, Qt::Vertical);
    layout->applyVerticalSizeConstraint(QLayout::SetDefaultConstraint);
    QCOMPARE(windowWdg.size().height(), layout->totalMinimumSize().height());

    // Inner widget check
    QWidget windowWidget;
    QHBoxLayout *topLayout = new QHBoxLayout(&windowWidget);
    QWidget *innerWidget = new QWidget();
    topLayout->addWidget(innerWidget);
    auto *layout2 = new LayoutWithContent(innerWidget);
    layout2->applyVerticalSizeConstraint(QLayout::SetMinimumSize);
    QCOMPARE(innerWidget->minimumSize().height(), layout2->totalMinimumSize().height());
    layout2->setVerticalSizeConstraint(QLayout::SetDefaultConstraint);
    QCOMPARE(innerWidget->minimumSize().height(), layout2->totalMinimumSize().height());
    layout2->activate();
    QCOMPARE(innerWidget->minimumSize().height(), 0);
}

void tst_QLayout::checkHorizontalSizeConstraint_data()
{
    setupDataSizeConstraint();
}

void tst_QLayout::checkHorizontalSizeConstraint()
{
    QFETCH(QLayout::SizeConstraint, constraint);

    QWidget windowWdg;
    auto *layout = new LayoutWithContent(&windowWdg);
    layout->setVerticalSizeConstraint(constraint);

    // Fixed size
    layout->applyHorizontalSizeConstraint(QLayout::SetFixedSize);
    QCOMPARE(windowWdg.width(), layout->totalSizeHint().width());
    layout->clearConstraintAndResize(QSize(100, 100), Qt::Horizontal);

    const QSize verySmall(5, 5);
    const QSize veryBig(1500, 1500);

    // Minimum size
    layout->clearConstraintAndResize(verySmall, Qt::Horizontal);
    layout->applyHorizontalSizeConstraint(QLayout::SetMinimumSize);
    QCOMPARE(windowWdg.size().width(), layout->minimumSize().width());

    layout->clearConstraintAndResize(veryBig, Qt::Horizontal);
    layout->applyHorizontalSizeConstraint(QLayout::SetMinimumSize);
    QCOMPARE(windowWdg.size().width(), veryBig.width());

    // Maximum size
    layout->clearConstraintAndResize(veryBig, Qt::Horizontal);
    layout->applyHorizontalSizeConstraint(QLayout::SetMaximumSize);
    QCOMPARE(windowWdg.size().width(), layout->maximumSize().width());

    layout->clearConstraintAndResize(verySmall, Qt::Horizontal);
    layout->applyHorizontalSizeConstraint(QLayout::SetMaximumSize);
    QCOMPARE(windowWdg.size().width(), verySmall.width());

    // Min and Max
    layout->clearConstraintAndResize(verySmall, Qt::Horizontal);
    layout->applyHorizontalSizeConstraint(QLayout::SetMinAndMaxSize);
    QCOMPARE(windowWdg.size().width(), layout->minimumSize().width());

    layout->clearConstraintAndResize(veryBig, Qt::Horizontal);
    layout->applyHorizontalSizeConstraint(QLayout::SetMinAndMaxSize);
    QCOMPARE(windowWdg.size().width(), layout->maximumSize().width());

    // Default constraint
    layout->clearConstraintAndResize(verySmall, Qt::Horizontal);
    layout->applyHorizontalSizeConstraint(QLayout::SetDefaultConstraint);
    QCOMPARE(windowWdg.size().width(), layout->totalMinimumSize().width());

    // Final innerWidget check
    QWidget windowWidget;
    QHBoxLayout *topLayout = new QHBoxLayout(&windowWidget);
    QWidget *innerWidget = new QWidget();
    topLayout->addWidget(innerWidget);
    auto *layout2 = new LayoutWithContent(innerWidget);
    layout2->applyHorizontalSizeConstraint(QLayout::SetMinimumSize);
    QCOMPARE(innerWidget->minimumSize().width(), layout2->totalMinimumSize().width());
    layout2->setHorizontalSizeConstraint(QLayout::SetDefaultConstraint);
    QCOMPARE(innerWidget->minimumSize().width(), layout2->totalMinimumSize().width());
    layout2->activate();
    QCOMPARE(innerWidget->minimumSize().width(), 0);
}

QTEST_MAIN(tst_QLayout)
#include "tst_qlayout.moc"
