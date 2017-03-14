/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QVNC_P_H
#define QVNC_P_H

#include "qvncscreen.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/qbytearray.h>
#include <QtCore/qvarlengtharray.h>
#include <qpa/qplatformcursor.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcVnc)

class QTcpSocket;
class QTcpServer;

class QVncScreen;
class QVncServer;
class QVncClientCursor;
class QVncClient;

// This fits with the VNC hextile messages
#define MAP_TILE_SIZE 16

class QVncDirtyMap
{
public:
    QVncDirtyMap(QVncScreen *screen);
    virtual ~QVncDirtyMap();

    void reset();
    bool dirty(int x, int y) const;
    virtual void setDirty(int x, int y, bool force = false) = 0;
    void setClean(int x, int y);

    QVncScreen *screen;
    int bytesPerPixel;
    int numDirty;
    int mapWidth;
    int mapHeight;

protected:
    uchar *map;
    uchar *buffer;
    int bufferWidth;
    int bufferHeight;
    int bufferStride;
    int numTiles;
};

template <class T>
class QVncDirtyMapOptimized : public QVncDirtyMap
{
public:
    QVncDirtyMapOptimized(QVncScreen *screen) : QVncDirtyMap(screen) {}
    ~QVncDirtyMapOptimized() {}

    void setDirty(int x, int y, bool force = false) override;
};


class QRfbRect
{
public:
    QRfbRect() {}
    QRfbRect(quint16 _x, quint16 _y, quint16 _w, quint16 _h) {
        x = _x; y = _y; w = _w; h = _h;
    }

    void read(QTcpSocket *s);
    void write(QTcpSocket *s) const;

    quint16 x;
    quint16 y;
    quint16 w;
    quint16 h;
};

class QRfbPixelFormat
{
public:
    static int size() { return 16; }

    void read(QTcpSocket *s);
    void write(QTcpSocket *s);

    int bitsPerPixel;
    int depth;
    bool bigEndian;
    bool trueColor;
    int redBits;
    int greenBits;
    int blueBits;
    int redShift;
    int greenShift;
    int blueShift;
};

class QRfbServerInit
{
public:
    QRfbServerInit() { name = 0; }
    ~QRfbServerInit() { delete[] name; }

    int size() const { return QRfbPixelFormat::size() + 8 + strlen(name); }
    void setName(const char *n);

    void read(QTcpSocket *s);
    void write(QTcpSocket *s);

    quint16 width;
    quint16 height;
    QRfbPixelFormat format;
    char *name;
};

class QRfbSetEncodings
{
public:
    bool read(QTcpSocket *s);

    quint16 count;
};

class QRfbFrameBufferUpdateRequest
{
public:
    bool read(QTcpSocket *s);

    char incremental;
    QRfbRect rect;
};

class QRfbKeyEvent
{
public:
    bool read(QTcpSocket *s);

    char down;
    int  keycode;
    int  unicode;
};

class QRfbPointerEvent
{
public:
    bool read(QTcpSocket *s);

    Qt::MouseButtons buttons;
    quint16 x;
    quint16 y;
};

class QRfbClientCutText
{
public:
    bool read(QTcpSocket *s);

    quint32 length;
};

class QRfbEncoder
{
public:
    QRfbEncoder(QVncClient *s) : client(s) {}
    virtual ~QRfbEncoder() {}

    virtual void write() = 0;

protected:
    QVncClient *client;
};

class QRfbRawEncoder : public QRfbEncoder
{
public:
    QRfbRawEncoder(QVncClient *s) : QRfbEncoder(s) {}

    void write() override;

private:
    QByteArray buffer;
};

template <class SRC> class QRfbHextileEncoder;

template <class SRC>
class QRfbSingleColorHextile
{
public:
    QRfbSingleColorHextile(QRfbHextileEncoder<SRC> *e) : encoder(e) {}
    bool read(const uchar *data, int width, int height, int stride);
    void write(QTcpSocket *socket) const;

private:
    QRfbHextileEncoder<SRC> *encoder;
};

template <class SRC>
class QRfbDualColorHextile
{
public:
    QRfbDualColorHextile(QRfbHextileEncoder<SRC> *e) : encoder(e) {}
    bool read(const uchar *data, int width, int height, int stride);
    void write(QTcpSocket *socket) const;

private:
    struct Rect {
        quint8 xy;
        quint8 wh;
    } Q_PACKED rects[8 * 16];

    quint8 numRects;
    QRfbHextileEncoder<SRC> *encoder;

private:
    inline int lastx() const { return rectx(numRects); }
    inline int lasty() const { return recty(numRects); }
    inline int rectx(int r) const { return rects[r].xy >> 4; }
    inline int recty(int r) const { return rects[r].xy & 0x0f; }
    inline int width(int r) const { return (rects[r].wh >> 4) + 1; }
    inline int height(int r) const { return (rects[r].wh & 0x0f) + 1; }

    inline void setX(int r, int x) {
        rects[r].xy = (x << 4) | (rects[r].xy & 0x0f);
    }
    inline void setY(int r, int y) {
        rects[r].xy = (rects[r].xy & 0xf0) | y;
    }
    inline void setWidth(int r, int width) {
        rects[r].wh = ((width - 1) << 4) | (rects[r].wh & 0x0f);
    }
    inline void setHeight(int r, int height) {
        rects[r].wh = (rects[r].wh & 0xf0) | (height - 1);
    }

    inline void setWidth(int width) { setWidth(numRects, width); }
    inline void setHeight(int height) { setHeight(numRects, height); }
    inline void setX(int x) { setX(numRects, x); }
    inline void setY(int y) { setY(numRects, y); }
    void next();
};

template <class SRC>
class QRfbMultiColorHextile
{
public:
    QRfbMultiColorHextile(QRfbHextileEncoder<SRC> *e) : encoder(e) {}
    bool read(const uchar *data, int width, int height, int stride);
    void write(QTcpSocket *socket) const;

private:
    inline quint8* rect(int r) {
        return rects.data() + r * (bpp + 2);
    }
    inline const quint8* rect(int r) const {
        return rects.constData() + r * (bpp + 2);
    }
    inline void setX(int r, int x) {
        quint8 *ptr = rect(r) + bpp;
        *ptr = (x << 4) | (*ptr & 0x0f);
    }
    inline void setY(int r, int y) {
        quint8 *ptr = rect(r) + bpp;
        *ptr = (*ptr & 0xf0) | y;
    }
    void setColor(SRC color);
    inline int rectx(int r) const {
        const quint8 *ptr = rect(r) + bpp;
        return *ptr >> 4;
    }
    inline int recty(int r) const {
        const quint8 *ptr = rect(r) + bpp;
        return *ptr & 0x0f;
    }
    inline void setWidth(int r, int width) {
        quint8 *ptr = rect(r) + bpp + 1;
        *ptr = ((width - 1) << 4) | (*ptr & 0x0f);
    }
    inline void setHeight(int r, int height) {
        quint8 *ptr = rect(r) + bpp + 1;
        *ptr = (*ptr & 0xf0) | (height - 1);
    }

    bool beginRect();
    void endRect();

    static const int maxRectsSize = 16 * 16;
    QVarLengthArray<quint8, maxRectsSize> rects;

    quint8 bpp;
    quint8 numRects;
    QRfbHextileEncoder<SRC> *encoder;
};

template <class SRC>
class QRfbHextileEncoder : public QRfbEncoder
{
public:
    QRfbHextileEncoder(QVncServer *s);
    void write();

private:
    enum SubEncoding {
        Raw = 1,
        BackgroundSpecified = 2,
        ForegroundSpecified = 4,
        AnySubrects = 8,
        SubrectsColoured = 16
    };

    QByteArray buffer;
    QRfbSingleColorHextile<SRC> singleColorHextile;
    QRfbDualColorHextile<SRC> dualColorHextile;
    QRfbMultiColorHextile<SRC> multiColorHextile;

    SRC bg;
    SRC fg;
    bool newBg;
    bool newFg;

    friend class QRfbSingleColorHextile<SRC>;
    friend class QRfbDualColorHextile<SRC>;
    friend class QRfbMultiColorHextile<SRC>;
};

#if QT_CONFIG(cursor)
class QVncClientCursor : public QPlatformCursor
{
public:
    QVncClientCursor();
    ~QVncClientCursor();

    void write(QVncClient *client) const;

    void changeCursor(QCursor *widgetCursor, QWindow *window) override;

    void addClient(QVncClient *client);
    uint removeClient(QVncClient *client);

    QImage cursor;
    QPoint hotspot;
    QVector<QVncClient *> clients;
};
#endif // QT_CONFIG(cursor)

class QVncServer : public QObject
{
    Q_OBJECT
public:
    QVncServer(QVncScreen *screen, quint16 port = 5900);
    ~QVncServer();

    enum ServerMsg { FramebufferUpdate = 0,
                     SetColourMapEntries = 1 };

    void setDirty();


    inline QVncScreen* screen() const { return qvnc_screen; }
    inline QVncDirtyMap* dirtyMap() const { return qvnc_screen->dirty; }
    QImage screenImage() const;
    void discardClient(QVncClient *client);

private slots:
    void newConnection();
    void init();

private:
    QTcpServer *serverSocket;
    QVector<QVncClient*> clients;
    QVncScreen *qvnc_screen;
    quint16 m_port;
};

QT_END_NAMESPACE

#endif
