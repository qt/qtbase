/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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
