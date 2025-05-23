// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial
#ifndef LOCALEWIDGET_H
#define LOCALEWIDGET_H

#include <QWidget>

class LocaleModel;
class QTableView;

class LocaleWidget : public QWidget
{
    Q_OBJECT
public:
    LocaleWidget(QWidget *parent = nullptr);
private:
    LocaleModel *m_model;
    QTableView *m_view;
};

#endif // LOCALEWIDGET_H
