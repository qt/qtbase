// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#undef QT_NO_FOREACH // this file contains unported legacy Q_FOREACH uses

#include "itemwindow.h"

#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>

void TextItem::paint(QPainter &painter)
{
    painter.fillRect(m_rect, m_col);
    painter.drawRect(m_rect);
    QTextOption textOption;
    textOption.setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    painter.drawText(m_rect, m_text, textOption);
}

void ButtonItem::mouseEvent(QMouseEvent *mouseEvent)
{
    if (mouseEvent->type() == QEvent::MouseButtonPress && rect().contains(mouseEvent->pos())) {
        mouseEvent->accept();
        emit clicked();
    }
}

void ButtonItem::keyEvent(QKeyEvent *keyEvent)
{
    if (m_shortcut && keyEvent->type() == QEvent::KeyPress
        && (keyEvent->key() + int(keyEvent->modifiers())) == m_shortcut) {
        keyEvent->accept();
        emit clicked();
    }
}

void ItemWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QRect rect(QPoint(0, 0), size());
    painter.fillRect(rect, m_background);
    foreach (Item *i, m_items)
        i->paint(painter);
}
