// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QVncScreen_H
#define QVncScreen_H

#include <QtFbSupport/private/qfbscreen_p.h>

QT_BEGIN_NAMESPACE

class QPainter;
class QFbCursor;
class QTcpSocket;
class QVncServer;
class QVncDirtyMap;
class QVncClientCursor;
class QVncClient;

class QVncScreen : public QFbScreen
{
    Q_OBJECT
public:
    QVncScreen(const QStringList &args);
    ~QVncScreen();

    bool initialize() override;

    QPixmap grabWindow(WId wid, int x, int y, int width, int height) const override;

    QRegion doRedraw() override;
    QImage *image() { return &mScreenImage; }

    void enableClientCursor(QVncClient *client);
    void disableClientCursor(QVncClient *client);
    QPlatformCursor *cursor() const override;

    Flags flags() const override;

    void clearDirty() { dirtyRegion = QRegion(); }

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    bool swapBytes() const;
#endif

    QStringList mArgs;

    qreal dpiX = 96;
    qreal dpiY = 96;
    QVncDirtyMap *dirty = nullptr;
    QRegion dirtyRegion;
    int refreshRate = 30;
    QVncServer *vncServer = nullptr;
#if QT_CONFIG(cursor)
    QVncClientCursor *clientCursor = nullptr;
#endif
};

QT_END_NAMESPACE

#endif // QVncScreen_H

