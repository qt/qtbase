// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef ARTHURSTYLE_H
#define ARTHURSTYLE_H

#include <QCommonStyle>

QT_USE_NAMESPACE

class ArthurStyle : public QCommonStyle
{
public:
    ArthurStyle();

    void drawHoverRect(QPainter *painter, const QRect &rect) const;

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                       QPainter *painter, const QWidget *widget = nullptr) const override;
    void drawControl(ControlElement element, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget) const override;
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                            QPainter *painter, const QWidget *widget) const override;
    QSize sizeFromContents(ContentsType type, const QStyleOption *option,
                           const QSize &size, const QWidget *widget) const override;

    QRect subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const override;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt,
                         SubControl sc, const QWidget *widget) const override;

    int pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const override;

    void polish(QPalette &palette) override;
    void polish(QWidget *widget) override;
    void unpolish(QWidget *widget) override;
};

#endif
