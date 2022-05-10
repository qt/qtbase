// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SLIPPYMAP_H
#define SLIPPYMAP_H

#include <QNetworkAccessManager>
#include <QPixmap>
#include <QUrl>

QT_BEGIN_NAMESPACE
class QNetworkReply;
class QPainter;
QT_END_NAMESPACE

class SlippyMap: public QObject
{
    Q_OBJECT

public:
    SlippyMap(QObject *parent = nullptr);
    void invalidate();
    void render(QPainter *p, const QRect &rect);
    void pan(const QPoint &delta);

    int width;
    int height;
    int zoom;
    qreal latitude;
    qreal longitude;

signals:
    void updated(const QRect &rect);

private slots:
    void handleNetworkData(QNetworkReply *reply);
    void download();

protected:
    QRect tileRect(const QPoint &tp);

private:
    QPoint m_offset;
    QRect m_tilesRect;
    QPixmap m_emptyTile;
    QHash<QPoint, QPixmap> m_tilePixmaps;
    QNetworkAccessManager m_manager;
    QUrl m_url;
};

#endif

