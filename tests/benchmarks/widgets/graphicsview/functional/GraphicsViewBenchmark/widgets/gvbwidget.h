// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef GVBWIDGET_H
#define GVBWIDGET_H

#include <QGraphicsWidget>

class GvbWidget : public QGraphicsWidget
{
    Q_OBJECT

public:

    GvbWidget(QGraphicsItem * parent = nullptr, Qt::WindowFlags wFlags = { });
    ~GvbWidget();
    virtual void keyPressEvent(QKeyEvent *event);
};

#endif
