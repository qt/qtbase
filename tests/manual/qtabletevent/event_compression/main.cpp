// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mousestatwidget.h"

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>

int main(int argc, char **argv){
  QApplication app(argc, argv);

  QWidget main;
  QVBoxLayout *layout = new QVBoxLayout(&main);
  layout->addWidget(new MouseStatWidget(true));
  layout->addWidget(new MouseStatWidget(false));
  main.resize(800, 600);
  main.show();
  return app.exec();
}
