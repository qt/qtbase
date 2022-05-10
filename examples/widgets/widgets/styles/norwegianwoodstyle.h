// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef NORWEGIANWOODSTYLE_H
#define NORWEGIANWOODSTYLE_H

#include <QProxyStyle>
#include <QPalette>

QT_BEGIN_NAMESPACE
class QPainterPath;
QT_END_NAMESPACE

//! [0]
class NorwegianWoodStyle : public QProxyStyle
{
    Q_OBJECT

public:
    NorwegianWoodStyle();

    QPalette standardPalette() const override;

    void polish(QWidget *widget) override;
    void unpolish(QWidget *widget) override;
    int pixelMetric(PixelMetric metric, const QStyleOption *option,
                    const QWidget *widget) const override;
    int styleHint(StyleHint hint, const QStyleOption *option,
                  const QWidget *widget, QStyleHintReturn *returnData) const override;
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                       QPainter *painter, const QWidget *widget) const override;
    void drawControl(ControlElement control, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget) const override;

private:
    static void setTexture(QPalette &palette, QPalette::ColorRole role,
                           const QImage &image);
    static QPainterPath roundRectPath(const QRect &rect);
    mutable QPalette m_standardPalette;
};
//! [0]

#endif
