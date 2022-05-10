// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtGui>

#include "label.h"

Label::Label(const QString &text, QGraphicsItem *parent)
    : GvbWidget(parent)
{
    m_textItem = new QGraphicsSimpleTextItem(this);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    setContentsMargins(0, 0, 0, 0);
    setText(text);
    // This flag was introduced in Qt 4.6.
    setFlag(QGraphicsItem::ItemHasNoContents, true);
}

Label::~Label()
{
}

void Label::setText(const QString &text)
{
    m_textItem->setText(text);
    prepareGeometryChange();
}

QString Label::text() const
{
    return m_textItem->text();
}

void Label::setFont(const QFont font)
{
    m_textItem->setFont(font);
}

void Label::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    GvbWidget::resizeEvent(event);
}

QSizeF Label::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
{
    switch (which) {
    case Qt::MinimumSize:
        // fall thru
    case Qt::PreferredSize: {
        QFontMetricsF fm(m_textItem->font());
        return QSizeF(fm.horizontalAdvance(m_textItem->text()), fm.height());
    }
    default:
        return GvbWidget::sizeHint(which, constraint);
    }
}
