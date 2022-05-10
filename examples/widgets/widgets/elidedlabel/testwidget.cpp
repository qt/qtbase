// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "testwidget.h"
#include "elidedlabel.h"

#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

//! [0]
TestWidget::TestWidget(QWidget *parent)
    : QWidget(parent)
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
    connect(switchButton, &QPushButton::clicked, this, &TestWidget::switchText);

    QPushButton *exitButton = new QPushButton(tr("Exit"));
    connect(exitButton, &QPushButton::clicked, this, &TestWidget::close);

    QLabel *label = new QLabel(tr("Elided"));
    label->setVisible(elidedText->isElided());
    connect(elidedText, &ElidedLabel::elisionChanged, label, &QLabel::setVisible);
    //! [2]

    //! [3]
    widthSlider = new QSlider(Qt::Horizontal);
    widthSlider->setMinimum(0);
    connect(widthSlider, &QSlider::valueChanged, this, &TestWidget::onWidthChanged);

    heightSlider = new QSlider(Qt::Vertical);
    heightSlider->setInvertedAppearance(true);
    heightSlider->setMinimum(0);
    connect(heightSlider, &QSlider::valueChanged, this, &TestWidget::onHeightChanged);
    //! [3]

    //! [4]
    QGridLayout *layout = new QGridLayout;
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
    Q_UNUSED(event);

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

