/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>

#include "renderarea.h"
#include "window.h"

//! [0]
const int IdRole = Qt::UserRole;
//! [0]

//! [1]
Window::Window()
{
    renderArea = new RenderArea;

    shapeComboBox = new QComboBox;
    shapeComboBox->addItem(tr("Polygon"), RenderArea::Polygon);
    shapeComboBox->addItem(tr("Rectangle"), RenderArea::Rect);
    shapeComboBox->addItem(tr("Rounded Rectangle"), RenderArea::RoundedRect);
    shapeComboBox->addItem(tr("Ellipse"), RenderArea::Ellipse);
    shapeComboBox->addItem(tr("Pie"), RenderArea::Pie);
    shapeComboBox->addItem(tr("Chord"), RenderArea::Chord);
    shapeComboBox->addItem(tr("Path"), RenderArea::Path);
    shapeComboBox->addItem(tr("Line"), RenderArea::Line);
    shapeComboBox->addItem(tr("Polyline"), RenderArea::Polyline);
    shapeComboBox->addItem(tr("Arc"), RenderArea::Arc);
    shapeComboBox->addItem(tr("Points"), RenderArea::Points);
    shapeComboBox->addItem(tr("Text"), RenderArea::Text);
    shapeComboBox->addItem(tr("Pixmap"), RenderArea::Pixmap);

    shapeLabel = new QLabel(tr("&Shape:"));
    shapeLabel->setBuddy(shapeComboBox);
//! [1]

//! [2]
    penWidthSpinBox = new QSpinBox;
    penWidthSpinBox->setRange(0, 20);
    penWidthSpinBox->setSpecialValueText(tr("0 (cosmetic pen)"));

    penWidthLabel = new QLabel(tr("Pen &Width:"));
    penWidthLabel->setBuddy(penWidthSpinBox);
//! [2]

//! [3]
    penStyleComboBox = new QComboBox;
    penStyleComboBox->addItem(tr("Solid"), Qt::SolidLine);
    penStyleComboBox->addItem(tr("Dash"), Qt::DashLine);
    penStyleComboBox->addItem(tr("Dot"), Qt::DotLine);
    penStyleComboBox->addItem(tr("Dash Dot"), Qt::DashDotLine);
    penStyleComboBox->addItem(tr("Dash Dot Dot"), Qt::DashDotDotLine);
    penStyleComboBox->addItem(tr("None"), Qt::NoPen);

    penStyleLabel = new QLabel(tr("&Pen Style:"));
    penStyleLabel->setBuddy(penStyleComboBox);

    penCapComboBox = new QComboBox;
    penCapComboBox->addItem(tr("Flat"), Qt::FlatCap);
    penCapComboBox->addItem(tr("Square"), Qt::SquareCap);
    penCapComboBox->addItem(tr("Round"), Qt::RoundCap);

    penCapLabel = new QLabel(tr("Pen &Cap:"));
    penCapLabel->setBuddy(penCapComboBox);

    penJoinComboBox = new QComboBox;
    penJoinComboBox->addItem(tr("Miter"), Qt::MiterJoin);
    penJoinComboBox->addItem(tr("Bevel"), Qt::BevelJoin);
    penJoinComboBox->addItem(tr("Round"), Qt::RoundJoin);

    penJoinLabel = new QLabel(tr("Pen &Join:"));
    penJoinLabel->setBuddy(penJoinComboBox);
//! [3]

//! [4]
    brushStyleComboBox = new QComboBox;
    brushStyleComboBox->addItem(tr("Linear Gradient"),
            Qt::LinearGradientPattern);
    brushStyleComboBox->addItem(tr("Radial Gradient"),
            Qt::RadialGradientPattern);
    brushStyleComboBox->addItem(tr("Conical Gradient"),
            Qt::ConicalGradientPattern);
    brushStyleComboBox->addItem(tr("Texture"), Qt::TexturePattern);
    brushStyleComboBox->addItem(tr("Solid"), Qt::SolidPattern);
    brushStyleComboBox->addItem(tr("Horizontal"), Qt::HorPattern);
    brushStyleComboBox->addItem(tr("Vertical"), Qt::VerPattern);
    brushStyleComboBox->addItem(tr("Cross"), Qt::CrossPattern);
    brushStyleComboBox->addItem(tr("Backward Diagonal"), Qt::BDiagPattern);
    brushStyleComboBox->addItem(tr("Forward Diagonal"), Qt::FDiagPattern);
    brushStyleComboBox->addItem(tr("Diagonal Cross"), Qt::DiagCrossPattern);
    brushStyleComboBox->addItem(tr("Dense 1"), Qt::Dense1Pattern);
    brushStyleComboBox->addItem(tr("Dense 2"), Qt::Dense2Pattern);
    brushStyleComboBox->addItem(tr("Dense 3"), Qt::Dense3Pattern);
    brushStyleComboBox->addItem(tr("Dense 4"), Qt::Dense4Pattern);
    brushStyleComboBox->addItem(tr("Dense 5"), Qt::Dense5Pattern);
    brushStyleComboBox->addItem(tr("Dense 6"), Qt::Dense6Pattern);
    brushStyleComboBox->addItem(tr("Dense 7"), Qt::Dense7Pattern);
    brushStyleComboBox->addItem(tr("None"), Qt::NoBrush);

    brushStyleLabel = new QLabel(tr("&Brush Style:"));
    brushStyleLabel->setBuddy(brushStyleComboBox);
//! [4]

//! [5]
    otherOptionsLabel = new QLabel(tr("Other Options:"));
//! [5] //! [6]
    antialiasingCheckBox = new QCheckBox(tr("&Antialiasing"));
//! [6] //! [7]
    transformationsCheckBox = new QCheckBox(tr("&Transformations"));
//! [7]

//! [8]
    connect(shapeComboBox, SIGNAL(activated(int)),
            this, SLOT(shapeChanged()));
    connect(penWidthSpinBox, SIGNAL(valueChanged(int)),
            this, SLOT(penChanged()));
    connect(penStyleComboBox, SIGNAL(activated(int)),
            this, SLOT(penChanged()));
    connect(penCapComboBox, SIGNAL(activated(int)),
            this, SLOT(penChanged()));
    connect(penJoinComboBox, SIGNAL(activated(int)),
            this, SLOT(penChanged()));
    connect(brushStyleComboBox, SIGNAL(activated(int)),
            this, SLOT(brushChanged()));
    connect(antialiasingCheckBox, SIGNAL(toggled(bool)),
            renderArea, SLOT(setAntialiased(bool)));
    connect(transformationsCheckBox, SIGNAL(toggled(bool)),
            renderArea, SLOT(setTransformed(bool)));
//! [8]

//! [9]
    QGridLayout *mainLayout = new QGridLayout;
//! [9] //! [10]
    mainLayout->setColumnStretch(0, 1);
    mainLayout->setColumnStretch(3, 1);
    mainLayout->addWidget(renderArea, 0, 0, 1, 4);
    mainLayout->setRowMinimumHeight(1, 6);
    mainLayout->addWidget(shapeLabel, 2, 1, Qt::AlignRight);
    mainLayout->addWidget(shapeComboBox, 2, 2);
    mainLayout->addWidget(penWidthLabel, 3, 1, Qt::AlignRight);
    mainLayout->addWidget(penWidthSpinBox, 3, 2);
    mainLayout->addWidget(penStyleLabel, 4, 1, Qt::AlignRight);
    mainLayout->addWidget(penStyleComboBox, 4, 2);
    mainLayout->addWidget(penCapLabel, 5, 1, Qt::AlignRight);
    mainLayout->addWidget(penCapComboBox, 5, 2);
    mainLayout->addWidget(penJoinLabel, 6, 1, Qt::AlignRight);
    mainLayout->addWidget(penJoinComboBox, 6, 2);
    mainLayout->addWidget(brushStyleLabel, 7, 1, Qt::AlignRight);
    mainLayout->addWidget(brushStyleComboBox, 7, 2);
    mainLayout->setRowMinimumHeight(8, 6);
    mainLayout->addWidget(otherOptionsLabel, 9, 1, Qt::AlignRight);
    mainLayout->addWidget(antialiasingCheckBox, 9, 2);
    mainLayout->addWidget(transformationsCheckBox, 10, 2);
    setLayout(mainLayout);

    shapeChanged();
    penChanged();
    brushChanged();
    antialiasingCheckBox->setChecked(true);

    setWindowTitle(tr("Basic Drawing"));
}
//! [10]

//! [11]
void Window::shapeChanged()
{
    RenderArea::Shape shape = RenderArea::Shape(shapeComboBox->itemData(
            shapeComboBox->currentIndex(), IdRole).toInt());
    renderArea->setShape(shape);
}
//! [11]

//! [12]
void Window::penChanged()
{
    int width = penWidthSpinBox->value();
    Qt::PenStyle style = Qt::PenStyle(penStyleComboBox->itemData(
            penStyleComboBox->currentIndex(), IdRole).toInt());
    Qt::PenCapStyle cap = Qt::PenCapStyle(penCapComboBox->itemData(
            penCapComboBox->currentIndex(), IdRole).toInt());
    Qt::PenJoinStyle join = Qt::PenJoinStyle(penJoinComboBox->itemData(
            penJoinComboBox->currentIndex(), IdRole).toInt());

    renderArea->setPen(QPen(Qt::blue, width, style, cap, join));
}
//! [12]

//! [13]
void Window::brushChanged()
{
    Qt::BrushStyle style = Qt::BrushStyle(brushStyleComboBox->itemData(
//! [13]
            brushStyleComboBox->currentIndex(), IdRole).toInt());

//! [14]
    if (style == Qt::LinearGradientPattern) {
        QLinearGradient linearGradient(0, 0, 100, 100);
        linearGradient.setColorAt(0.0, Qt::white);
        linearGradient.setColorAt(0.2, Qt::green);
        linearGradient.setColorAt(1.0, Qt::black);
        renderArea->setBrush(linearGradient);
//! [14] //! [15]
    } else if (style == Qt::RadialGradientPattern) {
        QRadialGradient radialGradient(50, 50, 50, 70, 70);
        radialGradient.setColorAt(0.0, Qt::white);
        radialGradient.setColorAt(0.2, Qt::green);
        radialGradient.setColorAt(1.0, Qt::black);
        renderArea->setBrush(radialGradient);
    } else if (style == Qt::ConicalGradientPattern) {
        QConicalGradient conicalGradient(50, 50, 150);
        conicalGradient.setColorAt(0.0, Qt::white);
        conicalGradient.setColorAt(0.2, Qt::green);
        conicalGradient.setColorAt(1.0, Qt::black);
        renderArea->setBrush(conicalGradient);
//! [15] //! [16]
    } else if (style == Qt::TexturePattern) {
        renderArea->setBrush(QBrush(QPixmap(":/images/brick.png")));
//! [16] //! [17]
    } else {
        renderArea->setBrush(QBrush(Qt::green, style));
    }
}
//! [17]
