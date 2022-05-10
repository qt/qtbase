// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CUSTOMSTYLE_H
#define CUSTOMSTYLE_H

#include <QProxyStyle>

//! [0]
class CustomStyle : public QProxyStyle
{
    Q_OBJECT

public:
    CustomStyle(const QWidget *widget);
    ~CustomStyle() {}

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                       QPainter *painter, const QWidget *widget) const override;
};
//! [0]

#endif
