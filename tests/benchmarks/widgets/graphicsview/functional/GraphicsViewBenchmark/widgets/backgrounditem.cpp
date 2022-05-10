// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    Q_UNUSED(widget);
    painter->setCompositionMode(QPainter::CompositionMode_Source);
    painter->drawPixmap(option->exposedRect, m_background, option->exposedRect);
}

void BackgroundItem::themeChange()
{
    m_background = Theme::p()->pixmap(m_fileName, size().toSize());
    update();
}

