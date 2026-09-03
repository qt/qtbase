// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>

//! [0]
class CheckBoxStyle : public QProxyStyle
{
public:
    using QProxyStyle::QProxyStyle;

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                       QPainter *painter, const QWidget *widget) const override;
};
//! [0]

//! [1]
void CheckBoxStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                                  QPainter *painter, const QWidget *widget) const
{
    if (element != PE_IndicatorCheckBox) {
        QProxyStyle::drawPrimitive(element, option, painter, widget);
        return;
    }

    const bool enabled = option->state & State_Enabled;
    const QPalette::ColorGroup group = !enabled ? QPalette::Disabled
                                     : option->state & State_Active ? QPalette::Active
                                                                    : QPalette::Inactive;
    const QPalette &palette = option->palette;
    const QRect rect = option->rect.adjusted(1, 1, -1, -1);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // Frame and background: highlight the frame while the mouse hovers over
    // the indicator, and darken the background while it's pressed.
    QColor frameColor = palette.color(group, QPalette::Mid);
    if (enabled && (option->state & State_MouseOver))
        frameColor = palette.color(group, QPalette::Highlight);
    const QPalette::ColorRole fillRole =
            option->state & State_Sunken ? QPalette::Mid : QPalette::Base;
    painter->setPen(frameColor);
    painter->setBrush(palette.brush(group, fillRole));
    painter->drawRoundedRect(rect, 2, 2);

    // Check mark for State_On, a filled square for the partially checked
    // State_NoChange, nothing for State_Off.
    const QRect inner = rect.adjusted(3, 3, -3, -3);
    if (option->state & State_On) {
        painter->setPen(QPen(palette.color(group, QPalette::Text), 2));
        painter->drawLine(inner.left(), inner.center().y(),
                          inner.center().x(), inner.bottom());
        painter->drawLine(inner.center().x(), inner.bottom(),
                          inner.right(), inner.top());
    } else if (option->state & State_NoChange) {
        painter->fillRect(inner, palette.brush(group, QPalette::Text));
    }

    painter->restore();
}
//! [1]

//! [2]
int main(int argc, char *argv[])
{
    QApplication::setStyle(new CheckBoxStyle);
    QApplication app(argc, argv);
    QCheckBox box("Send me updates");
    box.show();
    return app.exec();
}
//! [2]
