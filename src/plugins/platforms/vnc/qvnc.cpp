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
#include "qvnc_p.h"
#include "qvncscreen.h"
#include "QtNetwork/qtcpserver.h"
#include "QtNetwork/qtcpsocket.h"
#include <qpa/qwindowsysteminterface.h>
#include <QtGui/qguiapplication.h>
#include <qthread.h>

#ifdef Q_OS_WIN
#include <Winsock2.h>
#else
#include <arpa/inet.h>
#endif

QVncDirtyMap::QVncDirtyMap(QVncScreen *screen)
    : screen(screen), bytesPerPixel(0), numDirty(0)
{
    bytesPerPixel = (screen->depth() + 7) / 8;
    bufferWidth = screen->geometry().width();
    bufferHeight = screen->geometry().height();
    bufferStride = bufferWidth * bytesPerPixel;
    buffer = new uchar[bufferHeight * bufferStride];

    mapWidth = (bufferWidth + MAP_TILE_SIZE - 1) / MAP_TILE_SIZE;
    mapHeight = (bufferHeight + MAP_TILE_SIZE - 1) / MAP_TILE_SIZE;
    numTiles = mapWidth * mapHeight;
    map = new uchar[numTiles];
}

QVncDirtyMap::~QVncDirtyMap()
{
    delete[] map;
    delete[] buffer;
}

void QVncDirtyMap::reset()
{
    memset(map, 1, numTiles);
    memset(buffer, 0, bufferHeight * bufferStride);
    numDirty = numTiles;
}

inline bool QVncDirtyMap::dirty(int x, int y) const
{
    return map[y * mapWidth + x];
}

inline void QVncDirtyMap::setClean(int x, int y)
{
    map[y * mapWidth + x] = 0;
    --numDirty;
}

template <class T>
void QVncDirtyMapOptimized<T>::setDirty(int tileX, int tileY, bool force)
{
    static bool alwaysForce = qEnvironmentVariableIsSet("QT_VNC_NO_COMPAREBUFFER");
    if (alwaysForce)
        force = true;

    bool changed = false;

    if (!force) {
        const int lstep = bufferStride;
        const int startX = tileX * MAP_TILE_SIZE;
        const int startY = tileY * MAP_TILE_SIZE;
        const uchar *scrn = screen->image()->constBits()
                            + startY * lstep + startX * bytesPerPixel;
        uchar *old = buffer + startY * bufferStride + startX * sizeof(T);

        const int tileHeight = (startY + MAP_TILE_SIZE > bufferHeight ?
                                bufferHeight - startY : MAP_TILE_SIZE);
        const int tileWidth = (startX + MAP_TILE_SIZE > bufferWidth ?
                               bufferWidth - startX : MAP_TILE_SIZE);
        const bool doInlines = (tileWidth == MAP_TILE_SIZE);

        int y = tileHeight;

        if (doInlines) { // hw: memcmp/memcpy is inlined when using constants
            while (y) {
                if (memcmp(old, scrn, sizeof(T) * MAP_TILE_SIZE)) {
                    changed = true;
                    break;
                }
                scrn += lstep;
                old += bufferStride;
                --y;
            }

            while (y) {
                memcpy(old, scrn, sizeof(T) * MAP_TILE_SIZE);
                scrn += lstep;
                old += bufferStride;
                --y;
            }
        } else {
            while (y) {
                if (memcmp(old, scrn, sizeof(T) * tileWidth)) {
                    changed = true;
                    break;
                }
                scrn += lstep;
                old += bufferStride;
                --y;
            }

            while (y) {
                memcpy(old, scrn, sizeof(T) * tileWidth);
                scrn += lstep;
                old += bufferStride;
                --y;
            }
        }
    }

    const int mapIndex = tileY * mapWidth + tileX;
    if ((force || changed) && !map[mapIndex]) {
        map[mapIndex] = 1;
        ++numDirty;
    }
}

template class QVncDirtyMapOptimized<unsigned char>;
template class QVncDirtyMapOptimized<unsigned short>;
template class QVncDirtyMapOptimized<unsigned int>;

static const struct {
    int keysym;
    int keycode;
} keyMap[] = {
    { 0xff08, Qt::Key_Backspace },
    { 0xff09, Qt::Key_Tab       },
    { 0xff0d, Qt::Key_Return    },
    { 0xff1b, Qt::Key_Escape    },
    { 0xff63, Qt::Key_Insert    },
    { 0xffff, Qt::Key_Delete    },
    { 0xff50, Qt::Key_Home      },
    { 0xff57, Qt::Key_End       },
    { 0xff55, Qt::Key_PageUp    },
    { 0xff56, Qt::Key_PageDown  },
    { 0xff51, Qt::Key_Left      },
    { 0xff52, Qt::Key_Up        },
    { 0xff53, Qt::Key_Right     },
    { 0xff54, Qt::Key_Down      },
    { 0xffbe, Qt::Key_F1        },
    { 0xffbf, Qt::Key_F2        },
    { 0xffc0, Qt::Key_F3        },
    { 0xffc1, Qt::Key_F4        },
    { 0xffc2, Qt::Key_F5        },
    { 0xffc3, Qt::Key_F6        },
    { 0xffc4, Qt::Key_F7        },
    { 0xffc5, Qt::Key_F8        },
    { 0xffc6, Qt::Key_F9        },
    { 0xffc7, Qt::Key_F10       },
    { 0xffc8, Qt::Key_F11       },
    { 0xffc9, Qt::Key_F12       },
    { 0xffe1, Qt::Key_Shift     },
    { 0xffe2, Qt::Key_Shift     },
    { 0xffe3, Qt::Key_Control   },
    { 0xffe4, Qt::Key_Control   },
    { 0xffe7, Qt::Key_Meta      },
    { 0xffe8, Qt::Key_Meta      },
    { 0xffe9, Qt::Key_Alt       },
    { 0xffea, Qt::Key_Alt       },

    { 0xffb0, Qt::Key_0         },
    { 0xffb1, Qt::Key_1         },
    { 0xffb2, Qt::Key_2         },
    { 0xffb3, Qt::Key_3         },
    { 0xffb4, Qt::Key_4         },
    { 0xffb5, Qt::Key_5         },
    { 0xffb6, Qt::Key_6         },
    { 0xffb7, Qt::Key_7         },
    { 0xffb8, Qt::Key_8         },
    { 0xffb9, Qt::Key_9         },

    { 0xff8d, Qt::Key_Return    },
    { 0xffaa, Qt::Key_Asterisk  },
    { 0xffab, Qt::Key_Plus      },
    { 0xffad, Qt::Key_Minus     },
    { 0xffae, Qt::Key_Period    },
    { 0xffaf, Qt::Key_Slash     },

    { 0xff95, Qt::Key_Home      },
    { 0xff96, Qt::Key_Left      },
    { 0xff97, Qt::Key_Up        },
    { 0xff98, Qt::Key_Right     },
    { 0xff99, Qt::Key_Down      },
    { 0xff9a, Qt::Key_PageUp    },
    { 0xff9b, Qt::Key_PageDown  },
    { 0xff9c, Qt::Key_End       },
    { 0xff9e, Qt::Key_Insert    },
    { 0xff9f, Qt::Key_Delete    },

    { 0, 0 }
};

void QRfbRect::read(QTcpSocket *s)
{
    quint16 buf[4];
    s->read((char*)buf, 8);
    x = ntohs(buf[0]);
    y = ntohs(buf[1]);
    w = ntohs(buf[2]);
    h = ntohs(buf[3]);
}

void QRfbRect::write(QTcpSocket *s) const
{
    quint16 buf[4];
    buf[0] = htons(x);
    buf[1] = htons(y);
    buf[2] = htons(w);
    buf[3] = htons(h);
    s->write((char*)buf, 8);
}

void QRfbPixelFormat::read(QTcpSocket *s)
{
    char buf[16];
    s->read(buf, 16);
    bitsPerPixel = buf[0];
    depth = buf[1];
    bigEndian = buf[2];
    trueColor = buf[3];

    quint16 a = ntohs(*(quint16 *)(buf + 4));
    redBits = 0;
    while (a) { a >>= 1; redBits++; }

    a = ntohs(*(quint16 *)(buf + 6));
    greenBits = 0;
    while (a) { a >>= 1; greenBits++; }

    a = ntohs(*(quint16 *)(buf + 8));
    blueBits = 0;
    while (a) { a >>= 1; blueBits++; }

    redShift = buf[10];
    greenShift = buf[11];
    blueShift = buf[12];
}

void QRfbPixelFormat::write(QTcpSocket *s)
{
    char buf[16];
    buf[0] = bitsPerPixel;
    buf[1] = depth;
    buf[2] = bigEndian;
    buf[3] = trueColor;

    quint16 a = 0;
    for (int i = 0; i < redBits; i++) a = (a << 1) | 1;
    *(quint16 *)(buf + 4) = htons(a);

    a = 0;
    for (int i = 0; i < greenBits; i++) a = (a << 1) | 1;
    *(quint16 *)(buf + 6) = htons(a);

    a = 0;
    for (int i = 0; i < blueBits; i++) a = (a << 1) | 1;
    *(quint16 *)(buf + 8) = htons(a);

    buf[10] = redShift;
    buf[11] = greenShift;
    buf[12] = blueShift;
    s->write(buf, 16);
}


void QRfbServerInit::setName(const char *n)
{
    delete[] name;
    name = new char [strlen(n) + 1];
    strcpy(name, n);
}

void QRfbServerInit::read(QTcpSocket *s)
{
    s->read((char *)&width, 2);
    width = ntohs(width);
    s->read((char *)&height, 2);
    height = ntohs(height);
    format.read(s);

    quint32 len;
    s->read((char *)&len, 4);
    len = ntohl(len);

    name = new char [len + 1];
    s->read(name, len);
    name[len] = '\0';
}

void QRfbServerInit::write(QTcpSocket *s)
{
    quint16 t = htons(width);
    s->write((char *)&t, 2);
    t = htons(height);
    s->write((char *)&t, 2);
    format.write(s);
    quint32 len = strlen(name);
    len = htonl(len);
    s->write((char *)&len, 4);
    s->write(name, strlen(name));
}

bool QRfbSetEncodings::read(QTcpSocket *s)
{
    if (s->bytesAvailable() < 3)
        return false;

    char tmp;
    s->read(&tmp, 1);        // padding
    s->read((char *)&count, 2);
    count = ntohs(count);

    return true;
}

bool QRfbFrameBufferUpdateRequest::read(QTcpSocket *s)
{
    if (s->bytesAvailable() < 9)
        return false;

    s->read(&incremental, 1);
    rect.read(s);

    return true;
}

bool QRfbKeyEvent::read(QTcpSocket *s)
{
    if (s->bytesAvailable() < 7)
        return false;

    s->read(&down, 1);
    quint16 tmp;
    s->read((char *)&tmp, 2);  // padding

    quint32 key;
    s->read((char *)&key, 4);
    key = ntohl(key);

    unicode = 0;
    keycode = 0;
    int i = 0;
    while (keyMap[i].keysym && !keycode) {
        if (keyMap[i].keysym == (int)key)
            keycode = keyMap[i].keycode;
        i++;
    }

    if (keycode >= ' ' && keycode <= '~')
        unicode = keycode;

    if (!keycode) {
        if (key <= 0xff) {
            unicode = key;
            if (key >= 'a' && key <= 'z')
                keycode = Qt::Key_A + key - 'a';
            else if (key >= ' ' && key <= '~')
                keycode = Qt::Key_Space + key - ' ';
        }
    }

    return true;
}

bool QRfbPointerEvent::read(QTcpSocket *s)
{
    if (s->bytesAvailable() < 5)
        return false;

    char buttonMask;
    s->read(&buttonMask, 1);
    buttons = Qt::NoButton;
    if (buttonMask & 1)
        buttons |= Qt::LeftButton;
    if (buttonMask & 2)
        buttons |= Qt::MidButton;
    if (buttonMask & 4)
        buttons |= Qt::RightButton;

    quint16 tmp;
    s->read((char *)&tmp, 2);
    x = ntohs(tmp);
    s->read((char *)&tmp, 2);
    y = ntohs(tmp);

    return true;
}

bool QRfbClientCutText::read(QTcpSocket *s)
{
    if (s->bytesAvailable() < 7)
        return false;

    char tmp[3];
    s->read(tmp, 3);        // padding
    s->read((char *)&length, 4);
    length = ntohl(length);

    return true;
}

void QRfbRawEncoder::write()
{
//    QVncDirtyMap *map = server->dirtyMap();
    QTcpSocket *socket = server->clientSocket();

    const int bytesPerPixel = server->clientBytesPerPixel();

    // create a region from the dirty rects and send the region's merged rects.
    // ### use the tile map again
    QRegion rgn = server->screen()->dirtyRegion;
    server->screen()->clearDirty();
    QT_VNC_DEBUG() << "QRfbRawEncoder::write()" << rgn;
//    if (map) {
//        for (int y = 0; y < map->mapHeight; ++y) {
//            for (int x = 0; x < map->mapWidth; ++x) {
//                if (!map->dirty(x, y))
//                    continue;
//                rgn += QRect(x * MAP_TILE_SIZE, y * MAP_TILE_SIZE,
//                             MAP_TILE_SIZE, MAP_TILE_SIZE);
//                map->setClean(x, y);
//            }
//        }

//        rgn &= QRect(0, 0, server->screen()->geometry().width(),
//                     server->screen()->geometry().height());
//    }
    const QVector<QRect> rects = rgn.rects();

    {
        const char tmp[2] = { 0, 0 }; // msg type, padding
        socket->write(tmp, sizeof(tmp));
    }

    {
        const quint16 count = htons(rects.size());
        socket->write((char *)&count, sizeof(count));
    }

    if (rects.size() <= 0)
        return;

    const QImage screenImage = server->screenImage();

    for (const QRect &tileRect: rects) {
        const QRfbRect rect(tileRect.x(), tileRect.y(),
                            tileRect.width(), tileRect.height());
        rect.write(socket);

        const quint32 encoding = htonl(0); // raw encoding
        socket->write((char *)&encoding, sizeof(encoding));

        int linestep = screenImage.bytesPerLine();
        const uchar *screendata = screenImage.scanLine(rect.y)
                                  + rect.x * screenImage.depth() / 8;

        if (server->doPixelConversion()) {
            const int bufferSize = rect.w * rect.h * bytesPerPixel;
            if (bufferSize > buffer.size())
                buffer.resize(bufferSize);

            // convert pixels
            char *b = buffer.data();
            const int bstep = rect.w * bytesPerPixel;
            for (int i = 0; i < rect.h; ++i) {
                server->convertPixels(b, (const char*)screendata, rect.w);
                screendata += linestep;
                b += bstep;
            }
            socket->write(buffer.constData(), bufferSize);
        } else {
            for (int i = 0; i < rect.h; ++i) {
                socket->write((const char*)screendata, rect.w * bytesPerPixel);
                screendata += linestep;
            }
        }
        if (socket->state() == QAbstractSocket::UnconnectedState)
            break;
    }
    socket->flush();
}

QVncClientCursor::QVncClientCursor(QVncServer *s)
    : server(s)
{
}

QVncClientCursor::~QVncClientCursor()
{
}

void QVncClientCursor::write() const
{
    QTcpSocket *socket = server->clientSocket();

    // FramebufferUpdate header
    {
        const quint16 tmp[6] = { htons(0),
                                 htons(1),
                                 htons(hotspot.x()), htons(hotspot.y()),
                                 htons(cursor.width()),
                                 htons(cursor.height()) };
        socket->write((char*)tmp, sizeof(tmp));

        const quint32 encoding = htonl(-239);
        socket->write((char*)(&encoding), sizeof(encoding));
    }

    if (cursor.isNull())
        return;

    // write pixels
    Q_ASSERT(cursor.hasAlphaChannel());
    const QImage img = cursor.convertToFormat(server->screen()->format());
    const int n = server->clientBytesPerPixel() * img.width();
    char *buffer = new char[n];
    for (int i = 0; i < img.height(); ++i) {
        server->convertPixels(buffer, (const char*)img.scanLine(i), img.width());
        socket->write(buffer, n);
    }
    delete[] buffer;

    // write mask
    const QImage bitmap = cursor.createAlphaMask().convertToFormat(QImage::Format_Mono);
    Q_ASSERT(bitmap.depth() == 1);
    Q_ASSERT(bitmap.size() == img.size());
    const int width = (bitmap.width() + 7) / 8;
    for (int i = 0; i < bitmap.height(); ++i)
        socket->write((const char*)bitmap.scanLine(i), width);
}

#ifndef QT_NO_CURSOR
void QVncClientCursor::changeCursor(QCursor *widgetCursor, QWindow *window)
{
    Q_UNUSED(window);
    const Qt::CursorShape shape = widgetCursor ? widgetCursor->shape() : Qt::ArrowCursor;

    if (shape == Qt::BitmapCursor) {
        // application supplied cursor
        hotspot = widgetCursor->hotSpot();
        cursor = widgetCursor->pixmap().toImage();
    } else {
        // system cursor
        QPlatformCursorImage platformImage(0, 0, 0, 0, 0, 0);
        platformImage.set(shape);
        cursor = *platformImage.image();
        hotspot = platformImage.hotspot();
    }
    server->setDirtyCursor();
}
#endif


QVncServer::QVncServer(QVncScreen *screen)
    : qvnc_screen(screen)
{
    QTimer::singleShot(0, this, SLOT(init()));
}

QVncServer::QVncServer(QVncScreen *screen, int /*id*/)
    : qvnc_screen(screen)
{
    QTimer::singleShot(0, this, SLOT(init()));
}

void QVncServer::init()
{
    const int port = 5900;
    handleMsg = false;
    client = 0;
    encodingsPending = 0;
    cutTextPending = 0;
    keymod = 0;
    state = Disconnected;
    dirtyCursor = false;

    refreshRate = 25;
    timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(checkUpdate()));

    serverSocket = new QTcpServer(this);
    if (!serverSocket->listen(QHostAddress::Any, port))
        qDebug() << "QVncServer could not connect:" << serverSocket->errorString();
    else
        QT_VNC_DEBUG("QVncServer created on port %d", port);
    QT_VNC_DEBUG() << "running in thread" << thread() << QThread::currentThread();

    serverSocket->waitForNewConnection(-1);
    newConnection();

    connect(serverSocket, SIGNAL(newConnection()), this, SLOT(newConnection()));

#ifndef QT_NO_QWS_CURSOR
    qvnc_cursor = 0;
#endif
    encoder = 0;
}

QVncServer::~QVncServer()
{
    delete encoder;
    encoder = 0;
    delete client;
    client = 0;
#ifndef QT_NO_QWS_CURSOR
    delete qvnc_cursor;
    qvnc_cursor = 0;
#endif
}

void QVncServer::setDirty()
{
    if (state == Connected && !timer->isActive() &&
        ((dirtyMap()->numDirty > 0) || dirtyCursor)) {
        timer->start();
    }
}

void QVncServer::newConnection()
{
    if (client)
        delete client;

    client = serverSocket->nextPendingConnection();
    connect(client,SIGNAL(readyRead()),this,SLOT(readClient()));
    connect(client,SIGNAL(disconnected()),this,SLOT(discardClient()));
    handleMsg = false;
    encodingsPending = 0;
    cutTextPending = 0;
    supportHextile = false;
    wantUpdate = false;

    timer->start(1000 / refreshRate);
    dirtyMap()->reset();

    // send protocol version
    const char *proto = "RFB 003.003\n";
    client->write(proto, 12);
    state = Protocol;

    QT_VNC_DEBUG() << "new Connection" << thread();

    qvnc_screen->setPowerState(QPlatformScreen::PowerStateOn);
}

void QVncServer::readClient()
{
    QT_VNC_DEBUG() << "readClient" << state;
    switch (state) {
        case Protocol:
            if (client->bytesAvailable() >= 12) {
                char proto[13];
                client->read(proto, 12);
                proto[12] = '\0';
                QT_VNC_DEBUG("Client protocol version %s", proto);
                // No authentication
                quint32 auth = htonl(1);
                client->write((char *) &auth, sizeof(auth));
                state = Init;
            }
            break;

        case Init:
            if (client->bytesAvailable() >= 1) {
                quint8 shared;
                client->read((char *) &shared, 1);

                // Server Init msg
                QRfbServerInit sim;
                QRfbPixelFormat &format = sim.format;
                switch (qvnc_screen->depth()) {
                case 32:
                    format.bitsPerPixel = 32;
                    format.depth = 32;
                    format.bigEndian = 0;
                    format.trueColor = true;
                    format.redBits = 8;
                    format.greenBits = 8;
                    format.blueBits = 8;
                    format.redShift = 16;
                    format.greenShift = 8;
                    format.blueShift = 0;
                    break;

                case 24:
                    format.bitsPerPixel = 24;
                    format.depth = 24;
                    format.bigEndian = 0;
                    format.trueColor = true;
                    format.redBits = 8;
                    format.greenBits = 8;
                    format.blueBits = 8;
                    format.redShift = 16;
                    format.greenShift = 8;
                    format.blueShift = 0;
                    break;

                case 18:
                    format.bitsPerPixel = 24;
                    format.depth = 18;
                    format.bigEndian = 0;
                    format.trueColor = true;
                    format.redBits = 6;
                    format.greenBits = 6;
                    format.blueBits = 6;
                    format.redShift = 12;
                    format.greenShift = 6;
                    format.blueShift = 0;
                    break;

                case 16:
                    format.bitsPerPixel = 16;
                    format.depth = 16;
                    format.bigEndian = 0;
                    format.trueColor = true;
                    format.redBits = 5;
                    format.greenBits = 6;
                    format.blueBits = 5;
                    format.redShift = 11;
                    format.greenShift = 5;
                    format.blueShift = 0;
                    break;

                case 15:
                    format.bitsPerPixel = 16;
                    format.depth = 15;
                    format.bigEndian = 0;
                    format.trueColor = true;
                    format.redBits = 5;
                    format.greenBits = 5;
                    format.blueBits = 5;
                    format.redShift = 10;
                    format.greenShift = 5;
                    format.blueShift = 0;
                    break;

                case 12:
                    format.bitsPerPixel = 16;
                    format.depth = 12;
                    format.bigEndian = 0;
                    format.trueColor = true;
                    format.redBits = 4;
                    format.greenBits = 4;
                    format.blueBits = 4;
                    format.redShift = 8;
                    format.greenShift = 4;
                    format.blueShift = 0;
                    break;

                case 8:
                case 4:
                    format.bitsPerPixel = 8;
                    format.depth = 8;
                    format.bigEndian = 0;
                    format.trueColor = false;
                    format.redBits = 0;
                    format.greenBits = 0;
                    format.blueBits = 0;
                    format.redShift = 0;
                    format.greenShift = 0;
                    format.blueShift = 0;
                    break;

                default:
                    qDebug("QVNC cannot drive depth %d", qvnc_screen->depth());
                    discardClient();
                    return;
                }
                sim.width = qvnc_screen->geometry().width();
                sim.height = qvnc_screen->geometry().height();
                sim.setName("Qt for Embedded Linux VNC Server");
                sim.write(client);
                state = Connected;
            }
            break;

        case Connected:
            do {
                if (!handleMsg) {
                    client->read((char *)&msgType, 1);
                    handleMsg = true;
                }
                if (handleMsg) {
                    switch (msgType ) {
                    case SetPixelFormat:
                        setPixelFormat();
                        break;
                    case FixColourMapEntries:
                        qDebug("Not supported: FixColourMapEntries");
                        handleMsg = false;
                        break;
                    case SetEncodings:
                        setEncodings();
                        break;
                    case FramebufferUpdateRequest:
                        frameBufferUpdateRequest();
                        break;
                    case KeyEvent:
                        keyEvent();
                        break;
                    case PointerEvent:
                        pointerEvent();
                        break;
                    case ClientCutText:
                        clientCutText();
                        break;
                    default:
                        qDebug("Unknown message type: %d", (int)msgType);
                        handleMsg = false;
                    }
                }
            } while (!handleMsg && client->bytesAvailable());
            break;
    default:
        break;
    }
}

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
bool QVNCScreen::swapBytes() const
{
    if (depth() != 16)
        return false;

    if (screen())
        return screen()->frameBufferLittleEndian();
    return frameBufferLittleEndian();
}
#endif

void QVncServer::setPixelFormat()
{
    if (client->bytesAvailable() >= 19) {
        char buf[3];
        client->read(buf, 3); // just padding
        pixelFormat.read(client);
        QT_VNC_DEBUG("Want format: %d %d %d %d %d %d %d %d %d %d",
            int(pixelFormat.bitsPerPixel),
            int(pixelFormat.depth),
            int(pixelFormat.bigEndian),
            int(pixelFormat.trueColor),
            int(pixelFormat.redBits),
            int(pixelFormat.greenBits),
            int(pixelFormat.blueBits),
            int(pixelFormat.redShift),
            int(pixelFormat.greenShift),
            int(pixelFormat.blueShift));
        if (!pixelFormat.trueColor) {
            qDebug("Can only handle true color clients");
            discardClient();
        }
        handleMsg = false;
        sameEndian = (QSysInfo::ByteOrder == QSysInfo::BigEndian) == !!pixelFormat.bigEndian;
        needConversion = pixelConversionNeeded();
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
        swapBytes = qvnc_screen->swapBytes();
#endif
    }
}

void QVncServer::setEncodings()
{
    QRfbSetEncodings enc;

    if (!encodingsPending && enc.read(client)) {
        encodingsPending = enc.count;
        if (!encodingsPending)
            handleMsg = false;
    }

    if (encoder) {
        delete encoder;
        encoder = 0;
    }

    enum Encodings {
        Raw = 0,
        CopyRect = 1,
        RRE = 2,
        CoRRE = 4,
        Hextile = 5,
        ZRLE = 16,
        Cursor = -239,
        DesktopSize = -223
    };

    if (encodingsPending && (unsigned)client->bytesAvailable() >=
                                encodingsPending * sizeof(quint32)) {
        for (int i = 0; i < encodingsPending; ++i) {
            qint32 enc;
            client->read((char *)&enc, sizeof(qint32));
            enc = ntohl(enc);
            QT_VNC_DEBUG("QVncServer::setEncodings: %d", enc);
            switch (enc) {
            case Raw:
                if (!encoder) {
                    encoder = new QRfbRawEncoder(this);
                    QT_VNC_DEBUG("QVncServer::setEncodings: using raw");
                }
               break;
            case CopyRect:
                supportCopyRect = true;
                break;
            case RRE:
                supportRRE = true;
                break;
            case CoRRE:
                supportCoRRE = true;
                break;
            case Hextile:
                supportHextile = true;
                if (encoder)
                    break;
#if 0
                switch (qvnc_screen->depth()) {
                case 8:
                    encoder = new QRfbHextileEncoder<quint8>(this);
                    break;
                case 16:
                    encoder = new QRfbHextileEncoder<quint16>(this);
                    break;
                case 32:
                    encoder = new QRfbHextileEncoder<quint32>(this);
                    break;
                default:
                    break;
                }
                QT_VNC_DEBUG("QVncServer::setEncodings: using hextile");
#endif
                break;
            case ZRLE:
                supportZRLE = true;
                break;
            case Cursor:
                supportCursor = true;
                if (!qvnc_screen->screen()) {
                    delete qvnc_cursor;
                    qvnc_cursor = new QVncClientCursor(this);
                }
                break;
            case DesktopSize:
                supportDesktopSize = true;
                break;
            default:
                break;
            }
        }
        handleMsg = false;
        encodingsPending = 0;
    }

    if (!encoder) {
        encoder = new QRfbRawEncoder(this);
        QT_VNC_DEBUG("QVncServer::setEncodings: fallback using raw");
    }
}

void QVncServer::frameBufferUpdateRequest()
{
    QT_VNC_DEBUG() << "FramebufferUpdateRequest";
    QRfbFrameBufferUpdateRequest ev;

    if (ev.read(client)) {
        if (!ev.incremental) {
            QRect r(ev.rect.x, ev.rect.y, ev.rect.w, ev.rect.h);
            r.translate(qvnc_screen->geometry().topLeft());
            qvnc_screen->setDirty(r);
        }
        wantUpdate = true;
        checkUpdate();
        handleMsg = false;
    }
}

void QVncServer::pointerEvent()
{
    QRfbPointerEvent ev;
    if (ev.read(client)) {
        const QPoint pos = qvnc_screen->geometry().topLeft() + QPoint(ev.x, ev.y);
        QWindowSystemInterface::handleMouseEvent(0, pos, pos, ev.buttons, QGuiApplication::keyboardModifiers());
        handleMsg = false;
    }
}

void QVncServer::keyEvent()
{
    QRfbKeyEvent ev;

    if (ev.read(client)) {
        if (ev.keycode == Qt::Key_Shift)
            keymod = ev.down ? keymod | Qt::ShiftModifier :
                               keymod & ~Qt::ShiftModifier;
        else if (ev.keycode == Qt::Key_Control)
            keymod = ev.down ? keymod | Qt::ControlModifier :
                               keymod & ~Qt::ControlModifier;
        else if (ev.keycode == Qt::Key_Alt)
            keymod = ev.down ? keymod | Qt::AltModifier :
                               keymod & ~Qt::AltModifier;
        if (ev.unicode || ev.keycode)
            QWindowSystemInterface::handleKeyEvent(0, ev.down ? QEvent::KeyPress : QEvent::KeyRelease, ev.keycode, keymod, QString(ev.unicode));
        handleMsg = false;
    }
}

void QVncServer::clientCutText()
{
    QRfbClientCutText ev;

    if (cutTextPending == 0 && ev.read(client)) {
        cutTextPending = ev.length;
        if (!cutTextPending)
            handleMsg = false;
    }

    if (cutTextPending && client->bytesAvailable() >= cutTextPending) {
        char *text = new char [cutTextPending+1];
        client->read(text, cutTextPending);
        delete [] text;
        cutTextPending = 0;
        handleMsg = false;
    }
}


bool QVncServer::pixelConversionNeeded() const
{
    if (!sameEndian)
        return true;

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    if (qvnc_screen->swapBytes())
        return true;
#endif

    const int screendepth = qvnc_screen->depth();
    if (screendepth != pixelFormat.bitsPerPixel)
        return true;

    switch (screendepth) {
    case 32:
    case 24:
        return false;
    case 16:
        return (pixelFormat.redBits == 5
                && pixelFormat.greenBits == 6
                && pixelFormat.blueBits == 5);
    }
    return true;
}

// count: number of pixels
void QVncServer::convertPixels(char *dst, const char *src, int count) const
{
    const int screendepth = qvnc_screen->depth();

    // cutoffs
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    if (!swapBytes)
#endif
    if (sameEndian) {
        if (screendepth == pixelFormat.bitsPerPixel) { // memcpy cutoffs

            switch (screendepth) {
            case 32:
                memcpy(dst, src, count * sizeof(quint32));
                return;
            case 16:
                if (pixelFormat.redBits == 5
                    && pixelFormat.greenBits == 6
                    && pixelFormat.blueBits == 5)
                {
                    memcpy(dst, src, count * sizeof(quint16));
                    return;
                }
            }
        } else if (screendepth == 16 && pixelFormat.bitsPerPixel == 32) {
#if defined(__i386__) // Currently fails on ARM if dst is not 4 byte aligned
            const quint32 *src32 = reinterpret_cast<const quint32*>(src);
            quint32 *dst32 = reinterpret_cast<quint32*>(dst);
            int count32 = count * sizeof(quint16) / sizeof(quint32);
            while (count32--) {
                const quint32 s = *src32++;
                quint32 result1;
                quint32 result2;

                // red
                result1 = ((s & 0xf8000000) | ((s & 0xe0000000) >> 5)) >> 8;
                result2 = ((s & 0x0000f800) | ((s & 0x0000e000) >> 5)) << 8;

                // green
                result1 |= ((s & 0x07e00000) | ((s & 0x06000000) >> 6)) >> 11;
                result2 |= ((s & 0x000007e0) | ((s & 0x00000600) >> 6)) << 5;

                // blue
                result1 |= ((s & 0x001f0000) | ((s & 0x001c0000) >> 5)) >> 13;
                result2 |= ((s & 0x0000001f) | ((s & 0x0000001c) >> 5)) << 3;

                *dst32++ = result2;
                *dst32++ = result1;
            }
            if (count & 0x1) {
                const quint16 *src16 = reinterpret_cast<const quint16*>(src);
                *dst32 = qt_conv16ToRgb(src16[count - 1]);
            }
            return;
#endif
        }
    }

    const int bytesPerPixel = (pixelFormat.bitsPerPixel + 7) / 8;

    for (int i = 0; i < count; ++i) {
        int r, g, b;

        switch (screendepth) {
        case 8: {
            QRgb rgb = qvnc_screen->image()->colorTable()[int(*src)];
            r = qRed(rgb);
            g = qGreen(rgb);
            b = qBlue(rgb);
            src++;
            break;
        }
        case 16: {
            quint16 p = *reinterpret_cast<const quint16*>(src);
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
            if (swapBytes)
                p = ((p & 0xff) << 8) | ((p & 0xff00) >> 8);
#endif
            r = (p >> 11) & 0x1f;
            g = (p >> 5) & 0x3f;
            b = p & 0x1f;
            r <<= 3;
            g <<= 2;
            b <<= 3;
            src += sizeof(quint16);
            break;
        }
        case 32: {
            quint32 p = *reinterpret_cast<const quint32*>(src);
            r = (p >> 16) & 0xff;
            g = (p >> 8) & 0xff;
            b = p & 0xff;
            src += sizeof(quint32);
            break;
        }
        default: {
            r = g = b = 0;
            qDebug("QVNCServer: don't support %dbpp display", screendepth);
            return;
        }
        }

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
        if (swapBytes)
            qSwap(r, b);
#endif

        r >>= (8 - pixelFormat.redBits);
        g >>= (8 - pixelFormat.greenBits);
        b >>= (8 - pixelFormat.blueBits);

        int pixel = (r << pixelFormat.redShift) |
                    (g << pixelFormat.greenShift) |
                    (b << pixelFormat.blueShift);

        if (sameEndian || pixelFormat.bitsPerPixel == 8) {
            memcpy(dst, &pixel, bytesPerPixel);
            dst += bytesPerPixel;
            continue;
        }


        if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
            switch (pixelFormat.bitsPerPixel) {
            case 16:
                pixel = (((pixel & 0x0000ff00) << 8)  |
                         ((pixel & 0x000000ff) << 24));
                break;
            case 32:
                pixel = (((pixel & 0xff000000) >> 24) |
                         ((pixel & 0x00ff0000) >> 8)  |
                         ((pixel & 0x0000ff00) << 8)  |
                         ((pixel & 0x000000ff) << 24));
                break;
            default:
                qDebug("Cannot handle %d bpp client", pixelFormat.bitsPerPixel);
            }
        } else { // QSysInfo::ByteOrder == QSysInfo::LittleEndian
            switch (pixelFormat.bitsPerPixel) {
            case 16:
                pixel = (((pixel & 0xff000000) >> 8) |
                         ((pixel & 0x00ff0000) << 8));
                break;
            case 32:
                pixel = (((pixel & 0xff000000) >> 24) |
                         ((pixel & 0x00ff0000) >> 8)  |
                         ((pixel & 0x0000ff00) << 8)  |
                         ((pixel & 0x000000ff) << 24));
                break;
            default:
                qDebug("Cannot handle %d bpp client",
                       pixelFormat.bitsPerPixel);
                break;
            }
        }
        memcpy(dst, &pixel, bytesPerPixel);
        dst += bytesPerPixel;
    }
}

void QVncServer::checkUpdate()
{
    if (!wantUpdate)
        return;

    if (dirtyCursor) {
#ifndef QT_NO_QWS_CURSOR
        Q_ASSERT(qvnc_cursor);
        qvnc_cursor->write();
#endif
        dirtyCursor = false;
        wantUpdate = false;
        return;
    }

    if (!qvnc_screen->dirtyRegion.isEmpty()) {
        if (encoder)
            encoder->write();
        wantUpdate = false;
    }
    // ### re-enable map support
//    if (dirtyMap()->numDirty > 0) {
//        if (encoder)
//            encoder->write();
//        wantUpdate = false;
//    }
}

void QVncServer::discardClient()
{
    timer->stop();
    state = Disconnected;
    delete encoder;
    encoder = 0;
#ifndef QT_NO_QWS_CURSOR
    delete qvnc_cursor;
    qvnc_cursor = 0;
#endif

    qvnc_screen->setPowerState(QPlatformScreen::PowerStateOff);
}

inline QImage QVncServer::screenImage() const
{
    return *qvnc_screen->image();
}
