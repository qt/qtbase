// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "shapeitem.h"

//! [0]
QPainterPath ShapeItem::path() const
{
    return myPath;
}
//! [0]

//! [1]
QPoint ShapeItem::position() const
{
    return myPosition;
}
//! [1]

//! [2]
QColor ShapeItem::color() const
{
    return myColor;
}
//! [2]

//! [3]
QString ShapeItem::toolTip() const
{
    return myToolTip;
}
//! [3]

//! [4]
void ShapeItem::setPath(const QPainterPath &path)
{
    myPath = path;
}
//! [4]

//! [5]
void ShapeItem::setToolTip(const QString &toolTip)
{
    myToolTip = toolTip;
}
//! [5]

//! [6]
void ShapeItem::setPosition(const QPoint &position)
{
    myPosition = position;
}
//! [6]

//! [7]
void ShapeItem::setColor(const QColor &color)
{
    myColor = color;
}
//! [7]
