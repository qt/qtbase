// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef WIDGET2_H
#define WIDGET2_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget2;
}
QT_END_NAMESPACE

class Widget2 : public QWidget
{
  Q_OBJECT

public:
  explicit Widget2(QWidget* parent = nullptr);
  ~Widget2();
public slots:
  void onTextChanged(const QString& text);

private:
  Ui::Widget2* ui;
};

#endif // WIDGET2_H
