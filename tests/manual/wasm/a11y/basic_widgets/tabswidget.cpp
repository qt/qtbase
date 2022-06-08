// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "tabswidget.h"

GeneralTab::GeneralTab(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setSizeConstraint(QLayout::SetMaximumSize);

    layout->addWidget(new QLabel("This is a text label"));

    QPushButton *btn = new QPushButton("This is a push button");
    layout->addWidget(btn);
    connect(btn, &QPushButton::released, this, [=] () {
        btn->setText("You clicked me");
    });

    layout->addWidget(new QCheckBox("This is a check box"));

    layout->addWidget(new QRadioButton("Radio 1"));
    layout->addWidget(new QRadioButton("Radio 2"));

    QSlider *slider = new QSlider(Qt::Horizontal);
    slider->setTickInterval(10);
    slider->setTickPosition(QSlider::TicksAbove);
    layout->addWidget(slider);

    QSpinBox *spin = new QSpinBox();
    spin->setValue(10);
    spin->setSingleStep(1);
    layout->addWidget(spin);
    layout->addStretch();

    QScrollBar *scrollBar = new QScrollBar(Qt::Horizontal);
    scrollBar->setFocusPolicy(Qt::StrongFocus);
    layout->addWidget(scrollBar);

    setLayout(layout);
}


EditViewTab::EditViewTab(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setSizeConstraint(QLayout::SetMaximumSize);
    textEdit = new QPlainTextEdit();
    textEdit->setPlaceholderText("Enter Text here");
    layout->addWidget(textEdit);
    setLayout(layout);

}

void EditViewTab::showEvent( QShowEvent* event ) {
  if (!b_connected)
  {
    emit connectToToolBar();
    b_connected=true;
  }
  QWidget::showEvent( event );
}
