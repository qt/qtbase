// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtoolbarextension_p.h"
#include <qevent.h>
#include <qstyle.h>
#include <qstylepainter.h>
#include <qstyleoption.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QToolBarExtension::QToolBarExtension(QWidget *parent)
    : QToolButton(parent)
    , m_orientation(Qt::Horizontal)
{
    setObjectName("qt_toolbar_ext_button"_L1);
    setAutoRaise(true);
    setOrientation(m_orientation);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setCheckable(true);
}

void QToolBarExtension::setOrientation(Qt::Orientation o)
{
    QStyleOption opt;
    opt.initFrom(this);
    if (o == Qt::Horizontal) {
        setIcon(style()->standardIcon(QStyle::SP_ToolBarHorizontalExtensionButton, &opt, this));
    } else {
        setIcon(style()->standardIcon(QStyle::SP_ToolBarVerticalExtensionButton, &opt, this));
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
    const int ext = style()->pixelMetric(QStyle::PM_ToolBarExtensionExtent, &opt, this);
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
