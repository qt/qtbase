// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtoolbarseparator_p.h"

#include <qstyle.h>
#include <qstyleoption.h>
#include <qtoolbar.h>
#include <qpainter.h>

QT_BEGIN_NAMESPACE

void QToolBarSeparator::initStyleOption(QStyleOption *option) const
{
    option->initFrom(this);
    if (orientation() == Qt::Horizontal)
        option->state |= QStyle::State_Horizontal;
}

QToolBarSeparator::QToolBarSeparator(QToolBar *parent)
    : QWidget(parent), orient(parent->orientation())
{ setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum); }

void QToolBarSeparator::setOrientation(Qt::Orientation orientation)
{
    orient = orientation;
    update();
}

Qt::Orientation QToolBarSeparator::orientation() const
{ return orient; }

QSize QToolBarSeparator::sizeHint() const
{
    QStyleOption opt;
    initStyleOption(&opt);
    const int extent = style()->pixelMetric(QStyle::PM_ToolBarSeparatorExtent, &opt, parentWidget());
    return QSize(extent, extent);
}

void QToolBarSeparator::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    QStyleOption opt;
    initStyleOption(&opt);
    style()->drawPrimitive(QStyle::PE_IndicatorToolBarSeparator, &opt, &p, parentWidget());
}

QT_END_NAMESPACE

#include "moc_qtoolbarseparator_p.cpp"
