#ifndef RECTBUTTON_H
#define RECTBUTTON_H

#include <QGraphicsObject>

class RectButton : public QGraphicsObject
{
    Q_OBJECT
public:
    RectButton(QString buttonText);
    ~RectButton();

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

protected:
    QString m_ButtonText;

    virtual void mousePressEvent (QGraphicsSceneMouseEvent *event);

signals:
    void clicked();
};

#endif // RECTBUTTON_H
