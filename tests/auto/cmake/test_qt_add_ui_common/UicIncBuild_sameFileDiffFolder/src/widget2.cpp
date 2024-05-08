// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "widget2.h"

#include "../../../../ui_widget1.h"

Widget2::Widget2(QWidget* parent)
  : QWidget(parent)
  , ui(new Ui::Widget2)
{
  ui->setupUi(this);
  connect(ui->lineEdit, SIGNAL(textChanged(const QString&)), this,
          SLOT(onTextChanged(const QString&)));
}

Widget2::~Widget2()
{
  delete ui;
}

void Widget2::onTextChanged(const QString& text)
{
  ui->OnTextChanged->setText(text);
}
