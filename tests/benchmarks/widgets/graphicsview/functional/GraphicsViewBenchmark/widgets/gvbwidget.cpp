// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtGui>
#include "gvbwidget.h"

GvbWidget::GvbWidget(QGraphicsItem * parent, Qt::WindowFlags wFlags)
    : QGraphicsWidget(parent, wFlags)
{

}

GvbWidget::~GvbWidget()
{
}

void GvbWidget::keyPressEvent(QKeyEvent *event)
{
    Q_UNUSED(event);
}

