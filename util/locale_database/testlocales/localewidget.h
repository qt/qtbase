/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the utils of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
****************************************************************************/
#ifndef LOCALEWIDGET_H
#define LOCALEWIDGET_H

#include <QWidget>

class LocaleModel;
class QTableView;

class LocaleWidget : public QWidget
{
    Q_OBJECT
public:
    LocaleWidget(QWidget *parent = 0);
private:
    LocaleModel *m_model;
    QTableView *m_view;
};

#endif // LOCALEWIDGET_H
