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

#include "imagecomposer.h"

//! [0]
static const QSize resultSize(200, 200);
//! [0]

//! [1]
ImageComposer::ImageComposer()
{
    sourceButton = new QToolButton;
    sourceButton->setIconSize(resultSize);

    operatorComboBox = new QComboBox;
    addOp(QPainter::CompositionMode_SourceOver, tr("SourceOver"));
    addOp(QPainter::CompositionMode_DestinationOver, tr("DestinationOver"));
    addOp(QPainter::CompositionMode_Clear, tr("Clear"));
    addOp(QPainter::CompositionMode_Source, tr("Source"));
    addOp(QPainter::CompositionMode_Destination, tr("Destination"));
    addOp(QPainter::CompositionMode_SourceIn, tr("SourceIn"));
    addOp(QPainter::CompositionMode_DestinationIn, tr("DestinationIn"));
    addOp(QPainter::CompositionMode_SourceOut, tr("SourceOut"));
    addOp(QPainter::CompositionMode_DestinationOut, tr("DestinationOut"));
    addOp(QPainter::CompositionMode_SourceAtop, tr("SourceAtop"));
    addOp(QPainter::CompositionMode_DestinationAtop, tr("DestinationAtop"));
    addOp(QPainter::CompositionMode_Xor, tr("Xor"));
    addOp(QPainter::CompositionMode_Plus, tr("Plus"));
    addOp(QPainter::CompositionMode_Multiply, tr("Multiply"));
    addOp(QPainter::CompositionMode_Screen, tr("Screen"));
    addOp(QPainter::CompositionMode_Overlay, tr("Overlay"));
    addOp(QPainter::CompositionMode_Darken, tr("Darken"));
    addOp(QPainter::CompositionMode_Lighten, tr("Lighten"));
    addOp(QPainter::CompositionMode_ColorDodge, tr("ColorDodge"));
    addOp(QPainter::CompositionMode_ColorBurn, tr("ColorBurn"));
    addOp(QPainter::CompositionMode_HardLight, tr("HardLight"));
    addOp(QPainter::CompositionMode_SoftLight, tr("SoftLight"));
    addOp(QPainter::CompositionMode_Difference, tr("Difference"));
    addOp(QPainter::CompositionMode_Exclusion, tr("Exclusion"));
//! [1]

//! [2]
    destinationButton = new QToolButton;
    destinationButton->setIconSize(resultSize);

    equalLabel = new QLabel(tr("="));

    resultLabel = new QLabel;
    resultLabel->setMinimumWidth(resultSize.width());
//! [2]

//! [3]
    connect(sourceButton, SIGNAL(clicked()), this, SLOT(chooseSource()));
    connect(operatorComboBox, SIGNAL(activated(int)),
            this, SLOT(recalculateResult()));
    connect(destinationButton, SIGNAL(clicked()),
            this, SLOT(chooseDestination()));
//! [3]

//! [4]
    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(sourceButton, 0, 0, 3, 1);
    mainLayout->addWidget(operatorComboBox, 1, 1);
    mainLayout->addWidget(destinationButton, 0, 2, 3, 1);
    mainLayout->addWidget(equalLabel, 1, 3);
    mainLayout->addWidget(resultLabel, 0, 4, 3, 1);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    setLayout(mainLayout);
//! [4]

//! [5]
    resultImage = QImage(resultSize, QImage::Format_ARGB32_Premultiplied);

    loadImage(":/images/butterfly.png", &sourceImage, sourceButton);
    loadImage(":/images/checker.png", &destinationImage, destinationButton);

    setWindowTitle(tr("Image Composition"));
}
//! [5]

//! [6]
void ImageComposer::chooseSource()
{
    chooseImage(tr("Choose Source Image"), &sourceImage, sourceButton);
}
//! [6]

//! [7]
void ImageComposer::chooseDestination()
{
    chooseImage(tr("Choose Destination Image"), &destinationImage,
                destinationButton);
}
//! [7]

//! [8]
void ImageComposer::recalculateResult()
{
    QPainter::CompositionMode mode = currentMode();

    QPainter painter(&resultImage);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(resultImage.rect(), Qt::transparent);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.drawImage(0, 0, destinationImage);
    painter.setCompositionMode(mode);
    painter.drawImage(0, 0, sourceImage);
    painter.setCompositionMode(QPainter::CompositionMode_DestinationOver);
    painter.fillRect(resultImage.rect(), Qt::white);
    painter.end();

    resultLabel->setPixmap(QPixmap::fromImage(resultImage));
}
//! [8]

//! [9]
void ImageComposer::addOp(QPainter::CompositionMode mode, const QString &name)
{
    operatorComboBox->addItem(name, mode);
}
//! [9]

//! [10]
void ImageComposer::chooseImage(const QString &title, QImage *image,
                                QToolButton *button)
{
    QString fileName = QFileDialog::getOpenFileName(this, title);
    if (!fileName.isEmpty())
        loadImage(fileName, image, button);
}
//! [10]

//! [11]
void ImageComposer::loadImage(const QString &fileName, QImage *image,
                              QToolButton *button)
{
    image->load(fileName);

    QImage fixedImage(resultSize, QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&fixedImage);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(fixedImage.rect(), Qt::transparent);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.drawImage(imagePos(*image), *image);
    painter.end();
    button->setIcon(QPixmap::fromImage(fixedImage));

    *image = fixedImage;

    recalculateResult();
}
//! [11]

//! [12]
QPainter::CompositionMode ImageComposer::currentMode() const
{
    return (QPainter::CompositionMode)
           operatorComboBox->itemData(operatorComboBox->currentIndex()).toInt();
}
//! [12]

//! [13]
QPoint ImageComposer::imagePos(const QImage &image) const
{
    return QPoint((resultSize.width() - image.width()) / 2,
                  (resultSize.height() - image.height()) / 2);
}
//! [13]
