// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "slidersgroup.h"

#include <QBoxLayout>
#include <QDial>
#include <QScrollBar>
#include <QSlider>

//! [0]
SlidersGroup::SlidersGroup(const QString &title, QWidget *parent)
    : QGroupBox(title, parent)
{
    slider = new QSlider;
    slider->setFocusPolicy(Qt::StrongFocus);
    slider->setTickPosition(QSlider::TicksBothSides);
    slider->setTickInterval(10);
    slider->setSingleStep(1);

    scrollBar = new QScrollBar;
    scrollBar->setFocusPolicy(Qt::StrongFocus);

    dial = new QDial;
    dial->setFocusPolicy(Qt::StrongFocus);

//! [0] //! [1]
    connect(slider, &QSlider::valueChanged, scrollBar, &QScrollBar::setValue);
    connect(scrollBar, &QScrollBar::valueChanged, dial, &QDial::setValue);
    connect(dial, &QDial::valueChanged, slider, &QSlider::setValue);
    connect(dial, &QDial::valueChanged, this, &SlidersGroup::valueChanged);
//! [1] //! [4]
    slidersLayout = new QBoxLayout(QBoxLayout::LeftToRight);
    slidersLayout->addWidget(slider);
    slidersLayout->addWidget(scrollBar);
    slidersLayout->addWidget(dial);
    setLayout(slidersLayout);
}
//! [4]

//! [5]
void SlidersGroup::setValue(int value)
//! [5] //! [6]
{
    slider->setValue(value);
}
//! [6]

//! [7]
void SlidersGroup::setMinimum(int value)
//! [7] //! [8]
{
    slider->setMinimum(value);
    scrollBar->setMinimum(value);
    dial->setMinimum(value);
}
//! [8]

//! [9]
void SlidersGroup::setMaximum(int value)
//! [9] //! [10]
{
    slider->setMaximum(value);
    scrollBar->setMaximum(value);
    dial->setMaximum(value);
}
//! [10]

//! [11]
void SlidersGroup::invertAppearance(bool invert)
//! [11] //! [12]
{
    slider->setInvertedAppearance(invert);
    scrollBar->setInvertedAppearance(invert);
    dial->setInvertedAppearance(invert);
}
//! [12]

//! [13]
void SlidersGroup::invertKeyBindings(bool invert)
//! [13] //! [14]
{
    slider->setInvertedControls(invert);
    scrollBar->setInvertedControls(invert);
    dial->setInvertedControls(invert);
}
//! [14]

//! [15]
void SlidersGroup::setOrientation(Qt::Orientation orientation)
{
    slidersLayout->setDirection(orientation == Qt::Horizontal
                                ? QBoxLayout::TopToBottom
                                : QBoxLayout::LeftToRight);
    scrollBar->setOrientation(orientation);
    slider->setOrientation(orientation);
}
//! [15]
