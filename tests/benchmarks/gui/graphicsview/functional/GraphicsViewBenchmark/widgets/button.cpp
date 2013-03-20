/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>
#include "button.h"
#include "theme.h"

static const int MinTextWidthAsChars = 8;

class ButtonPrivate {
    Q_DECLARE_PUBLIC(Button)

public:

    ButtonPrivate(Button *button)
       : down(false)
       , q_ptr(button)
    {
        textItem = new QGraphicsSimpleTextItem(q_ptr);
    }

    QGraphicsSimpleTextItem *textItem;
    bool down;
    Button *q_ptr;
};

Button::Button(const QString &text, QGraphicsItem *parent, QSizeF minimumSize)
    : QGraphicsWidget(parent)
    , d_ptr(new ButtonPrivate(this)), m_background(), m_selected(false)
{
    Q_D(Button);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    //setCacheMode(QGraphicsItem::ItemCoordinateCache);
    if(minimumSize.isValid())
        setMinimumSize(minimumSize);
    setContentsMargins(0, 0, 0, 0);
    d->textItem->setText(text);
    prepareGeometryChange();

    m_font = Theme::p()->font(Theme::MenuItem);
    d->textItem->setFont(m_font);
    connect(Theme::p(), SIGNAL(themeChanged()), this, SLOT(themeChange()));
}

Button::~Button()
{
    delete d_ptr;
}

bool Button::isDown()
{
    Q_D(Button);

    return d->down;
}

void Button::setText(const QString &text)
{
    Q_D(Button);
    d->textItem->setText(text);
    update();
}

QString Button::text()
{
    Q_D(Button);
    return d->textItem->text();
}

void Button::paint(QPainter *painter,
        const QStyleOptionGraphicsItem *option,
        QWidget *widget)
{
    Q_UNUSED(widget);
    Q_UNUSED(option);

    if(!m_background.isNull())
        painter->drawPixmap(QPoint(), m_background);
    if(m_selected) {
        painter->setBrush(Qt::black);
        painter->setOpacity(0.2);
        painter->drawRect(boundingRect().toRect());
    }
}

QSizeF Button::sizeHint(Qt::SizeHint which,
        const QSizeF &constraint) const
{
    Q_D(const Button);

    switch (which)
    {
    case Qt::MinimumSize:
         {
         QFontMetricsF fm(d->textItem->font());
         return QSizeF(MinTextWidthAsChars * fm.maxWidth(), fm.height());
         }
     case Qt::PreferredSize:
         {
         QFontMetricsF fm(d->textItem->font());
         return QSizeF(fm.width(d->textItem->text()), fm.height());
         }
    default:
        return QGraphicsWidget::sizeHint(which, constraint);
    }
}

void Button::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(Button);

    if (event->button() != Qt::LeftButton ||
                  !sceneBoundingRect().contains(event->scenePos()))
        return;

    d->down = true;

    prepareGeometryChange();
    emit pressed();

}

void Button::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_D(Button);

    if (!d->down || event->button() != Qt::LeftButton)
        return;

    d->down = false;

    prepareGeometryChange();

    emit released();

    if (sceneBoundingRect().contains(event->scenePos()))
        emit clicked();
}

void Button::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
}

void Button::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    Q_D(Button);
    QGraphicsWidget::resizeEvent(event);

    QRectF rect = d->textItem->boundingRect();
    QRectF buttonrect = this->boundingRect();
    d->textItem->setPos((buttonrect.width() - rect.width())/2, (buttonrect.height() - rect.height())/2 );

    QSize currentSize = buttonrect.size().toSize();
    if( m_background.size() != currentSize && (currentSize.width() > 0 && currentSize.height() > 0)  ) {
        m_background = Theme::p()->pixmap("status_field_middle.svg", buttonrect.size().toSize());
    }
}

void Button::setBackground(QPixmap& background)
{
    m_background = background;
}

void Button::themeChange()
{
    Q_D(Button);

    m_font = Theme::p()->font(Theme::MenuItem);
    d->textItem->setFont(m_font);
}
