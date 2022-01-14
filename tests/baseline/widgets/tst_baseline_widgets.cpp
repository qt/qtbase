/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <qbaselinetest.h>
#include <qwidgetbaselinetest.h>
#include <QtWidgets>

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

void tst_Widgets::tst_QSpinBox_data()
{
    QTest::addColumn<QAbstractSpinBox::ButtonSymbols>("buttons");

    QTest::addRow("NoButtons") << QSpinBox::NoButtons;
    QTest::addRow("UpDownArrows") << QSpinBox::UpDownArrows;
    QTest::addRow("PlusMinus") << QSpinBox::PlusMinus;
}

void tst_Widgets::tst_QSpinBox()
{
    QFETCH(const QSpinBox::ButtonSymbols, buttons);

    QSpinBox *spinBox = new QSpinBox;
    spinBox->setButtonSymbols(buttons);
    spinBox->setMinimumWidth(200);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(spinBox);

    testWindow()->setLayout(layout);

    takeStandardSnapshots();

    // Left is default alignment:
    QBASELINE_CHECK(takeSnapshot(), "align_left");

    spinBox->setAlignment(Qt::AlignHCenter);
    QBASELINE_CHECK(takeSnapshot(), "align_center");

    spinBox->setAlignment(Qt::AlignRight);
    QBASELINE_CHECK(takeSnapshot(), "align_right");
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

    // fixedWidth <0 will be interpreted as variable width
    QTest::newRow("RoundedNorth_3_variableWidth") << QTabBar::RoundedNorth << 3 << -1;
    QTest::newRow("RoundedEast_3_variableWidth") << QTabBar::RoundedEast << 3 << -1;
    QTest::newRow("RoundedWest_3_variableWidth") << QTabBar::RoundedWest << 3 << -1;
    QTest::newRow("RoundedSouth_3_variableWidth") << QTabBar::RoundedSouth << 3 << -1;
    QTest::newRow("RoundedNorth_20_fixedWidth") << QTabBar::RoundedNorth << 20 << 250;
}

void tst_Widgets::tst_QTabBar()
{
    QFETCH(QTabBar::Shape, shape);
    QFETCH(int, numberTabs);
    QFETCH(int, fixedWidth);

    QTabBar bar (testWindow());
    bar.setShape(shape);
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
}

#define main _realmain
QTEST_MAIN(tst_Widgets)
#undef main

int main(int argc, char *argv[])
{
    qSetGlobalQHashSeed(0);   // Avoid rendering variations caused by QHash randomization

    QBaselineTest::handleCmdLineArgs(&argc, &argv);
    return _realmain(argc, argv);
}

#include "tst_baseline_widgets.moc"
