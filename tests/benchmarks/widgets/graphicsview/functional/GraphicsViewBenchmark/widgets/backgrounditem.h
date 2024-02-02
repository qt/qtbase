// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef __BACKGROUNDITEM_H__
#define __BACKGROUNDITEM_H__

#include <QPixmap>

#include "gvbwidget.h"

class QGraphicsWidget;

class BackgroundItem : public GvbWidget
{
    Q_OBJECT

public:
    BackgroundItem(const QString &filename, QGraphicsWidget *parent=0);
    ~BackgroundItem();

    void paint(QPainter *painter,
                const QStyleOptionGraphicsItem *option,
                QWidget *widget = 0);
    void resizeEvent(QGraphicsSceneResizeEvent *event);

public slots:
    void themeChange();

private:
    QPixmap m_background;
    QString m_fileName;
};

#endif /* __BACKGROUNDITEM_H__ */
