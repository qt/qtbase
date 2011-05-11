#include "rectbutton.h"
#include <QPainter>

RectButton::RectButton(QString buttonText) : m_ButtonText(buttonText)
{
}


RectButton::~RectButton()
{
}


void RectButton::mousePressEvent (QGraphicsSceneMouseEvent *event)
{
    emit clicked();
}


QRectF RectButton::boundingRect() const
{
    return QRectF(0.0, 0.0, 90.0, 40.0);
}


void RectButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->setBrush(Qt::gray);
    painter->drawRoundedRect(boundingRect(), 5, 5);

    painter->setPen(Qt::white);
    painter->drawText(20, 25, m_ButtonText);
}
