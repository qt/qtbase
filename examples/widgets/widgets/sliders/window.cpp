// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "slidersgroup.h"
#include "window.h"
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QSpinBox>
#include <QStackedWidget>

//! [0]
Window::Window(QWidget *parent)
    : QWidget(parent)
{
    slidersGroup = new SlidersGroup(tr("Sliders"));

    createControls(tr("Controls"));
//! [0]

//! [1]
    layout = new QBoxLayout(QBoxLayout::LeftToRight);
    layout->addWidget(controlsGroup);
    layout->addWidget(slidersGroup);
    setLayout(layout);

    minimumSpinBox->setValue(0);
    maximumSpinBox->setValue(20);
    valueSpinBox->setValue(5);

    setWindowTitle(tr("Sliders"));
}
//! [1]

//! [2]
void Window::createControls(const QString &title)
//! [2] //! [3]
{
    controlsGroup = new QGroupBox(title);

    minimumLabel = new QLabel(tr("Minimum value:"));
    maximumLabel = new QLabel(tr("Maximum value:"));
    valueLabel = new QLabel(tr("Current value:"));

    invertedAppearance = new QCheckBox(tr("Inverted appearance"));
    invertedKeyBindings = new QCheckBox(tr("Inverted key bindings"));

//! [3] //! [4]
    minimumSpinBox = new QSpinBox;
//! [4] //! [5]
    minimumSpinBox->setRange(-100, 100);
    minimumSpinBox->setSingleStep(1);

    maximumSpinBox = new QSpinBox;
    maximumSpinBox->setRange(-100, 100);
    maximumSpinBox->setSingleStep(1);

    valueSpinBox = new QSpinBox;
    valueSpinBox->setRange(-100, 100);
    valueSpinBox->setSingleStep(1);

//! [5] //! [6]
    connect(slidersGroup, &SlidersGroup::valueChanged,
            valueSpinBox, &QSpinBox::setValue);
    connect(valueSpinBox, &QSpinBox::valueChanged,
            slidersGroup, &SlidersGroup::setValue);
    connect(minimumSpinBox, &QSpinBox::valueChanged,
            slidersGroup, &SlidersGroup::setMinimum);
    connect(maximumSpinBox, &QSpinBox::valueChanged,
            slidersGroup, &SlidersGroup::setMaximum);
    connect(invertedAppearance, &QCheckBox::toggled,
            slidersGroup, &SlidersGroup::invertAppearance);
    connect(invertedKeyBindings, &QCheckBox::toggled,
            slidersGroup, &SlidersGroup::invertKeyBindings);

    QGridLayout *controlsLayout = new QGridLayout;
    controlsLayout->addWidget(minimumLabel, 0, 0);
    controlsLayout->addWidget(maximumLabel, 1, 0);
    controlsLayout->addWidget(valueLabel, 2, 0);
    controlsLayout->addWidget(minimumSpinBox, 0, 1);
    controlsLayout->addWidget(maximumSpinBox, 1, 1);
    controlsLayout->addWidget(valueSpinBox, 2, 1);
    controlsLayout->addWidget(invertedAppearance, 0, 2);
    controlsLayout->addWidget(invertedKeyBindings, 1, 2);
    controlsGroup->setLayout(controlsLayout);

}
//! [6]


//! [7]
void Window::resizeEvent(QResizeEvent *)
{
    if (width() == 0 || height() == 0)
        return;

    const double aspectRatio = double(width()) / double(height());

    if (aspectRatio < 1.0) {
        layout->setDirection(QBoxLayout::TopToBottom);
        slidersGroup->setOrientation(Qt::Horizontal);
    } else if (aspectRatio > 1.0) {
        layout->setDirection(QBoxLayout::LeftToRight);
        slidersGroup->setOrientation(Qt::Vertical);
    }
}
//! [7]
