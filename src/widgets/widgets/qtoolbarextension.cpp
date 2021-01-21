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

#include "qtoolbarextension_p.h"
#include <qevent.h>
#include <qstyle.h>
#include <qstylepainter.h>
#include <qstyleoption.h>

QT_BEGIN_NAMESPACE

QToolBarExtension::QToolBarExtension(QWidget *parent)
    : QToolButton(parent)
    , m_orientation(Qt::Horizontal)
{
    setObjectName(QLatin1String("qt_toolbar_ext_button"));
    setAutoRaise(true);
    setOrientation(m_orientation);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setCheckable(true);
}

void QToolBarExtension::setOrientation(Qt::Orientation o)
{
    QStyleOption opt;
    opt.init(this);
    if (o == Qt::Horizontal) {
        setIcon(style()->standardIcon(QStyle::SP_ToolBarHorizontalExtensionButton, &opt));
    } else {
        setIcon(style()->standardIcon(QStyle::SP_ToolBarVerticalExtensionButton, &opt));
    }
    m_orientation = o;
}

void QToolBarExtension::paintEvent(QPaintEvent *)
{
    QStylePainter p(this);
    QStyleOptionToolButton opt;
    initStyleOption(&opt);
    // We do not need to draw both extension arrows
    opt.features &= ~QStyleOptionToolButton::HasMenu;
    p.drawComplexControl(QStyle::CC_ToolButton, opt);
}


QSize QToolBarExtension::sizeHint() const
{
    QStyleOption opt;
    opt.initFrom(this);
    const int ext = style()->pixelMetric(QStyle::PM_ToolBarExtensionExtent, &opt);
    return QSize(ext, ext);
}

bool QToolBarExtension::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::LayoutDirectionChange:
        setOrientation(m_orientation);
        break;
    default:
        break;
    }
    return QToolButton::event(event);
}

QT_END_NAMESPACE

#include "moc_qtoolbarextension_p.cpp"
