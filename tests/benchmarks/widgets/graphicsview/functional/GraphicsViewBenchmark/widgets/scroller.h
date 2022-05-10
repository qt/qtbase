// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef SCROLLER_H
#define SCROLLER_H

#include <QObject>

class ScrollerPrivate;
class AbstractScrollArea;

class Scroller : public QObject
{
    Q_OBJECT

public:

    Scroller(QObject *parent = nullptr);
    virtual ~Scroller();

public:

    void setScrollable(AbstractScrollArea *area);
    void setScrollFactor(qreal scrollFactor);
    void stopScrolling();

private:

    bool eventFilter(QObject *obj, QEvent *ev);

private:

    Q_DECLARE_PRIVATE(Scroller)
    Q_DISABLE_COPY(Scroller)

    Q_PRIVATE_SLOT(d_ptr, void updateScrolling())

    ScrollerPrivate * const d_ptr;
};

#endif // SCROLLER_H
