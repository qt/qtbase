// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SHAPEITEM_H
#define SHAPEITEM_H

#include <QColor>
#include <QPainterPath>
#include <QPoint>

//! [0]
class ShapeItem
{
public:
    void setPath(const QPainterPath &path);
    void setToolTip(const QString &toolTip);
    void setPosition(const QPoint &position);
    void setColor(const QColor &color);

    QPainterPath path() const;
    QPoint position() const;
    QColor color() const;
    QString toolTip() const;

private:
    QPainterPath myPath;
    QPoint myPosition;
    QColor myColor;
    QString myToolTip;
};
//! [0]

#endif
