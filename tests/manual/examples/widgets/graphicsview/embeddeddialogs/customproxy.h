// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CUSTOMPROXY_H
#define CUSTOMPROXY_H

#include <QTimeLine>
#include <QGraphicsProxyWidget>

class CustomProxy : public QGraphicsProxyWidget
{
    Q_OBJECT

public:
    explicit CustomProxy(QGraphicsItem *parent = nullptr, Qt::WindowFlags wFlags = { });

    QRectF boundingRect() const override;
    void paintWindowFrame(QPainter *painter, const QStyleOptionGraphicsItem *option,
                          QWidget *widget) override;

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private slots:
    void updateStep(qreal step);
    void stateChanged(QTimeLine::State);
    void zoomIn();
    void zoomOut();

private:
    QTimeLine *timeLine;
    QGraphicsItem *currentPopup = nullptr;
    bool popupShown = false;
};

#endif // CUSTOMPROXY_H
