// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef IMAGEITEM_H
#define IMAGEITEM_H

#include <QtCore>
#include <QtWidgets/QGraphicsPixmapItem>

//! [0]
class ImageItem : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT
public:
    enum { Type = UserType + 1 };

    ImageItem(int id, const QPixmap &pixmap, QGraphicsItem *parent = nullptr);

    int type() const override { return Type; }
    void adjust();
    int id() const;

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private Q_SLOTS:
    void setFrame(int frame);
    void updateItemPosition();

private:
    QTimeLine timeLine;
    int recordId;
    double z;
};
//! [0]

#endif
