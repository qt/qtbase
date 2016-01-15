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
        return QSizeF(fm.width(m_textItem->text()), fm.height());
    }
    default:
        return GvbWidget::sizeHint(which, constraint);
    }
}
