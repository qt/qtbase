// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef STARRATING_H
#define STARRATING_H

#include <QPainter>
#include <QPolygonF>
#include <QSize>

//! [0]
class StarRating
{
public:
    enum class EditMode { Editable, ReadOnly };

    explicit StarRating(int starCount = 1, int maxStarCount = 5);

    void paint(QPainter *painter, const QRect &rect,
               const QPalette &palette, EditMode mode) const;
    QSize sizeHint() const;
    int starCount() const { return myStarCount; }
    int maxStarCount() const { return myMaxStarCount; }
    void setStarCount(int starCount) { myStarCount = starCount; }
    void setMaxStarCount(int maxStarCount) { myMaxStarCount = maxStarCount; }

private:
    QPolygonF starPolygon;
    QPolygonF diamondPolygon;
    int myStarCount;
    int myMaxStarCount;
};
//! [0]

//! [1]
Q_DECLARE_METATYPE(StarRating)
//! [1]

#endif
