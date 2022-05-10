// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef KNOB_H
#define KNOB_H

#include <QGraphicsItem>

class Knob : public QGraphicsEllipseItem
{
public:
    Knob();

    bool sceneEvent(QEvent *event) override;
};

#endif // KNOB_H
