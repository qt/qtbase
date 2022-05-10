// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef FLICKABLE_H
#define FLICKABLE_H

#include <QWidget>

class FlickableTicker;
class FlickablePrivate;

class Flickable
{
public:

    Flickable();
    virtual ~Flickable();

    void setThreshold(int threshold);
    int threshold() const;

    void setAcceptMouseClick(QWidget *target);

    void handleMousePress(QMouseEvent *event);
    void handleMouseMove(QMouseEvent *event);
    void handleMouseRelease(QMouseEvent *event);

protected:
    virtual QPoint scrollOffset() const = 0;
    virtual void setScrollOffset(const QPoint &offset) = 0;

private:
    void tick();

private:
    FlickablePrivate *d;
    friend class FlickableTicker;
};

#endif // FLICKABLE_H
