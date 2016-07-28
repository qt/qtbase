/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
