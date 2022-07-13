// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>

#include "customstyle.h"

CustomStyle::CustomStyle(const QWidget *widget)
{
//! [0]
    const QSpinBox *spinBox = qobject_cast<const QSpinBox *>(widget);
    if (spinBox) {
//! [0] //! [1]
    }
//! [1]
}

//! [2]
void CustomStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                                QPainter *painter, const QWidget *widget) const
{
    if (element == PE_IndicatorSpinUp || element == PE_IndicatorSpinDown) {
        QPolygon points(3);
        int x = option->rect.x();
        int y = option->rect.y();
        int w = option->rect.width() / 2;
        int h = option->rect.height() / 2;
        x += (option->rect.width() - w) / 2;
        y += (option->rect.height() - h) / 2;

        if (element == PE_IndicatorSpinUp) {
            points[0] = QPoint(x, y + h);
            points[1] = QPoint(x + w, y + h);
            points[2] = QPoint(x + w / 2, y);
        } else { // PE_SpinBoxDown
            points[0] = QPoint(x, y);
            points[1] = QPoint(x + w, y);
            points[2] = QPoint(x + w / 2, y + h);
        }

        if (option->state & State_Enabled) {
            painter->setPen(option->palette.mid().color());
            painter->setBrush(option->palette.buttonText());
        } else {
            painter->setPen(option->palette.buttonText().color());
            painter->setBrush(option->palette.mid());
        }
        painter->drawPolygon(points);
    } else {
        QProxyStyle::drawPrimitive(element, option, painter, widget);
//! [2] //! [3]
    }
//! [3] //! [4]
}
//! [4]
