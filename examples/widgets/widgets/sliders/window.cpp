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
    horizontalSliders = new SlidersGroup(Qt::Horizontal, tr("Horizontal"));
    verticalSliders = new SlidersGroup(Qt::Vertical, tr("Vertical"));

    stackedWidget = new QStackedWidget;
    stackedWidget->addWidget(horizontalSliders);
    stackedWidget->addWidget(verticalSliders);

    createControls(tr("Controls"));
//! [0]

//! [1]
    connect(horizontalSliders, &SlidersGroup::valueChanged,
//! [1] //! [2]
            verticalSliders, &SlidersGroup::setValue);
    connect(verticalSliders, &SlidersGroup::valueChanged,
            valueSpinBox, &QSpinBox::setValue);
    connect(valueSpinBox, &QSpinBox::valueChanged,
            horizontalSliders, &SlidersGroup::setValue);

    layout = new QGridLayout;
    layout->addWidget(stackedWidget, 0, 1);
    layout->addWidget(controlsGroup, 0, 0);

    setLayout(layout);

    minimumSpinBox->setValue(0);
    maximumSpinBox->setValue(20);
    valueSpinBox->setValue(5);

    setWindowTitle(tr("Sliders"));
}
//! [2]

//! [3]
void Window::createControls(const QString &title)
//! [3] //! [4]
{
    controlsGroup = new QGroupBox(title);

    minimumLabel = new QLabel(tr("Minimum value:"));
    maximumLabel = new QLabel(tr("Maximum value:"));
    valueLabel = new QLabel(tr("Current value:"));

    invertedAppearance = new QCheckBox(tr("Inverted appearance"));
    invertedKeyBindings = new QCheckBox(tr("Inverted key bindings"));

//! [4] //! [5]
    minimumSpinBox = new QSpinBox;
//! [5] //! [6]
    minimumSpinBox->setRange(-100, 100);
    minimumSpinBox->setSingleStep(1);

    maximumSpinBox = new QSpinBox;
    maximumSpinBox->setRange(-100, 100);
    maximumSpinBox->setSingleStep(1);

    valueSpinBox = new QSpinBox;
    valueSpinBox->setRange(-100, 100);
    valueSpinBox->setSingleStep(1);

    orientationCombo = new QComboBox;
    orientationCombo->addItem(tr("Horizontal slider-like widgets"));
    orientationCombo->addItem(tr("Vertical slider-like widgets"));

//! [6] //! [7]
    connect(orientationCombo, &QComboBox::activated,
//! [7] //! [8]
            stackedWidget, &QStackedWidget::setCurrentIndex);
    connect(minimumSpinBox, &QSpinBox::valueChanged,
            horizontalSliders, &SlidersGroup::setMinimum);
    connect(minimumSpinBox, &QSpinBox::valueChanged,
            verticalSliders, &SlidersGroup::setMinimum);
    connect(maximumSpinBox, &QSpinBox::valueChanged,
            horizontalSliders, &SlidersGroup::setMaximum);
    connect(maximumSpinBox, &QSpinBox::valueChanged,
            verticalSliders, &SlidersGroup::setMaximum);
    connect(invertedAppearance, &QCheckBox::toggled,
            horizontalSliders, &SlidersGroup::invertAppearance);
    connect(invertedAppearance, &QCheckBox::toggled,
            verticalSliders, &SlidersGroup::invertAppearance);
    connect(invertedKeyBindings, &QCheckBox::toggled,
            horizontalSliders, &SlidersGroup::invertKeyBindings);
    connect(invertedKeyBindings, &QCheckBox::toggled,
            verticalSliders, &SlidersGroup::invertKeyBindings);

    QGridLayout *controlsLayout = new QGridLayout;
    controlsLayout->addWidget(minimumLabel, 0, 0);
    controlsLayout->addWidget(maximumLabel, 1, 0);
    controlsLayout->addWidget(valueLabel, 2, 0);
    controlsLayout->addWidget(minimumSpinBox, 0, 1);
    controlsLayout->addWidget(maximumSpinBox, 1, 1);
    controlsLayout->addWidget(valueSpinBox, 2, 1);
    controlsLayout->addWidget(invertedAppearance, 0, 2);
    controlsLayout->addWidget(invertedKeyBindings, 1, 2);
    controlsLayout->addWidget(orientationCombo, 3, 0, 1, 3);
    controlsGroup->setLayout(controlsLayout);

}
//! [8]


void Window::resizeEvent(QResizeEvent *e)
{
    Q_UNUSED(e);
    if (width() == 0 || height() == 0)
        return;

     const double aspectRatio = double(width()) / double(height());

    if ((aspectRatio < 1.0) && (oldAspectRatio > 1.0)) {
        layout->removeWidget(controlsGroup);
        layout->removeWidget(stackedWidget);

        layout->addWidget(stackedWidget, 1, 0);
        layout->addWidget(controlsGroup, 0, 0);

        oldAspectRatio = aspectRatio;
    }
    else if ((aspectRatio > 1.0) && (oldAspectRatio < 1.0)) {
        layout->removeWidget(controlsGroup);
        layout->removeWidget(stackedWidget);

        layout->addWidget(stackedWidget, 0, 1);
        layout->addWidget(controlsGroup, 0, 0);

        oldAspectRatio = aspectRatio;
    }
}



