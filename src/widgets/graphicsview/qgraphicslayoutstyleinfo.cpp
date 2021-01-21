/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qgraphicslayoutstyleinfo_p.h"

#include "qgraphicslayout_p.h"
#include "qgraphicswidget.h"
#include <QtWidgets/qstyle.h>
#include <QtWidgets/qwidget.h>
#include <QtWidgets/qapplication.h>

QT_BEGIN_NAMESPACE

QGraphicsLayoutStyleInfo::QGraphicsLayoutStyleInfo(const QGraphicsLayoutPrivate *layout)
    : m_layout(layout), m_style(nullptr)
{
    m_widget.reset(new QWidget); // pixelMetric might need a widget ptr
    m_styleOption.initFrom(m_widget.get());
    m_isWindow = m_styleOption.state & QStyle::State_Window;
}

QGraphicsLayoutStyleInfo::~QGraphicsLayoutStyleInfo()
{
}

qreal QGraphicsLayoutStyleInfo::combinedLayoutSpacing(QLayoutPolicy::ControlTypes controls1,
                                                      QLayoutPolicy::ControlTypes controls2,
                                                      Qt::Orientation orientation) const
{
    Q_ASSERT(style());
    return style()->combinedLayoutSpacing(QSizePolicy::ControlTypes(int(controls1)), QSizePolicy::ControlTypes(int(controls2)),
                                          orientation, const_cast<QStyleOption*>(&m_styleOption), widget());
}

qreal QGraphicsLayoutStyleInfo::perItemSpacing(QLayoutPolicy::ControlType control1,
                                               QLayoutPolicy::ControlType control2,
                                               Qt::Orientation orientation) const
{
    Q_ASSERT(style());
    return style()->layoutSpacing(QSizePolicy::ControlType(control1), QSizePolicy::ControlType(control2),
                                  orientation, const_cast<QStyleOption*>(&m_styleOption), widget());
}

qreal QGraphicsLayoutStyleInfo::spacing(Qt::Orientation orientation) const
{
    Q_ASSERT(style());
    return style()->pixelMetric(orientation == Qt::Horizontal
        ? QStyle::PM_LayoutHorizontalSpacing : QStyle::PM_LayoutVerticalSpacing,
        &m_styleOption);
}

qreal QGraphicsLayoutStyleInfo::windowMargin(Qt::Orientation orientation) const
{
    return style()->pixelMetric(orientation == Qt::Vertical
                                ? QStyle::PM_LayoutBottomMargin
                                : QStyle::PM_LayoutRightMargin,
                                const_cast<QStyleOption*>(&m_styleOption), widget());
}

QWidget *QGraphicsLayoutStyleInfo::widget() const { return m_widget.get(); }

QStyle *QGraphicsLayoutStyleInfo::style() const
{
    if (!m_style) {
        Q_ASSERT(m_layout);
        QGraphicsItem *item = m_layout->parentItem();
        m_style = (item && item->isWidget()) ? static_cast<QGraphicsWidget*>(item)->style() : QApplication::style();
    }
    return m_style;
}

QT_END_NAMESPACE
