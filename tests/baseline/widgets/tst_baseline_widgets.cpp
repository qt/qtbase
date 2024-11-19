// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qbaselinetest.h>
#include <qwidgetbaselinetest.h>
#include <QtWidgets>
#include <QStyleOptionSlider>

class tst_Widgets : public QWidgetBaselineTest
{
    Q_OBJECT

public:
    tst_Widgets() = default;

private slots:
    void tst_QSlider_data();
    void tst_QSlider();

    void tst_QPushButton_data();
    void tst_QPushButton();

    void tst_QPushButtonSquare();

    void tst_QProgressBar_data();
    void tst_QProgressBar();

    void tst_QSpinBox_data();
    void tst_QSpinBox();

    void tst_QDoubleSpinBox_data();
    void tst_QDoubleSpinBox();

    void tst_QDateTimeEdit_data();
    void tst_QDateTimeEdit();

    void tst_QTimeEdit_data();
    void tst_QTimeEdit();

    void tst_QDateEdit_data();
    void tst_QDateEdit();

    void tst_QDial_data();
    void tst_QDial();

    void tst_QCheckbox_data();
    void tst_QCheckbox();

    void tst_QRadioButton_data();
    void tst_QRadioButton();

    void tst_QScrollBar_data();
    void tst_QScrollBar();

    void tst_QTabBar_data();
    void tst_QTabBar();

    void tst_QTabWidget_data();
    void tst_QTabWidget();

    void tst_QListView_data();
    void tst_QListView();

    void tst_QTableView_data();
    void tst_QTableView();

    void tst_QTreeView_data();
    void tst_QTreeView();

    void tst_QLineEdit_data();
    void tst_QLineEdit();

    void tst_QMenu_data();
    void tst_QMenu();

    void tst_QCombobox_data();
    void tst_QCombobox();

    void tst_QComboboxDelegate_data();
    void tst_QComboboxDelegate();

    void tst_QCommandLinkButton_data();
    void tst_QCommandLinkButton();

    void tst_QLCDNumber_data();
    void tst_QLCDNumber();

private:

    // Abstract SpinBox test for QSpinBox, QDoubleSpinBox, QDateTimeEdit, QDateEdit, QTimeEdit
    void tst_SpinBox_data();
    void tst_SpinBox(QAbstractSpinBox* spinBox);

    // 78 standard icons from 6.3
    const int numberStandardIcons = 78;

    // recursive methods for QTreeView population
    void tst_QTreeView_populateTree(QStandardItem* node, int height, int itemsPerNode, bool hasIcon);
    QStandardItem* tst_QTreeView_populateItem(int height, int number, bool hasIcon);
};

void tst_Widgets::tst_QSlider_data()
{
    QTest::addColumn<Qt::Orientation>("orientation");
    QTest::addColumn<QSlider::TickPosition>("tickPosition");

    QBaselineTest::newRow("horizontal") << Qt::Horizontal << QSlider::NoTicks;
    QBaselineTest::newRow("horizontal ticks above") << Qt::Horizontal << QSlider::TicksAbove;
    QBaselineTest::newRow("horizontal ticks below") << Qt::Horizontal << QSlider::TicksBelow;
    QBaselineTest::newRow("horizontal ticks both") << Qt::Horizontal << QSlider::TicksBothSides;
    QBaselineTest::newRow("vertical") << Qt::Vertical << QSlider::NoTicks;
    QBaselineTest::newRow("vertical ticks left") << Qt::Vertical << QSlider::TicksLeft;
    QBaselineTest::newRow("vertical ticks right") << Qt::Vertical << QSlider::TicksRight;
    QBaselineTest::newRow("vertical ticks both") << Qt::Vertical << QSlider::TicksBothSides;
}

void tst_Widgets::tst_QSlider()
{
    struct PublicSlider : QSlider { friend tst_Widgets; };
    QFETCH(Qt::Orientation, orientation);
    QFETCH(QSlider::TickPosition, tickPosition);

    QBoxLayout *box = new QBoxLayout(orientation == Qt::Horizontal ? QBoxLayout::TopToBottom
                                                                   : QBoxLayout::LeftToRight);
    QList<QSlider*> _sliders;
    for (int i = 0; i < 3; ++i) {
        QSlider *slider = new QSlider;
        slider->setOrientation(orientation);
        slider->setTickPosition(tickPosition);
        _sliders << slider;
        box->addWidget(slider);
    }
    const auto sliders = _sliders;

    testWindow()->setLayout(box);

    // we want to see sliders with different values
    int value = 0;
    for (const auto &slider : sliders)
        slider->setValue(value += 33);

    takeStandardSnapshots();

    PublicSlider *slider = static_cast<PublicSlider*>(sliders.first());
    QStyleOptionSlider sliderOptions;
    slider->initStyleOption(&sliderOptions);
    const QRect handleRect = slider->style()->subControlRect(QStyle::CC_Slider, &sliderOptions,
                                                             QStyle::SubControl::SC_SliderHandle, slider);
    QTest::mousePress(slider, Qt::LeftButton, {}, handleRect.center());
    QBASELINE_CHECK(takeSnapshot(), "pressed");
    QTest::mouseRelease(slider, Qt::LeftButton, {}, handleRect.center());
    QBASELINE_CHECK(takeSnapshot(), "released");

    slider->setSliderDown(true);
    QBASELINE_CHECK(takeSnapshot(), "down");

    sliders.first()->setSliderDown(false);
    QBASELINE_CHECK(takeSnapshot(), "notdown");
}

void tst_Widgets::tst_QPushButton_data()
{
    QTest::addColumn<bool>("isFlat");

    QBaselineTest::newRow("normal") << false;
    QBaselineTest::newRow("flat") << true;
}

void tst_Widgets::tst_QPushButton()
{
    QFETCH(bool, isFlat);

    QVBoxLayout *vbox = new QVBoxLayout;
    QPushButton *testButton = new QPushButton("Ok");
    testButton->setFlat(isFlat);
    vbox->addWidget(testButton);

    testWindow()->setLayout(vbox);
    takeStandardSnapshots();

    testButton->setDown(true);
    QBASELINE_CHECK(takeSnapshot(), "down");
    testButton->setDown(false);
    QBASELINE_CHECK(takeSnapshot(), "up");

    testButton->setDefault(true);
    QBASELINE_CHECK(takeSnapshot(), "default_up");
    testButton->setDown(true);
    QBASELINE_CHECK(takeSnapshot(), "default_down");
    testButton->setDown(false);
}

void tst_Widgets::tst_QPushButtonSquare()
{
    QVBoxLayout layout;

    QPushButton button(testWindow());
    button.setText(QLatin1String("Square"));
    const auto sizeHint = button.sizeHint().width();
    // Depending on the current QStyle, this may result in
    // a different button look - on macOS it will look as
    // a toolbutton:
    button.setFixedSize(sizeHint, sizeHint);

    layout.addWidget(&button);
    testWindow()->setLayout(&layout);

    takeStandardSnapshots();

    button.setCheckable(true);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "square_unchecked");
    button.setChecked(true);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "square_checked");
}

void tst_Widgets::tst_QProgressBar_data()
{
    QTest::addColumn<Qt::Orientation>("orientation");
    QTest::addColumn<bool>("invertedAppearance");
    QTest::addColumn<bool>("textVisible");

    QTest::newRow("vertical_normalAppearance_textVisible") << Qt::Vertical << false << true;
    QTest::newRow("vertical_invertedAppearance_textVisible") << Qt::Vertical << true << true;
    QTest::newRow("horizontal_normalAppearance_textVisible") << Qt::Horizontal << false << true;
    QTest::newRow("horizontal_invertedAppearance_textVisible") << Qt::Horizontal << true << true;
    QTest::newRow("vertical_normalAppearance_textNotVisible") << Qt::Vertical << false << false;
    QTest::newRow("vertical_invertedAppearance_textNotVisible") << Qt::Vertical << true << false;
    QTest::newRow("horizontal_normalAppearance_textNotVisible") << Qt::Horizontal << false << false;
    QTest::newRow("horizontal_invertedAppearance_textNotVisible") << Qt::Horizontal << true << false;
}

void tst_Widgets::tst_QProgressBar()
{
    QFETCH(Qt::Orientation, orientation);
    QFETCH(bool, invertedAppearance);
    QFETCH(bool, textVisible);

    QBoxLayout box((orientation == Qt::Vertical) ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom);

    for (int i = 0; i < 4; ++i) {
        QProgressBar *bar = new QProgressBar(testWindow());
        bar->setOrientation(orientation);
        bar->setInvertedAppearance(invertedAppearance);
        bar->setTextVisible(textVisible);
        bar->setValue(i * 33);
        box.addWidget(bar);
    }

    testWindow()->setLayout(&box);
    takeStandardSnapshots();
}

void tst_Widgets::tst_SpinBox_data()
{
    QTest::addColumn<QAbstractSpinBox::ButtonSymbols>("buttons");

    QTest::addRow("NoButtons") << QAbstractSpinBox::NoButtons;
    QTest::addRow("UpDownArrows") << QAbstractSpinBox::UpDownArrows;
    QTest::addRow("PlusMinus") << QAbstractSpinBox::PlusMinus;
}

void tst_Widgets::tst_SpinBox(QAbstractSpinBox *spinBox)
{
    QFETCH(const QAbstractSpinBox::ButtonSymbols, buttons);

    spinBox->setButtonSymbols(buttons);
    spinBox->setMinimumWidth(200);

    QVBoxLayout layout;
    layout.addWidget(spinBox);
    testWindow()->setLayout(&layout);

    takeStandardSnapshots();

    spinBox->setAlignment(Qt::AlignHCenter);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "alignCenter");

    spinBox->setAlignment(Qt::AlignRight);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "alignRight");

    // Press / release up button
    QStyleOptionSpinBox styleOption;
    styleOption.initFrom(spinBox);
    QPoint clickTarget = spinBox->style()->subControlRect(QStyle::CC_SpinBox,&styleOption,
                                                 QStyle::SC_SpinBoxUp,spinBox).center();

    QTest::mousePress(spinBox, Qt::LeftButton, Qt::KeyboardModifiers(), clickTarget);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "upPressed");
    QTest::mouseRelease(spinBox, Qt::LeftButton, Qt::KeyboardModifiers(), clickTarget);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "upReleased");

    // Press / release down button
    clickTarget = spinBox->style()->subControlRect(QStyle::CC_SpinBox,&styleOption,
                                           QStyle::SC_SpinBoxDown,spinBox).center();

    QTest::mousePress(spinBox, Qt::LeftButton, Qt::KeyboardModifiers(), clickTarget);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "downPressed");
    QTest::mouseRelease(spinBox, Qt::LeftButton, Qt::KeyboardModifiers(), clickTarget);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "downReleased");
}

void tst_Widgets::tst_QSpinBox_data()
{
    tst_SpinBox_data();
}

void tst_Widgets::tst_QSpinBox()
{
    QSpinBox spinBox;
    tst_SpinBox(&spinBox);
}

void tst_Widgets::tst_QDoubleSpinBox_data()
{
    tst_SpinBox_data();
}

void tst_Widgets::tst_QDoubleSpinBox()
{
    QDoubleSpinBox spinBox;
    tst_SpinBox(&spinBox);
}

void tst_Widgets::tst_QDateTimeEdit_data()
{
    tst_SpinBox_data();
}

void tst_Widgets::tst_QDateTimeEdit()
{
    QDateTimeEdit edit;
    tst_SpinBox(&edit);

    // show calendar popup
    QStyleOptionSpinBox styleOption;
    styleOption.initFrom(&edit);
    const QRect buttonUp = edit.style()->subControlRect(QStyle::CC_SpinBox,&styleOption,
                                                        QStyle::SC_SpinBoxUp,&edit);

    // no rect for popup button => use bottom center of up-button
    QPoint clickTarget = buttonUp.center();
    clickTarget.setY(buttonUp.bottomLeft().y());
    edit.setCalendarPopup(true);
    QTest::mouseClick(&edit, Qt::LeftButton, Qt::KeyboardModifiers(), clickTarget);
    QCalendarWidget* calendar = edit.calendarWidget();
    QVERIFY(calendar);
    QVBoxLayout layout;
    layout.addWidget(calendar);
    testWindow()->setLayout(&layout);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "showCalendar");
}

void tst_Widgets::tst_QTimeEdit_data()
{
    tst_SpinBox_data();
}

void tst_Widgets::tst_QTimeEdit()
{
    QTimeEdit edit;
    tst_SpinBox(&edit);
}

void tst_Widgets::tst_QDateEdit_data()
{
    tst_SpinBox_data();
}

void tst_Widgets::tst_QDateEdit()
{
    QDateEdit edit;
    tst_SpinBox(&edit);
}

void tst_Widgets::tst_QDial_data()
{
    QTest::addColumn<int>("minimum");
    QTest::addColumn<int>("maximum");
    QTest::addColumn<bool>("notchesVisible");
    QTest::addColumn<qreal>("notchTarget");

    QTest::newRow("0..99_notches") << 0 << 99 << true << 3.7;
    QTest::newRow("0..99_noNotches") << 0 << 99 << false << 3.7;
    QTest::newRow("1..100_notches") << 1 << 100 << true << 5.7;
    QTest::newRow("1..100_noNotches") << 1 << 100 << false << 3.7;
    QTest::newRow("1..5_notches") << 1 << 5 << true << 8.7;
    QTest::newRow("1..5_noNotches") << 1 << 5 << false << 3.7;
}

void tst_Widgets::tst_QDial()
{
    QFETCH(int, minimum);
    QFETCH(int, maximum);
    QFETCH(bool, notchesVisible);
    QFETCH(qreal, notchTarget);

    QVERIFY(maximum > minimum);
    const int steps = maximum - minimum;

    QDial dial(testWindow());
    dial.setMinimum(minimum);
    dial.setMaximum(maximum);
    dial.setNotchTarget(notchTarget);
    dial.setSliderPosition(minimum + (steps / 2));
    dial.setNotchesVisible(notchesVisible);

    QBoxLayout box(QBoxLayout::LeftToRight);
    box.addWidget(&dial);
    testWindow()->setLayout(&box);
    takeStandardSnapshots();
}

void tst_Widgets::tst_QCheckbox_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("hasIcon");
    QTest::addColumn<bool>("isTriState");

    QTest::newRow("SimpleCheckbox") << "" << false << false;
    QTest::newRow("SimpleCheckboxWithIcon") << "" << true << false;
    QTest::newRow("SimpleCheckboxWithText") << "checkBox" << false << false;
    QTest::newRow("SimpleCheckboxWithTextAndIcon") << "checkBox with icon" << true << false;
    QTest::newRow("SimpleTristate") << "" << false << true;
    QTest::newRow("SimpleTristateWithText") << "tristateBox" << false << true;
}

void tst_Widgets::tst_QCheckbox()
{
    QFETCH(QString, text);
    QFETCH(bool, hasIcon);
    QFETCH(bool, isTriState);

    class CheckBox : public QCheckBox
    {
    public:
        using QCheckBox::initStyleOption;
    };

    QBoxLayout layout(QBoxLayout::TopToBottom);
    CheckBox box;
    box.setTristate(isTriState);

    if (!text.isEmpty())
        box.setText(text);

    if (hasIcon)
        box.setIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));

    layout.addWidget(&box);
    testWindow()->setLayout(&layout);
    takeStandardSnapshots();

    do {
        const Qt::CheckState checkState = box.checkState();
        QStyleOptionButton styleOption;
        box.initStyleOption(&styleOption);
        const QPoint clickTarget = box.style()->subElementRect(QStyle::SE_CheckBoxClickRect, &styleOption, &box).center();

        const std::array titles = {"unChecked", "partiallyChecked", "checked"};
        const QString snapShotTitle = titles[checkState];

        QTest::mousePress(&box,Qt::MouseButton::LeftButton, Qt::KeyboardModifiers(), clickTarget,0);
        QVERIFY(box.isDown());
        QBASELINE_CHECK_DEFERRED(takeSnapshot(), (snapShotTitle + "_pressed").toLocal8Bit().constData());

        QTest::mouseRelease(&box,Qt::MouseButton::LeftButton, Qt::KeyboardModifiers(), clickTarget,0);
        QVERIFY(!box.isDown());
        QVERIFY(checkState != box.checkState());
        QBASELINE_CHECK_DEFERRED(takeSnapshot(), (snapShotTitle + "_released").toLocal8Bit().constData());

    } while (box.checkState() != Qt::Unchecked);
}

void tst_Widgets::tst_QRadioButton_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("hasIcon");

    QTest::newRow("SimpleRadioButton") << "" << false;
    QTest::newRow("RadioButtonWithText") << "RadioButton" << false;
    QTest::newRow("SimpleRadioButtonWithIcon") << "" << true;
    QTest::newRow("RadioButtonWithTextAndIcon") << "RadioButton" << true;
}

void tst_Widgets::tst_QRadioButton()
{
    QFETCH(QString,text);
    QFETCH(bool,hasIcon);

    class RadioButton : public QRadioButton
    {
    public:
        using QRadioButton::QRadioButton;
        using QRadioButton::initStyleOption;
    };

    RadioButton button1(testWindow());

    if (!text.isEmpty())
        button1.setText(text);

    if (hasIcon)
        button1.setIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));

    button1.setChecked(false);

    RadioButton button2(testWindow());

    if (!text.isEmpty())
        button2.setText(text);

    if (hasIcon)
        button2.setIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));

    // button2 has to start checked for the following tests to work
    button2.setChecked(true);

    QBoxLayout box(QBoxLayout::TopToBottom);
    box.addWidget(&button1);
    box.addWidget(&button2);
    testWindow()->setLayout(&box);
    takeStandardSnapshots();

    QStyleOptionButton styleOption;
    button1.initStyleOption(&styleOption);
    const QPoint clickTarget = button1.style()->subElementRect(QStyle::SE_RadioButtonClickRect, &styleOption, &button1).center();

    QTest::mousePress(&button1,Qt::MouseButton::LeftButton, Qt::KeyboardModifiers(), clickTarget,0);
    QVERIFY(button1.isDown());
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "pressUnchecked");
    QTest::mouseRelease(&button1,Qt::MouseButton::LeftButton, Qt::KeyboardModifiers(), clickTarget,0);
    QVERIFY(!button1.isDown());

    // button1 has grabbed the check from button2
    QVERIFY(button1.isChecked());
    QVERIFY(!button2.isChecked());
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "releaseUnchecked");

    // press and release checked button1 again
    QTest::mousePress(&button1,Qt::MouseButton::LeftButton, Qt::KeyboardModifiers(), clickTarget,0);
    QVERIFY(button1.isDown());
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "pressChecked");
    QTest::mouseRelease(&button1,Qt::MouseButton::LeftButton, Qt::KeyboardModifiers(), clickTarget,0);
    QVERIFY(!button1.isDown());

    // checkstate not supposed to change
    QVERIFY(button1.isChecked());
    QVERIFY(!button2.isChecked());
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "releaseChecked");
}

void tst_Widgets::tst_QScrollBar_data()
{
    QTest::addColumn<Qt::Orientation>("orientation");

    QTest::newRow("Horizontal") << Qt::Horizontal;
    QTest::newRow("Vertical") << Qt::Vertical;
}

void tst_Widgets::tst_QScrollBar()
{
    QFETCH(Qt::Orientation, orientation);

    QBoxLayout box((orientation == Qt::Vertical) ? QBoxLayout::LeftToRight
                                                 : QBoxLayout::TopToBottom);
    QList<QScrollBar*> bars;
    for (int i = 0; i < 4; ++i) {

        QScrollBar *bar = new QScrollBar(testWindow());
        (orientation == Qt::Vertical) ? bar->setFixedHeight(100)
                                      : bar->setFixedWidth(100);

        bar->setOrientation(orientation);
        bar->setValue(i * 33);
        box.addWidget(bar);
        bars.append(bar);
    }

    testWindow()->setLayout(&box);
    takeStandardSnapshots();

    // press left/up of first bar
    QScrollBar *bar = bars.at(0);
    QStyleOptionSlider styleOption = qt_qscrollbarStyleOption(bar);
    QPoint clickTarget = bar->style()->subControlRect(QStyle::CC_ScrollBar, &styleOption,
                                                      QStyle::SC_ScrollBarSubLine, bar).center();
    QTest::mousePress(bar,Qt::MouseButton::LeftButton, Qt::KeyboardModifiers(), clickTarget,0);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "pressLeftUp");
    QTest::mouseRelease(bar,Qt::MouseButton::LeftButton, Qt::KeyboardModifiers(), clickTarget,0);

    // press slider of first bar
    styleOption = qt_qscrollbarStyleOption(bar);
    clickTarget = bar->style()->subControlRect(QStyle::CC_ScrollBar, &styleOption,
                                               QStyle::SC_ScrollBarSlider, bar).center();
    QTest::mousePress(bar,Qt::MouseButton::LeftButton, Qt::KeyboardModifiers(), clickTarget,0);
    QVERIFY(bar->isSliderDown());
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "pressSlider");
    QTest::mouseRelease(bar,Qt::MouseButton::LeftButton, Qt::KeyboardModifiers(), clickTarget,0);

    // Press AddPage up on first bar
    clickTarget = bar->style()->subControlRect(QStyle::CC_ScrollBar, &styleOption,
                                               QStyle::SC_ScrollBarAddPage, bar).center();
    QTest::mousePress(bar,Qt::MouseButton::LeftButton, Qt::KeyboardModifiers(), clickTarget,0);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "pressAddPage");
    QTest::mouseRelease(bar,Qt::MouseButton::LeftButton, Qt::KeyboardModifiers(), clickTarget,0);

    // press SubPage of last bar
    bar = bars.at(3);
    styleOption = qt_qscrollbarStyleOption(bar);
    clickTarget = bar->style()->subControlRect(QStyle::CC_ScrollBar, &styleOption,
                                               QStyle::SC_ScrollBarAddLine, bar).center();
    QTest::mousePress(bar,Qt::MouseButton::LeftButton, Qt::KeyboardModifiers(), clickTarget,0);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "pressRightDown");
    QTest::mouseRelease(bar,Qt::MouseButton::LeftButton, Qt::KeyboardModifiers(), clickTarget,0);
}

void tst_Widgets::tst_QTabBar_data()
{
    QTest::addColumn<QTabBar::Shape>("shape");
    QTest::addColumn<int>("numberTabs");
    QTest::addColumn<int>("fixedWidth");
    QTest::addColumn<bool>("isClosable");

    // fixedWidth <0 will be interpreted as variable width
    QTest::newRow("RoundedNorth_3_variableWidth") << QTabBar::RoundedNorth << 3 << -1 << false;
    QTest::newRow("RoundedEast_3_variableWidth") << QTabBar::RoundedEast << 3 << -1 << false;
    QTest::newRow("RoundedWest_3_variableWidth") << QTabBar::RoundedWest << 3 << -1 << false;
    QTest::newRow("RoundedSouth_3_variableWidth") << QTabBar::RoundedSouth << 3 << -1 << false;
    QTest::newRow("RoundedNorth_20_fixedWidth") << QTabBar::RoundedNorth << 20 << 250 << true;
}

void tst_Widgets::tst_QTabBar()
{
    QFETCH(QTabBar::Shape, shape);
    QFETCH(int, numberTabs);
    QFETCH(int, fixedWidth);
    QFETCH(bool, isClosable);

    QTabBar bar (testWindow());
    bar.setShape(shape);
    bar.setTabsClosable(isClosable);
    if (fixedWidth > 0)
        bar.setFixedWidth(fixedWidth);

    for (int i = 0; i < numberTabs; ++i) {
        bar.insertTab(i,"Tab_" + QString::number(i));
    }

    QBoxLayout box(QBoxLayout::LeftToRight, testWindow());
    box.addWidget(&bar);
    testWindow()->setLayout(&box);

    takeStandardSnapshots();

    // press/release first tab
    bar.setCurrentIndex(0);
    QPoint clickTarget = bar.tabRect(0).center();
    QTest::mousePress(&bar,Qt::MouseButton::LeftButton, Qt::KeyboardModifiers(), clickTarget,0);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "pressFirstTab");
    QTest::mouseRelease(&bar,Qt::MouseButton::LeftButton, Qt::KeyboardModifiers(), clickTarget,0);
    QVERIFY(bar.currentIndex() == 0);

    // press/release second tab if it exists
    if (bar.count() > 1) {
        clickTarget = bar.tabRect(1).center();
        QTest::mousePress(&bar,Qt::MouseButton::LeftButton, Qt::KeyboardModifiers(), clickTarget,0);
        QBASELINE_CHECK_DEFERRED(takeSnapshot(), "pressSecondTab");
        QTest::mouseRelease(&bar,Qt::MouseButton::LeftButton, Qt::KeyboardModifiers(), clickTarget,0);
        QVERIFY(bar.currentIndex() == 1);
    }

    // test press/release on close button
    if (isClosable) {

        // CloseButton is either left or right
        QWidget *leftButton = bar.tabButton(bar.currentIndex(),QTabBar::ButtonPosition::LeftSide);
        QWidget *rightButton = bar.tabButton(bar.currentIndex(),QTabBar::ButtonPosition::RightSide);
        QAbstractButton *button = qobject_cast<QAbstractButton*>(leftButton);
        if (button == nullptr)
            button = qobject_cast<QAbstractButton*>(rightButton);

        if (button != nullptr) {
            clickTarget = button->rect().center();
            QTest::mousePress(button,Qt::MouseButton::LeftButton,
                              Qt::KeyboardModifiers(), clickTarget,0);
            QBASELINE_CHECK_DEFERRED(takeSnapshot(), "pressCloseFirstTab");
            QTest::mouseRelease(button,Qt::MouseButton::LeftButton,
                                Qt::KeyboardModifiers(), clickTarget,0);
            QBASELINE_CHECK_DEFERRED(takeSnapshot(), "releaseCloseFirstTab");
        }
    }
}

void tst_Widgets::tst_QTabWidget_data()
{
    QTest::addColumn<QTabWidget::TabPosition>("tabPosition");
    QTest::addColumn<int>("numberTabs");
    QTest::addColumn<QString>("tabText");
    QTest::addColumn<int>("fixedWidth");
    QTest::addColumn<bool>("isClosable");
    QTest::addColumn<bool>("isDocumentMode");

    // fixedWidth <0 will be interpreted as variable width
    QTest::newRow("North_3_variableWidthDocMode") << QTabWidget::North << 3 << "This is a tab text" << -1 << false << true;
    QTest::newRow("East_3_variableWidth") << QTabWidget::East << 3 << "This is a tab text" << -1 << false << false;
    QTest::newRow("West_3_variableWidthDocMode") << QTabWidget::West << 3 << "This is a tab text" << -1 << false << true;
    QTest::newRow("South_3_variableWidth") << QTabWidget::South << 3 << "This is a tab text" << -1 << true << false;
    QTest::newRow("North_20_fixedWidthDocMode") << QTabWidget::North << 20
                                         << "This is a very long text to actually force wrapping!" << 100 << true << true;
    QTest::newRow("South_20_variableWidth") << QTabWidget::South << 20
                                         << "This is a very long text to actually force wrapping!" << -1 << false << false;
}

void tst_Widgets::tst_QTabWidget()
{
    QFETCH(QTabWidget::TabPosition, tabPosition);
    QFETCH(int, numberTabs);
    QFETCH(QString, tabText);
    QFETCH(int, fixedWidth);
    QFETCH(bool, isClosable);
    QFETCH(bool, isDocumentMode);

    QTabWidget tabWidget (testWindow());
    if (fixedWidth > 0)
        tabWidget.setFixedWidth(fixedWidth);
    tabWidget.setTabPosition(tabPosition);
    tabWidget.setTabsClosable(isClosable);
    tabWidget.setDocumentMode(isDocumentMode);

    for (int i = 0; i < numberTabs; ++i) {
        QLabel *tabLabel = new QLabel("Tab number " + QString::number(i) + "\n" + tabText, &tabWidget);
        QBoxLayout *tabBox = new QBoxLayout(QBoxLayout::TopToBottom,&tabWidget);
        tabBox->addWidget(tabLabel);
        tabWidget.insertTab(i,tabLabel,"Tab_" + QString::number(i));
        tabWidget.setCurrentIndex(i);
        tabWidget.currentWidget()->setLayout(tabBox);
    }

    tabWidget.setCurrentIndex(0);
    QBoxLayout box(QBoxLayout::LeftToRight, testWindow());
    box.addWidget(&tabWidget);
    testWindow()->setLayout(&box);
    takeStandardSnapshots();

    // press/release on second tab if it exists
    if (numberTabs > 1) {
        const QPoint clickTarget = tabWidget.tabBar()->tabRect(1).center();
        QTest::mousePress(tabWidget.tabBar(),Qt::MouseButton::LeftButton,Qt::KeyboardModifiers(), clickTarget,0);
        QBASELINE_CHECK_DEFERRED(takeSnapshot(), "pressSecondTab");
        QTest::mouseRelease(tabWidget.tabBar(),Qt::MouseButton::LeftButton, Qt::KeyboardModifiers(), clickTarget,0);
        QVERIFY(tabWidget.currentIndex() == 1);
    }

    // test press/release on close button
    if (isClosable) {

        // CloseButton is either left or right
        QWidget *leftButton = tabWidget.tabBar()->tabButton(tabWidget.currentIndex(),QTabBar::ButtonPosition::LeftSide);
        QWidget *rightButton = tabWidget.tabBar()->tabButton(tabWidget.currentIndex(),QTabBar::ButtonPosition::RightSide);
        QAbstractButton *button = qobject_cast<QAbstractButton*>(leftButton);
        if (button == nullptr)
            button = qobject_cast<QAbstractButton*>(rightButton);

        if (button != nullptr) {
            const QPoint clickTarget = button->rect().center();
            QTest::mousePress(button,Qt::MouseButton::LeftButton,
                              Qt::KeyboardModifiers(), clickTarget,0);
            QBASELINE_CHECK_DEFERRED(takeSnapshot(), "pressCloseTab");
            QTest::mouseRelease(button,Qt::MouseButton::LeftButton,
                                Qt::KeyboardModifiers(), clickTarget,0);
            QBASELINE_CHECK_DEFERRED(takeSnapshot(), "releaseCloseTab");
        }
    }
}

void tst_Widgets::tst_QListView_data()
{
    QTest::addColumn<QListView::ViewMode>("viewMode");
    QTest::addColumn<bool>("isWrapping");
    QTest::addColumn<bool>("hasWordWrap");
    QTest::addColumn<int>("numberItems");
    QTest::addColumn<QSize>("fixedSize");


    // QSize() will be interpreted as variable size
    QTest::newRow("ListModeWrappingNoWordWrapFixed_10") <<
                  QListView::ListMode << true << false << 10 << QSize(100, 500);
    QTest::newRow("ListModeNoWrappingNoWordWrapVariable_20") <<
                  QListView::ListMode << false << true << 20 << QSize();
    QTest::newRow("ListModeNoWrappingWordWrapVariable_30") <<
                  QListView::ListMode << false << true << 30 << QSize();
    QTest::newRow("IconModeNoWrappingNoWordWrapFixed_10") <<
                  QListView::IconMode << false << false << 10 << QSize(100, 500);
    QTest::newRow("IconModeWrappingNoWordWrapVariable_20") <<
                  QListView::IconMode << true << false << 20 << QSize();
    QTest::newRow("IconModeWrappingWordWrapVariable_30") <<
                  QListView::IconMode << true << true << 30 << QSize(100, 500);
}
void tst_Widgets::tst_QListView()
{
    QFETCH(QListView::ViewMode,viewMode);
    QFETCH(bool,isWrapping);
    QFETCH(bool,hasWordWrap);
    QFETCH(int,numberItems);
    QFETCH(QSize,fixedSize);

    QListView listView;
    listView.setViewMode(viewMode);
    listView.setWrapping(isWrapping);
    listView.setWordWrap(hasWordWrap);
    if (fixedSize.isValid())
        listView.setFixedSize(fixedSize);

    QStandardItemModel model(0,1,testWindow());

    // Populate model, add standard icons if required
    const QString itemText = hasWordWrap ? "This is a long text for word wrapping Item_"
                                         : "ListItem_";
    int icon = 0;
    for (int i = 0; i < numberItems; ++i) {
        QStandardItem *item;
        if (viewMode == QListView::IconMode) {
            item = new QStandardItem(QApplication::style()->standardIcon
                   (static_cast<QStyle::StandardPixmap>(icon)), itemText + QString::number(i));
            icon = (icon + 1) % numberStandardIcons;
        } else {
            item = new QStandardItem(itemText + QString::number(i));
        }
        model.appendRow(item);
    }

    listView.setModel(&model);
    QBoxLayout layout(QBoxLayout::LeftToRight, testWindow());
    layout.addWidget(&listView);
    testWindow()->setLayout(&layout);
    takeStandardSnapshots();

    // click on first item
    QPoint clickTarget = listView.visualRect(model.index(0,0)).center();
    QTest::mouseClick(listView.viewport(),Qt::MouseButton::LeftButton,
                      Qt::KeyboardModifiers(), clickTarget,0);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "clickFirstItem");

    // click on scond item
    if (numberItems > 1) {
        clickTarget = listView.visualRect(model.index(1,0)).center();
        QTest::mouseClick(listView.viewport(),Qt::MouseButton::LeftButton,
                            Qt::KeyboardModifiers(), clickTarget,0);
        QBASELINE_CHECK_DEFERRED(takeSnapshot(), "clickSecondItem");
    }

    // Hide first row
    listView.setRowHidden(0,true);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "hideFirstItem");
}

void tst_Widgets::tst_QTableView_data()
{
    QTest::addColumn<bool>("hasHeader");
    QTest::addColumn<bool>("hasRowNumbers");
    QTest::addColumn<bool>("hasWordWrap");
    QTest::addColumn<int>("numberRows");
    QTest::addColumn<int>("numberColumns");
    QTest::addColumn<int>("iconColumn");
    QTest::addColumn<QSize>("fixedSize");

    // QSize() => variable size; iconColumn -1 => no icon
    QTest::newRow("HeaderRowNumWordWrapFixed_10") << true << true << true << 10 << 3 << -1 << QSize(500, 100);
    QTest::newRow("HeaderVariable_20") << true << false << false << 20 << 4 << 1 << QSize();
    QTest::newRow("HeaderFixed_20") << true << false << false << 20 << 4 << 1 << QSize(500, 700);
}

void tst_Widgets::tst_QTableView()
{
    QFETCH(bool, hasHeader);
    QFETCH(bool, hasRowNumbers);
    QFETCH(bool, hasWordWrap);
    QFETCH(int, numberRows);
    QFETCH(int, numberColumns);
    QFETCH(int, iconColumn);
    QFETCH(QSize, fixedSize);

    // Populate model
    int icon = 0;
    QStandardItemModel model(numberRows, numberColumns, testWindow());

    if (hasHeader) {
        for (int i = 0; i < numberColumns; ++i)
            model.setHorizontalHeaderItem(i, new QStandardItem("Header_" + QString::number(i)));
    }

    const QString wrap = hasWordWrap ? "\n long text to wrap words" : "" ;
    for (int row = 0; row < numberRows; ++row) {
        for (int column = 0; column < numberColumns; ++column) {
            QStandardItem *item;
            const QString itemText = QString::number(row) + "/" + QString::number(column) + wrap;
            if (iconColumn == column) {
                item = new QStandardItem(QApplication::style()->standardIcon
                           (static_cast<QStyle::StandardPixmap>(icon)),itemText);

                icon = (icon + 1) % numberStandardIcons;
            } else {
                item = new QStandardItem(itemText);
            }
            model.setItem(row,column,item);
        }
        if (hasRowNumbers)
            model.setVerticalHeaderItem(row, new QStandardItem(QString::number(row)));
    }

    QTableView tableView(testWindow());
    tableView.setWordWrap(hasWordWrap);
    if (fixedSize.isValid())
        tableView.setFixedSize(fixedSize);

    QBoxLayout layout(QBoxLayout::LeftToRight, testWindow());
    tableView.setModel(&model);
    layout.addWidget(&tableView);

    takeStandardSnapshots();

    // Hide grid
    tableView.setShowGrid(false);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "hideGrid");
    tableView.setShowGrid(true);

    // click item 0,0
    QPoint clickTarget = tableView.visualRect(model.index(0,0)).center();
    QTest::mouseClick(tableView.viewport(),Qt::MouseButton::LeftButton,
                      Qt::KeyboardModifiers(), clickTarget,0);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "clickFirstItem");

    // click item 0,1 if it exists
    if (numberColumns > 1) {
        clickTarget = tableView.visualRect(model.index(0,1)).center();
        QTest::mouseClick(tableView.viewport(),Qt::MouseButton::LeftButton,
                          Qt::KeyboardModifiers(), clickTarget,0);
        QBASELINE_CHECK_DEFERRED(takeSnapshot(), "clickSecondItem");
    }

    tableView.clearSelection();

    // Hide first row and column
    tableView.setRowHidden(0, true);
    tableView.setColumnHidden(0, true);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "hideFirstRowColumn");
    tableView.setRowHidden(0, false);
    tableView.setColumnHidden(0, false);

    // Select first row
    tableView.selectRow(0);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "selectFirstRow");

    // Select first column
    tableView.selectColumn(0);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "selectFirstColumn");
}

void tst_Widgets::tst_QTreeView_data()
{
    QTest::addColumn<bool>("showHeader");
    QTest::addColumn<bool>("hasIcons");
    QTest::addColumn<bool>("alternatingRowColors");
    QTest::addColumn<QSize>("fixedSize");
    QTest::addColumn<int>("treeHeight");
    QTest::addColumn<int>("itemsPerNode");

    // QSize() => variable size
    QTest::newRow("HeaderIcons_4_3") << true << true << false << QSize() << 3 << 2;
    QTest::newRow("NoHeaderNoIcons_4_4") << false << false << false << QSize(100, 350) << 3 << 2;
    QTest::newRow("AlternatingRows") << true << true << true << QSize() << 3 << 2;
}

void tst_Widgets::tst_QTreeView()
{
    QFETCH(bool, showHeader);
    QFETCH(bool, hasIcons);
    QFETCH(bool, alternatingRowColors);
    QFETCH(QSize, fixedSize);
    QFETCH(int, treeHeight);
    QFETCH(int, itemsPerNode);
    QVERIFY(treeHeight > 0 && itemsPerNode > 0);

    QTreeView treeView(testWindow());
    fixedSize.isValid()  ? treeView.setFixedSize(fixedSize)
                         : treeView.setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);

    QStandardItemModel model(&treeView);
    showHeader ? model.setHorizontalHeaderItem(0, new QStandardItem("TreeHeader"))
               : treeView.setHeaderHidden(true);

    treeView.setAlternatingRowColors(alternatingRowColors);

    // Populate tree model
    for (int i = 0; i < itemsPerNode; ++i) {
        QStandardItem* root = tst_QTreeView_populateItem(treeHeight, i, hasIcons);
        tst_QTreeView_populateTree(root,treeHeight - 1,itemsPerNode, hasIcons);
        model.appendRow(root);
    }

    treeView.setModel(&model);
    QBoxLayout layout(QBoxLayout::LeftToRight, testWindow());
    layout.addWidget(&treeView);
    testWindow()->setLayout(&layout);

    treeView.expandAll();
    treeView.resizeColumnToContents(0);
    takeStandardSnapshots();

    // Partly expand if possible
    if (treeHeight > 1) {
        treeView.expandToDepth(1);
        QBASELINE_CHECK_DEFERRED(takeSnapshot(), "partlyExpanded");
    }

    // Click on first node
    QPoint clickTarget = treeView.visualRect(model.index(0, 0)).center();
    QTest::mouseClick(treeView.viewport(),Qt::MouseButton::LeftButton,
                      Qt::KeyboardModifiers(), clickTarget, 0);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "clickFirstNode");

    // Hide first row
    treeView.setRowHidden(0, model.index(0, 0), true);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "hideFirstRow");
    treeView.setRowHidden(0, model.index(0, 0), false);

    // Click on second row if it exists
    if (itemsPerNode > 1) {
        clickTarget = treeView.visualRect(model.index(1, 0)).center();
        QTest::mouseClick(treeView.viewport(), Qt::MouseButton::LeftButton,
                          Qt::KeyboardModifiers(), clickTarget, 0);
        QBASELINE_CHECK_DEFERRED(takeSnapshot(), "clickSecondNode");
    }
}

void tst_Widgets::tst_QTreeView_populateTree(QStandardItem* node, int height, int itemsPerNode, bool hasIcon)
{
    QList<QStandardItem*> items;
    for (int i = 0; i < itemsPerNode; ++i) {
        if (height == 0) {
            items.append(tst_QTreeView_populateItem(height, i, hasIcon));
        } else {
            QStandardItem* item = tst_QTreeView_populateItem(height, i, hasIcon);
            tst_QTreeView_populateTree(item, height - 1, itemsPerNode, hasIcon);
            items.append(item);
        }
    }
    return node->appendColumn(items);
}

QStandardItem* tst_Widgets::tst_QTreeView_populateItem(int height, int number, bool hasIcon)
{
    static int icon = 0;
    static int itemCount = 0;

    QStandardItem* item;
    const QString itemText = QString("%1/%2/%3").arg(height).arg(number).arg(itemCount);
    ++itemCount;

    if (hasIcon) {
        item = new QStandardItem(QApplication::style()->standardIcon
               (static_cast<QStyle::StandardPixmap>(icon)), itemText);

        icon = (icon + 1) % numberStandardIcons;
    } else {
        item = new QStandardItem(itemText);
    }
    return item;
}

void tst_Widgets::tst_QLineEdit_data()
{
    QTest::addColumn<bool>("hasFrame");
    QTest::addColumn<QLineEdit::EchoMode>("echoMode");
    QTest::addColumn<QString>("placeHolderText");
    QTest::addColumn<QString>("text");

    QTest::newRow("framePassword") << true << QLineEdit::Password << "password" << "secret";
    QTest::newRow("noFrameCleartext") << false << QLineEdit::Normal << "text" << "this is a text";
}

void tst_Widgets::tst_QLineEdit()
{
    QFETCH(const bool, hasFrame);
    QFETCH(const QLineEdit::EchoMode, echoMode);
    QFETCH(const QString, placeHolderText);
    QFETCH(const QString, text);

    QLineEdit lineEdit(testWindow());
    lineEdit.setFrame(hasFrame);
    lineEdit.setEchoMode(echoMode);
    lineEdit.setPlaceholderText(placeHolderText);

    QHBoxLayout layout;
    layout.addWidget(&lineEdit);
    testWindow()->setLayout(&layout);
    takeStandardSnapshots();

    lineEdit.setText(text);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "setText");

    lineEdit.setAlignment(Qt::AlignRight);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "alignedRight");

    lineEdit.setAlignment(Qt::AlignCenter);
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "alignedCenter");

    lineEdit.setSelection(0,text.size());
    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "textSelected");
}

void tst_Widgets::tst_QMenu_data()
{
    QTest::addColumn<QStringList>("actions");

    const QStringList menu1 = {"Text", "", "TextAndIcon", "", "SubMenu", "", "Checked"};
    QTest::newRow("showMenuPopup") << menu1;
}

void tst_Widgets::tst_QMenu()
{
    QFETCH(const QStringList, actions);

    testWindow()->resize(300, 200);

    QBoxLayout layout(QBoxLayout::TopToBottom);
    QMenu menu1;

    for (const auto& menuItem : actions) {
        if (!menuItem.isEmpty()) {
            if (menuItem == "Text") {
                menu1.addAction(QString("MenuItem"));
                menu1.addAction(QString(""));
            } else if (menuItem == "TextAndIcon") {
                // Using pixmap icon
                QPixmap pix(10, 10);
                pix.fill(Qt::green);
                menu1.addAction(QIcon(pix), QString("MenuWithIcon"));
                menu1.addAction(QIcon(), QString("MenuNoIcon"));
            } else if (menuItem == "SubMenu") {
                QMenu* submenu = menu1.addMenu(QString("&Submenu1"));
                submenu->addAction("SubMenuA");
                submenu->addAction("SubMenuB");
            } else if (menuItem == "Checked") {
                auto checked = menu1.addAction(QString("MenuChecked"));
                checked->setCheckable(true);
                checked->setChecked(true);
                auto notChecked = menu1.addAction(QString("MenuNotChecked"));
                notChecked->setCheckable(true);
                notChecked->setChecked(false);
            }
        } else {
            menu1.addSeparator();
        }
    }

    layout.addWidget(&menu1);
    testWindow()->setLayout(&layout);

    testWindow()->show();
    QVERIFY(QTest::qWaitForWindowExposed(testWindow()));
    QRect testWindowRect(testWindow()->geometry());
    // There can be rounded corners in the window and this leads to test
    // case to be fuzzy. Adjust window rectangle that need to be captured
    int adjustPixel = menu1.geometry().left();
    testWindowRect.adjust(adjustPixel, adjustPixel, -adjustPixel, -adjustPixel);
    QBASELINE_CHECK_DEFERRED(takeScreenSnapshot(testWindowRect), "showitems");

    // Normal menu item with text
    QTest::keyClick(&menu1, Qt::Key_Down);
    QBASELINE_CHECK_DEFERRED(takeScreenSnapshot(testWindowRect), "selectmenutext");
    QTest::keyClick(&menu1, Qt::Key_Down);
    QBASELINE_CHECK_DEFERRED(takeScreenSnapshot(testWindowRect), "selectmenunotext");

    // Menu with icon and text
    QTest::keyClick(&menu1, Qt::Key_Down);
    QBASELINE_CHECK_DEFERRED(takeScreenSnapshot(testWindowRect), "selectmenuwithicon");
    QTest::keyClick(&menu1, Qt::Key_Down);
    QBASELINE_CHECK_DEFERRED(takeScreenSnapshot(testWindowRect), "selectmenuwithnullicon");

    // Sub-menu items
    QTest::keyClick(&menu1, Qt::Key_Down);
    QTest::keyClick(&menu1, Qt::Key_Right);
    QBASELINE_CHECK_DEFERRED(takeScreenSnapshot(testWindowRect), "selectsubmenu");
    QTest::keyClick(&menu1, Qt::Key_Left);

    // Checked menu
    QTest::keyClick(&menu1, Qt::Key_Down);
    QBASELINE_CHECK_DEFERRED(takeScreenSnapshot(testWindowRect), "selectmenuchecked");
    QTest::keyClick(&menu1, Qt::Key_Down);
    QBASELINE_CHECK_DEFERRED(takeScreenSnapshot(testWindowRect), "selectmenunotchecked");
}

void tst_Widgets::tst_QCombobox_data()
{
    QTest::addColumn<bool>("hasFrame");
    QTest::addColumn<bool>("isEditable");

    QTest::addRow("frameNonEditable") << true << false;
    QTest::addRow("frameEditable") << true << true;
    QTest::addRow("noFrameNonEditable") << false << false;
    QTest::addRow("noFrameEditable") << false << true;
}

void tst_Widgets::tst_QCombobox()
{
    QFETCH(const bool, hasFrame);
    QFETCH(const bool, isEditable);

    testWindow()->resize(300, 300);

    QScopedPointer<QComboBox> combobox(new QComboBox(testWindow()));
    QStringList items;
    items << tr("Item1") << tr("Item2") << tr("Item3");
    QStringListModel* itemModel = new QStringListModel(items, this);
    combobox->setModel(itemModel);
    combobox->setFrame(hasFrame);
    combobox->setEditable(isEditable);

    QHBoxLayout layout;
    layout.addWidget(combobox.get());
    testWindow()->setLayout(&layout);
    takeStandardSnapshots();

    QTest::keyClick(combobox.get(), Qt::Key_Down, Qt::AltModifier);
    QBASELINE_CHECK_DEFERRED(takeScreenSnapshot(testWindow()->geometry()), "combobox");
}

void tst_Widgets::tst_QComboboxDelegate_data()
{
    QTest::addColumn<int>("paddingTest");
    QTest::addColumn<int>("widthTest");

    QTest::addRow("padding0") << 2 << 0;
    QTest::addRow("padding20") << 20 << 0;
    QTest::addRow("padding50") << 50 << 0;
    QTest::addRow("width0") << 2 << 0;
    QTest::addRow("width20") << 2 << 20;
    QTest::addRow("width150") << 2 << 450;
}

void tst_Widgets::tst_QComboboxDelegate()
{
    QFETCH(int, paddingTest);
    QFETCH(int, widthTest);

    testWindow()->resize(300, 300);
    QScopedPointer<QComboBox> combobox(new QComboBox(testWindow()));

    class RectDelegate : public QAbstractItemDelegate
    {
    public:
        int sizeHintPadding = 2;
        int sizeHintWidth = 22;
        RectDelegate(QObject *parent = nullptr) : QAbstractItemDelegate(parent) {}

        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
        {
            Q_UNUSED(index);
            QRect rect = option.rect;
            int padding = sizeHintPadding;
            const int height = 22;
            const int width = height + sizeHintWidth;
            int yOffset = (option.rect.height() - height) / 2;
            int x = rect.x() + padding;
            int y = rect.y() + yOffset;
            painter->setClipRect(rect);
            painter->setBrush(QBrush(Qt::blue));
            painter->drawRect(QRect(x, y, width, height));
        }

        QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
        {
            Q_UNUSED(option);
            Q_UNUSED(index);
            const int height = 22;
            const int width = height;
            return QSize(width + 2 * sizeHintPadding, height + 2 * sizeHintPadding);
        }
    };

    auto rect1 = new RectDelegate(this);
    rect1->sizeHintPadding = paddingTest;
    rect1->sizeHintWidth = widthTest;
    combobox->setLabelDrawingMode(QComboBox::LabelDrawingMode::UseDelegate);
    combobox->setItemDelegate(rect1);
    combobox->addItem("item1");

    auto rect2 = new RectDelegate(this);
    rect2->sizeHintPadding = paddingTest;
    rect2->sizeHintWidth = widthTest;
    combobox->setLabelDrawingMode(QComboBox::LabelDrawingMode::UseDelegate);
    combobox->setItemDelegate(rect2);
    combobox->addItem("item2");

    QHBoxLayout layout;
    layout.addWidget(combobox.get());
    testWindow()->setLayout(&layout);
    takeStandardSnapshots();

    QTest::keyClick(combobox.get(), Qt::Key_Down, Qt::AltModifier);
    QBASELINE_CHECK_DEFERRED(takeScreenSnapshot(testWindow()->geometry()), "combobox");
}

void tst_Widgets::tst_QCommandLinkButton_data()
{
    QTest::addColumn<bool>("flat");
    QTest::addColumn<QString>("description");

    QTest::addRow("flatDescription") << true << QString("Command button very specific to windows vista");
    QTest::addRow("flatNoDescription") << true << QString("");
    QTest::addRow("noFlatNoDescription") << false << QString("");
}

void tst_Widgets::tst_QCommandLinkButton()
{
    QFETCH(const bool, flat);
    QFETCH(const QString, description);

    QScopedPointer<QCommandLinkButton> commandLink(new QCommandLinkButton(QString("CommandLink"), description, testWindow()));
    commandLink->setFlat(flat);
    commandLink->setDescription(description);

    QHBoxLayout layout;
    layout.addWidget(commandLink.get());
    testWindow()->setLayout(&layout);
    takeStandardSnapshots();
}

void tst_Widgets::tst_QLCDNumber_data()
{
    QTest::addColumn<int>("segmentstyle");

    QTest::addRow("outline") << 0;
    QTest::addRow("filled") << 1;
    QTest::addRow("flat") << 2;
}

void tst_Widgets::tst_QLCDNumber()
{
    QFETCH(const int, segmentstyle);

    testWindow()->resize(100, 100);

    QScopedPointer<QLCDNumber> lcdNumber(new QLCDNumber(99, testWindow()));
    lcdNumber->setHexMode();
    lcdNumber->setSegmentStyle(static_cast<QLCDNumber::SegmentStyle>(segmentstyle));


    QHBoxLayout layout;
    layout.addWidget(lcdNumber.get());
    testWindow()->setLayout(&layout);

    QBASELINE_CHECK_DEFERRED(takeSnapshot(), "lcdnumber");
}

QBASELINETEST_MAIN(tst_Widgets)

#include "tst_baseline_widgets.moc"
