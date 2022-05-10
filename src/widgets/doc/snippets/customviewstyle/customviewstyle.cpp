// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>

#include "../customstyle/customstyle.h"

void CustomStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                                QPainter *painter, const QWidget *widget) const
{

//![0]
    switch (element) {
        case (PE_PanelItemViewItem): {
            painter->save();

            QPoint topLeft = option->rect.topLeft();
            QPoint bottomRight = option->rect.topRight();
            QLinearGradient backgroundGradient(topLeft, bottomRight);
            backgroundGradient.setColorAt(0.0, QColor(Qt::yellow).lighter(190));
            backgroundGradient.setColorAt(1.0, Qt::white);
            painter->fillRect(option->rect, QBrush(backgroundGradient));

            painter->restore();
        break;
        }
        default:
            QProxyStyle::drawPrimitive(element, option, painter, widget);
    }
//![0]
}
