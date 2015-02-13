/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "testwidget.h"
#include "elidedlabel.h"

#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QGridLayout>

//! [0]
TestWidget::TestWidget(QWidget *parent):
    QWidget(parent)
{
    const QString romeo = tr(
        "But soft, what light through yonder window breaks? / "
        "It is the east, and Juliet is the sun. / "
        "Arise, fair sun, and kill the envious moon, / "
        "Who is already sick and pale with grief / "
        "That thou, her maid, art far more fair than she."
    );

    const QString macbeth = tr(
        "To-morrow, and to-morrow, and to-morrow, / "
        "Creeps in this petty pace from day to day, / "
        "To the last syllable of recorded time; / "
        "And all our yesterdays have lighted fools / "
        "The way to dusty death. Out, out, brief candle! / "
        "Life's but a walking shadow, a poor player, / "
        "That struts and frets his hour upon the stage, / "
        "And then is heard no more. It is a tale / "
        "Told by an idiot, full of sound and fury, / "
        "Signifying nothing."
    );

    const QString harry = tr("Feeling lucky, punk?");

    textSamples << romeo << macbeth << harry;
    //! [0]

    //! [1]
    sampleIndex = 0;
    elidedText = new ElidedLabel(textSamples[sampleIndex], this);
    elidedText->setFrameStyle(QFrame::Box);
    //! [1]

    //! [2]
    QPushButton *switchButton = new QPushButton(tr("Switch text"));
    connect(switchButton, SIGNAL(clicked(bool)), this, SLOT(switchText()));

    QPushButton *exitButton = new QPushButton(tr("Exit"));
    connect(exitButton, SIGNAL(clicked(bool)), this, SLOT(close()));

    QLabel *label = new QLabel(tr("Elided"));
    label->setVisible(elidedText->isElided());
    connect(elidedText, SIGNAL(elisionChanged(bool)), label, SLOT(setVisible(bool)));
    //! [2]

    //! [3]
    widthSlider = new QSlider(Qt::Horizontal);
    widthSlider->setMinimum(0);
    connect(widthSlider, SIGNAL(valueChanged(int)), this, SLOT(onWidthChanged(int)));

    heightSlider = new QSlider(Qt::Vertical);
    heightSlider->setInvertedAppearance(true);
    heightSlider->setMinimum(0);
    connect(heightSlider, SIGNAL(valueChanged(int)), this, SLOT(onHeightChanged(int)));
    //! [3]

    //! [4]
    QGridLayout *layout = new QGridLayout();
    layout->addWidget(label, 0, 1, Qt::AlignCenter);
    layout->addWidget(switchButton, 0, 2);
    layout->addWidget(exitButton, 0, 3);
    layout->addWidget(widthSlider, 1, 1, 1, 3);
    layout->addWidget(heightSlider, 2, 0);
    layout->addWidget(elidedText, 2, 1, 1, 3, Qt::AlignTop | Qt::AlignLeft);

    setLayout(layout);
    //! [4]
}

//! [6]
void TestWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)

    int maxWidth = widthSlider->width();
    widthSlider->setMaximum(maxWidth);
    widthSlider->setValue(maxWidth / 2);

    int maxHeight = heightSlider->height();
    heightSlider->setMaximum(maxHeight);
    heightSlider->setValue(maxHeight / 2);

    elidedText->setFixedSize(widthSlider->value(), heightSlider->value());
}
//! [6]

//! [7]
void TestWidget::switchText()
{
    sampleIndex = (sampleIndex + 1) % textSamples.size();
    elidedText->setText(textSamples.at(sampleIndex));
}
//! [7]

//! [8]
void TestWidget::onWidthChanged(int width)
{
    elidedText->setFixedWidth(width);
}

void TestWidget::onHeightChanged(int height)
{
    elidedText->setFixedHeight(height);
}
//! [8]


