/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef SCROLLER_H
#define SCROLLER_H

#include <QObject>

class ScrollerPrivate;
class AbstractScrollArea;

class Scroller : public QObject
{
    Q_OBJECT

public:

    Scroller(QObject *parent = 0);
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
