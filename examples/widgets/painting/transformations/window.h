// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include "renderarea.h"

#include <QList>
#include <QPainterPath>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

//! [0]
class Window : public QWidget
{
    Q_OBJECT

public:
    Window();

public slots:
    void operationChanged();
    void shapeSelected(int index);
//! [0]

//! [1]
private:
    void setupShapes();

    enum { NumTransformedAreas = 3 };
    RenderArea *originalRenderArea;
    RenderArea *transformedRenderAreas[NumTransformedAreas];
    QComboBox *shapeComboBox;
    QComboBox *operationComboBoxes[NumTransformedAreas];
    QList<QPainterPath> shapes;
};
//! [1]

#endif
