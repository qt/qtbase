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

    void tst_QProgressBar_data();
    void tst_QProgressBar();

    void tst_QSpinBox_data();
    void tst_QSpinBox();
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
