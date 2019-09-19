/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "slidersgroup.h"
#include "window.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
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
    connect(valueSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            horizontalSliders, &SlidersGroup::setValue);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(controlsGroup);
    layout->addWidget(stackedWidget);
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
    connect(orientationCombo, QOverload<int>::of(&QComboBox::activated),
//! [7] //! [8]
            stackedWidget, &QStackedWidget::setCurrentIndex);
    connect(minimumSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            horizontalSliders, &SlidersGroup::setMinimum);
    connect(minimumSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            verticalSliders, &SlidersGroup::setMinimum);
    connect(maximumSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            horizontalSliders, &SlidersGroup::setMaximum);
    connect(maximumSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
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
