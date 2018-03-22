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

#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneResizeEvent>
#include <QPainter>
#include <QRectF>

#include "backgrounditem.h"
#include "theme.h"

BackgroundItem::BackgroundItem(const QString &filename, QGraphicsWidget *parent)
    : GvbWidget(parent),
      m_background(),
      m_fileName(filename)
{
    setContentsMargins(0,0,0,0);

    connect(Theme::p(), SIGNAL(themeChanged()), this, SLOT(themeChange()));
}

BackgroundItem::~BackgroundItem()
{
}

void BackgroundItem::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    GvbWidget::resizeEvent(event);
    m_background = Theme::p()->pixmap(m_fileName, size().toSize());
}

void BackgroundItem::paint(QPainter *painter,
                           const QStyleOptionGraphicsItem *option,
                           QWidget *widget)
{
    Q_UNUSED(widget)
    painter->setCompositionMode(QPainter::CompositionMode_Source);
    painter->drawPixmap(option->exposedRect, m_background, option->exposedRect);
}

void BackgroundItem::themeChange()
{
    m_background = Theme::p()->pixmap(m_fileName, size().toSize());
    update();
}

