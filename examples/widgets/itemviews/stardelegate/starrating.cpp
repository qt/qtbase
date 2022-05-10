// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "starrating.h"

#include <QtWidgets>
#include <cmath>

constexpr int PaintingScaleFactor = 20;

//! [0]
StarRating::StarRating(int starCount, int maxStarCount)
    : myStarCount(starCount),
      myMaxStarCount(maxStarCount)
{
    starPolygon << QPointF(1.0, 0.5);
    for (int i = 1; i < 5; ++i)
        starPolygon << QPointF(0.5 + 0.5 * std::cos(0.8 * i * 3.14),
                               0.5 + 0.5 * std::sin(0.8 * i * 3.14));

    diamondPolygon << QPointF(0.4, 0.5) << QPointF(0.5, 0.4)
                   << QPointF(0.6, 0.5) << QPointF(0.5, 0.6)
                   << QPointF(0.4, 0.5);
}
//! [0]

//! [1]
QSize StarRating::sizeHint() const
{
    return PaintingScaleFactor * QSize(myMaxStarCount, 1);
}
//! [1]

//! [2]
void StarRating::paint(QPainter *painter, const QRect &rect,
                       const QPalette &palette, EditMode mode) const
{
    painter->save();

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);
    painter->setBrush(mode == EditMode::Editable ?
                          palette.highlight() :
                          palette.windowText());

    const int yOffset = (rect.height() - PaintingScaleFactor) / 2;
    painter->translate(rect.x(), rect.y() + yOffset);
    painter->scale(PaintingScaleFactor, PaintingScaleFactor);

    for (int i = 0; i < myMaxStarCount; ++i) {
        if (i < myStarCount)
            painter->drawPolygon(starPolygon, Qt::WindingFill);
        else if (mode == EditMode::Editable)
            painter->drawPolygon(diamondPolygon, Qt::WindingFill);
        painter->translate(1.0, 0.0);
    }

    painter->restore();
}
//! [2]
