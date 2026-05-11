// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSSTYLE_P_H
#define QOHOSSTYLE_P_H

#include <QCommonStyle>
#include <QObject>

QT_BEGIN_NAMESPACE

class Q_WIDGETS_EXPORT QOhosStyle : public QCommonStyle
{
    Q_OBJECT

public:
    QOhosStyle();
    ~QOhosStyle() = default;

    void drawPrimitive(
        PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const override;
    void drawControl(
        ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const override;
    void drawComplexControl(
        ComplexControl control, const QStyleOptionComplex *option, QPainter *painter,
        const QWidget *widget) const override;
    QRect subControlRect(
        ComplexControl control, const QStyleOptionComplex *option, SubControl subControl,
        const QWidget *widget) const override;
    QRect subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const override;
    QSize sizeFromContents(
        ContentsType contents, const QStyleOption *option, const QSize &contentsSize,
        const QWidget *widget) const override;
    int pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const override;
    int styleHint(
        StyleHint hint, const QStyleOption *option, const QWidget *widget,
        QStyleHintReturn *hintReturn) const override;

    void polish(QWidget *widget) override;
    void unpolish(QWidget *widget) override;

    QPalette standardPalette() const override;

private:
    Q_DISABLE_COPY(QOhosStyle)
};

QT_END_NAMESPACE

#endif
