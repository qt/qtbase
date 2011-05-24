/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qvncserver.h"

#include <QtCore/qtimer.h>
#include <QtCore/qregexp.h>
#include <QtGui/qwidget.h>
#include <QtGui/qpolygon.h>
#include <QtGui/qpainter.h>

#include <QtGui/qevent.h>
#include <QWindowSystemInterface>

#include <qplatformdefs.h>

#include <qdebug.h>

#include <stdlib.h>


#define QT_QWS_VNC_DEBUG
#define QT_NO_QWS_CURSOR //###


QT_BEGIN_NAMESPACE



//copied from qscreen_qws.h
#ifndef QT_QWS_DEPTH16_RGB
#define QT_QWS_DEPTH16_RGB 565
#endif
static const int qt_rbits = (QT_QWS_DEPTH16_RGB/100);
static const int qt_gbits = (QT_QWS_DEPTH16_RGB/10%10);
static const int qt_bbits = (QT_QWS_DEPTH16_RGB%10);
static const int qt_red_shift = qt_bbits+qt_gbits-(8-qt_rbits);
static const int qt_green_shift = qt_bbits-(8-qt_gbits);
static const int qt_neg_blue_shift = 8-qt_bbits;
static const int qt_blue_mask = (1<<qt_bbits)-1;
static const int qt_green_mask = (1<<(qt_gbits+qt_bbits))-(1<<qt_bbits);
static const int qt_red_mask = (1<<(qt_rbits+qt_gbits+qt_bbits))-(1<<(qt_gbits+qt_bbits));

static const int qt_red_rounding_shift = qt_red_shift + qt_rbits;
static const int qt_green_rounding_shift = qt_green_shift + qt_gbits;
static const int qt_blue_rounding_shift = qt_bbits - qt_neg_blue_shift;


inline QRgb qt_conv16ToRgb(ushort c)
{
    const int r=(c & qt_red_mask);
    const int g=(c & qt_green_mask);
    const int b=(c & qt_blue_mask);
    const int tr = r >> qt_red_shift | r >> qt_red_rounding_shift;
    const int tg = g >> qt_green_shift | g >> qt_green_rounding_shift;
    const int tb = b << qt_neg_blue_shift | b >> qt_blue_rounding_shift;

    return qRgb(tr,tg,tb);
}



//===========================================================================

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
    wheelDirection = WheelNone;
    if (buttonMask & 1)
        buttons |= Qt::LeftButton;
    if (buttonMask & 2)
        buttons |= Qt::MidButton;
    if (buttonMask & 4)
        buttons |= Qt::RightButton;
    if (buttonMask & 8)
        wheelDirection = WheelUp;
    if (buttonMask & 16)
        wheelDirection = WheelDown;
    if (buttonMask & 32)
        wheelDirection = WheelLeft;
    if (buttonMask & 64)
        wheelDirection = WheelRight;

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

//===========================================================================

QVNCServer::QVNCServer(QVNCScreen *screen)
    : qvnc_screen(screen), cursor(0)
{
    init(5900);
}

QVNCServer::QVNCServer(QVNCScreen *screen, int id)
    : qvnc_screen(screen), cursor(0)
{
    init(5900 + id);
}

void QVNCServer::init(uint port)
{
    qDebug() << "QVNCServer::init" << port;

    handleMsg = false;
    client = 0;
    encodingsPending = 0;
    cutTextPending = 0;
    keymod = 0;
    state = Unconnected;
    dirtyCursor = false;

    refreshRate = 25;
    timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(checkUpdate()));

    serverSocket = new QTcpServer(this);
    if (!serverSocket->listen(QHostAddress::Any, port))
        qDebug() << "QVNCServer could not connect:" << serverSocket->errorString();
    else
        qDebug("QVNCServer created on port %d", port);

    connect(serverSocket, SIGNAL(newConnection()), this, SLOT(newConnection()));

#ifndef QT_NO_QWS_CURSOR
    qvnc_cursor = 0;
#endif
    encoder = 0;
}

QVNCServer::~QVNCServer()
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

void QVNCServer::setDirty()
{
    if (state == Connected && !timer->isActive() &&
        ((dirtyMap()->numDirty > 0) || dirtyCursor)) {
        timer->start();
    }
}

void QVNCServer::newConnection()
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

//     if (!qvnc_screen->screen())
//         QWSServer::instance()->enablePainting(true);
}

void QVNCServer::readClient()
{
    switch (state) {
        case Protocol:
            if (client->bytesAvailable() >= 12) {
                char proto[13];
                client->read(proto, 12);
                proto[12] = '\0';
                qDebug("Client protocol version %s", proto);
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

#if 0//Q_BYTE_ORDER == Q_BIG_ENDIAN
bool QVNCScreen::swapBytes() const
{
    if (depth() != 16)
        return false;

    if (screen())
        return screen()->frameBufferLittleEndian();
    return frameBufferLittleEndian();
}
#endif

void QVNCServer::setPixelFormat()
{
    if (client->bytesAvailable() >= 19) {
        char buf[3];
        client->read(buf, 3); // just padding
        pixelFormat.read(client);
#ifdef QT_QWS_VNC_DEBUG
        qDebug("Want format: %d %d %d %d %d %d %d %d %d %d",
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
#endif
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

void QVNCServer::setEncodings()
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

    supportCursor = false;

    if (encodingsPending && (unsigned)client->bytesAvailable() >=
                                encodingsPending * sizeof(quint32)) {
        for (int i = 0; i < encodingsPending; ++i) {
            qint32 enc;
            client->read((char *)&enc, sizeof(qint32));
            enc = ntohl(enc);
#ifdef QT_QWS_VNC_DEBUG
            qDebug("QVNCServer::setEncodings: %d", enc);
#endif
            switch (enc) {
            case Raw:
                if (!encoder) {
                    encoder = new QRfbRawEncoder(this);
#ifdef QT_QWS_VNC_DEBUG
                    qDebug("QVNCServer::setEncodings: using raw");
#endif
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
                switch (qvnc_screen->depth()) {
#ifdef QT_QWS_DEPTH_8
                case 8:
                    encoder = new QRfbHextileEncoder<quint8>(this);
                    break;
#endif
#ifdef QT_QWS_DEPTH_12
                case 12:
                    encoder = new QRfbHextileEncoder<qrgb444>(this);
                    break;
#endif
#ifdef QT_QWS_DEPTH_15
                case 15:
                    encoder = new QRfbHextileEncoder<qrgb555>(this);
                    break;
#endif
#ifdef QT_QWS_DEPTH_16
                case 16:
                    encoder = new QRfbHextileEncoder<quint16>(this);
                    break;
#endif
#ifdef QT_QWS_DEPTH_18
                case 18:
                    encoder = new QRfbHextileEncoder<qrgb666>(this);
                    break;
#endif
#ifdef QT_QWS_DEPTH_24
                case 24:
                    encoder = new QRfbHextileEncoder<qrgb888>(this);
                    break;
#endif
#ifdef QT_QWS_DEPTH_32
                case 32:
                    encoder = new QRfbHextileEncoder<quint32>(this);
                    break;
#endif
                default:
                    break;
                }
#ifdef QT_QWS_VNC_DEBUG
                qDebug("QVNCServer::setEncodings: using hextile");
#endif
                break;
            case ZRLE:
                supportZRLE = true;
                break;
            case Cursor:
                supportCursor = true;
#ifndef QT_NO_QWS_CURSOR
                if (!qvnc_screen->screen() || qt_screencursor->isAccelerated()) {
                    delete qvnc_cursor;
                    qvnc_cursor = new QVNCClientCursor(this);
                }
#endif
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
#ifdef QT_QWS_VNC_DEBUG
        qDebug("QVNCServer::setEncodings: fallback using raw");
#endif
    }

    if (cursor)
        cursor->setCursorMode(supportCursor);
}

void QVNCServer::frameBufferUpdateRequest()
{
    QRfbFrameBufferUpdateRequest ev;

    if (ev.read(client)) {
        if (!ev.incremental) {
            QRect r(ev.rect.x, ev.rect.y, ev.rect.w, ev.rect.h);
////###            r.translate(qvnc_screen->offset());
            qvnc_screen->d_ptr->setDirty(r, true);
        }
        wantUpdate = true;
        checkUpdate();
        handleMsg = false;
    }
}

static bool buttonChange(Qt::MouseButtons before, Qt::MouseButtons after, Qt::MouseButton *button, bool *isPress)
{
    if (before == after)
        return false;
    for (int b = Qt::LeftButton;  b <= Qt::MidButton; b<<=1) {
        if ((before & b) != (after & b)) {
            *button = static_cast<Qt::MouseButton>(b);
            *isPress = (after & b);
            return true;
        }
    }
    return false;
}

void QVNCServer::pointerEvent()
{
    QPoint screenOffset = this->screen()->geometry().topLeft();

    QRfbPointerEvent ev;
    if (ev.read(client)) {
        QPoint eventPoint(ev.x, ev.y);
        eventPoint += screenOffset;         // local to global translation

        if (ev.wheelDirection == ev.WheelNone) {
            QEvent::Type type = QEvent::MouseMove;
            Qt::MouseButton button = Qt::NoButton;
            bool isPress;
            if (buttonChange(buttons, ev.buttons, &button, &isPress))
                type = isPress ? QEvent::MouseButtonPress : QEvent::MouseButtonRelease;
            QWindowSystemInterface::handleMouseEvent(0, eventPoint, eventPoint, ev.buttons);
        } else {
            // No buttons or motion reported at the same time as wheel events
            Qt::Orientation orientation;
            if (ev.wheelDirection == ev.WheelLeft || ev.wheelDirection == ev.WheelRight)
                orientation = Qt::Horizontal;
            else
                orientation = Qt::Vertical;
            int delta = 120 * ((ev.wheelDirection == ev.WheelLeft || ev.wheelDirection == ev.WheelUp) ? 1 : -1);
            QWindowSystemInterface::handleWheelEvent(0, eventPoint, eventPoint, delta, orientation);
        }
        handleMsg = false;
    }
}

void QVNCServer::keyEvent()
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
        if (ev.unicode || ev.keycode) {
//            qDebug() << "keyEvent" << hex << ev.unicode << ev.keycode <<  keymod << ev.down;
            QEvent::Type type = ev.down ? QEvent::KeyPress : QEvent::KeyRelease;
            QString str;
            if (ev.unicode && ev.unicode != 0xffff)
                str = QString(ev.unicode);
            QWindowSystemInterface::handleKeyEvent(0, type, ev.keycode, keymod, str);
        }
        handleMsg = false;
    }
}

void QVNCServer::clientCutText()
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

// stride in bytes
template <class SRC>
bool QRfbSingleColorHextile<SRC>::read(const uchar *data,
                                       int width, int height, int stride)
{
    const int depth = encoder->server->screen()->depth();
    if (width % (depth / 8)) // hw: should rather fallback to simple loop
        return false;

    static int alwaysFalse = qgetenv("QT_VNC_NOCHECKFILL").toInt();
    if (alwaysFalse)
        return false;

    switch (depth) {
    case 4: {
        const quint8 *data8 = reinterpret_cast<const quint8*>(data);
        if ((data8[0] & 0xf) != (data8[0] >> 4))
            return false;
        width /= 2;
    } // fallthrough
    case 8: {
        const quint8 *data8 = reinterpret_cast<const quint8*>(data);
        if (data8[0] != data8[1])
            return false;
        width /= 2;
    } // fallthrough
    case 12:
    case 15:
    case 16: {
        const quint16 *data16 = reinterpret_cast<const quint16*>(data);
        if (data16[0] != data16[1])
            return false;
        width /= 2;
    } // fallthrough
    case 18:
    case 24:
    case 32: {
        const quint32 *data32 = reinterpret_cast<const quint32*>(data);
        const quint32 first = data32[0];
        const int linestep = (stride / sizeof(quint32)) - width;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                if (*(data32++) != first)
                    return false;
            }
            data32 += linestep;
        }
        break;
    }
    default:
        return false;
    }

    SRC color = reinterpret_cast<const SRC*>(data)[0];
    encoder->newBg |= (color != encoder->bg);
    encoder->bg = color;
    return true;
}

template <class SRC>
void QRfbSingleColorHextile<SRC>::write(QTcpSocket *socket) const
{
    if (true || encoder->newBg) {
        const int bpp = encoder->server->clientBytesPerPixel();
        const int padding = 3;
        QVarLengthArray<char> buffer(padding + 1 + bpp);
        buffer[padding] = 2; // BackgroundSpecified
        encoder->server->convertPixels(buffer.data() + padding + 1,
                                       reinterpret_cast<char*>(&encoder->bg),
                                       1);
        socket->write(buffer.data() + padding, bpp + 1);
//        encoder->newBg = false;
    } else {
        char subenc = 0;
        socket->write(&subenc, 1);
    }
}

template <class SRC>
bool QRfbDualColorHextile<SRC>::read(const uchar *data,
                                     int width, int height, int stride)
{
    const SRC *ptr = reinterpret_cast<const SRC*>(data);
    const int linestep = (stride / sizeof(SRC)) - width;

    SRC c1;
    SRC c2 = 0;
    int n1 = 0;
    int n2 = 0;
    int x = 0;
    int y = 0;

    c1 = *ptr;

    // find second color
    while (y < height) {
        while (x < width) {
            if (*ptr == c1) {
                ++n1;
            } else {
                c2 = *ptr;
                goto found_second_color;
            }
            ++ptr;
            ++x;
        }
        x = 0;
        ptr += linestep;
        ++y;
    }

found_second_color:
    // finish counting
    while (y < height) {
        while (x < width) {
            if (*ptr == c1) {
                ++n1;
            } else if (*ptr == c2) {
                ++n2;
            } else {
                return false;
            }
            ++ptr;
            ++x;
        }
        x = 0;
        ptr += linestep;
        ++y;
    }

    if (n2 > n1) {
        const quint32 tmpC = c1;
        c1 = c2;
        c2 = tmpC;
    }

    encoder->newBg |= (c1 != encoder->bg);
    encoder->newFg |= (c2 != encoder->fg);

    encoder->bg = c1;
    encoder->fg = c2;

    // create map
    bool inRect = false;
    numRects = 0;
    ptr = reinterpret_cast<const SRC*>(data);
    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            if (inRect && *ptr == encoder->bg) {
                // rect finished
                setWidth(x - lastx());
                next();
                inRect = false;
            } else if (!inRect && *ptr == encoder->fg) {
                // rect start
                setX(x);
                setY(y);
                setHeight(1);
                inRect = true;
            }
            ++ptr;
        }
        if (inRect) {
            // finish rect
            setWidth(width - lastx());
            next();
            inRect = false;
        }
        ptr += linestep;
    }

    return true;
}

template <class SRC>
void QRfbDualColorHextile<SRC>::write(QTcpSocket *socket) const
{
    const int bpp = encoder->server->clientBytesPerPixel();
    const int padding = 3;
    QVarLengthArray<char> buffer(padding + 2 * bpp + sizeof(char) + sizeof(numRects));
    char &subenc = buffer[padding];
    int n = padding + sizeof(subenc);

    subenc = 0x8; // AnySubrects

    if (encoder->newBg) {
        subenc |= 0x2; // Background
        encoder->server->convertPixels(buffer.data() + n, (char*)&encoder->bg, 1);
        n += bpp;
//        encoder->newBg = false;
    }

    if (encoder->newFg) {
        subenc |= 0x4; // Foreground
        encoder->server->convertPixels(buffer.data() + n, (char*)&encoder->fg, 1);
        n += bpp;
//        encoder->newFg = false;
    }
    buffer[n] = numRects;
    n += sizeof(numRects);

    socket->write(buffer.data() + padding, n - padding);
    socket->write((char*)rects, numRects * sizeof(Rect));
}

template <class SRC>
void QRfbDualColorHextile<SRC>::next()
{
    for (int r = numRects - 1; r >= 0; --r) {
        if (recty(r) == lasty())
            continue;
        if (recty(r) < lasty() - 1) // only search previous scanline
            break;
        if (rectx(r) == lastx() && width(r) == width(numRects)) {
            ++rects[r].wh;
            return;
        }
    }
    ++numRects;
}

template <class SRC>
inline void QRfbMultiColorHextile<SRC>::setColor(SRC color)
{
    encoder->server->convertPixels(reinterpret_cast<char*>(rect(numRects)),
                                   (const char*)&color, 1);
}

template <class SRC>
inline bool QRfbMultiColorHextile<SRC>::beginRect()
{
    if ((rects.size() + bpp + 2) > maxRectsSize)
        return false;
    rects.resize(rects.size() + bpp + 2);
    return true;
}

template <class SRC>
inline void QRfbMultiColorHextile<SRC>::endRect()
{
    setHeight(numRects, 1);
    ++numRects;
}

template <class SRC>
bool QRfbMultiColorHextile<SRC>::read(const uchar *data,
                                      int width, int height, int stride)
{
    const SRC *ptr = reinterpret_cast<const SRC*>(data);
    const int linestep = (stride / sizeof(SRC)) - width;

    bpp = encoder->server->clientBytesPerPixel();

    if (encoder->newBg)
        encoder->bg = ptr[0];

    const SRC bg = encoder->bg;
    SRC color = bg;
    bool inRect = false;

    numRects = 0;
    rects.clear();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (inRect && *ptr != color) { // end rect
                setWidth(numRects, x - rectx(numRects));
                endRect();
                inRect = false;
            }

            if (!inRect && *ptr != bg) { // begin rect
                if (!beginRect())
                    return false;
                inRect = true;
                color = *ptr;
                setColor(color);
                setX(numRects, x);
                setY(numRects, y);
            }
            ++ptr;
        }
        if (inRect) { // end rect
            setWidth(numRects, width - rectx(numRects));
            endRect();
            inRect = false;
        }
        ptr += linestep;
    }

    return true;
}

template <class SRC>
void QRfbMultiColorHextile<SRC>::write(QTcpSocket *socket) const
{
    const int padding = 3;
    QVarLengthArray<quint8> buffer(bpp + padding + sizeof(quint8) + sizeof(numRects));

    quint8 &subenc = buffer[padding];
    int n = padding + sizeof(quint8);

    subenc = 8 | 16; // AnySubrects | SubrectsColoured

    if (encoder->newBg) {
        subenc |= 0x2; // Background
        encoder->server->convertPixels(reinterpret_cast<char*>(buffer.data() + n),
                                       reinterpret_cast<const char*>(&encoder->bg),
                                       1);
        n += bpp;
//        encoder->newBg = false;
    }

    buffer[n] = numRects;
    n += sizeof(numRects);

    socket->write(reinterpret_cast<const char*>(buffer.data() + padding),
                  n - padding);
    socket->write(reinterpret_cast<const char*>(rects.constData()),
                  rects.size());
}

bool QVNCServer::pixelConversionNeeded() const
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
    case 18:
        return (pixelFormat.redBits == 6
                && pixelFormat.greenBits == 6
                && pixelFormat.blueBits == 6);
    case 16:
        return (pixelFormat.redBits == 5
                && pixelFormat.greenBits == 6
                && pixelFormat.blueBits == 5);
    case 15:
        return (pixelFormat.redBits == 5
                && pixelFormat.greenBits == 5
                && pixelFormat.blueBits == 5);
    case 12:
        return (pixelFormat.redBits == 4
                && pixelFormat.greenBits == 4
                && pixelFormat.blueBits == 4);
    }
    return true;
}

// count: number of pixels
void QVNCServer::convertPixels(char *dst, const char *src, int count) const
{
    const int screendepth = qvnc_screen->depth();
    const bool isBgr = false; //### qvnc_screen->pixelType() == QScreen::BGRPixel;

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

//    nibble = 0;

    for (int i = 0; i < count; ++i) {
        int r, g, b;

        switch (screendepth) {
#if 0
        case 4: {
            if (!nibble) {
                r = ((*src) & 0x0f) << 4;
            } else {
                r = (*src) & 0xf0;
                src++;
            }
            nibble = !nibble;
            g = b = r;
            break;
        }
#endif
#if 0
        case 8: {
            QRgb rgb = qvnc_screen->clut()[int(*src)];
            r = qRed(rgb);
            g = qGreen(rgb);
            b = qBlue(rgb);
            src++;
            break;
        }
#endif
#ifdef QT_QWS_DEPTH_12
        case 12: {
            quint32 p = quint32(*reinterpret_cast<const qrgb444*>(src));
            r = qRed(p);
            g = qGreen(p);
            b = qBlue(p);
            src += sizeof(qrgb444);
            break;
        }
#endif
#ifdef QT_QWS_DEPTH_15
        case 15: {
            quint32 p = quint32(*reinterpret_cast<const qrgb555*>(src));
            r = qRed(p);
            g = qGreen(p);
            b = qBlue(p);
            src += sizeof(qrgb555);
            break;
        }
#endif
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
#ifdef QT_QWS_DEPTH_18
        case 18: {
            quint32 p = quint32(*reinterpret_cast<const qrgb666*>(src));
            r = qRed(p);
            g = qGreen(p);
            b = qBlue(p);
            src += sizeof(qrgb666);
            break;
        }
#endif
#ifdef QT_QWS_DEPTH_24
        case 24: {
            quint32 p = quint32(*reinterpret_cast<const qrgb888*>(src));
            r = qRed(p);
            g = qGreen(p);
            b = qBlue(p);
            src += sizeof(qrgb888);
            break;
        }
#endif
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
        if (swapBytes ^ isBgr)
#else
        if (isBgr)
#endif
            qSwap(r, b);

        r >>= (8 - pixelFormat.redBits);
        g >>= (8 - pixelFormat.greenBits);
        b >>= (8 - pixelFormat.blueBits);

        int pixel = (r << pixelFormat.redShift) |
                    (g << pixelFormat.greenShift) |
                    (b << pixelFormat.blueShift);

        if (sameEndian || pixelFormat.bitsPerPixel == 8) {
            memcpy(dst, &pixel, bytesPerPixel); // XXX: do a simple for-loop instead?
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
        memcpy(dst, &pixel, bytesPerPixel); // XXX: simple for-loop instead?
        dst += bytesPerPixel;
    }
}

#ifndef QT_NO_QWS_CURSOR
static void blendCursor(QImage &image, const QRect &imageRect)
{
    const QRect cursorRect = qt_screencursor->boundingRect();
    const QRect intersection = (cursorRect & imageRect);
    const QRect destRect = intersection.translated(-imageRect.topLeft());
    const QRect srcRect = intersection.translated(-cursorRect.topLeft());

    QPainter painter(&image);
    painter.drawImage(destRect, qt_screencursor->image(), srcRect);
    painter.end();
}
#endif // QT_NO_QWS_CURSOR

QVNCDirtyMap::QVNCDirtyMap(QVNCScreen *s)
    : bytesPerPixel(0), numDirty(0), screen(s)
{
    bytesPerPixel = (screen->depth() + 7) / 8;
    QSize screenSize = screen->geometry().size();
    bufferWidth = screenSize.width();
    bufferHeight = screenSize.height();
    bufferStride = bufferWidth * bytesPerPixel;
    buffer = new uchar[bufferHeight * bufferStride];

    mapWidth = (bufferWidth + MAP_TILE_SIZE - 1) / MAP_TILE_SIZE;
    mapHeight = (bufferHeight + MAP_TILE_SIZE - 1) / MAP_TILE_SIZE;
    numTiles = mapWidth * mapHeight;
    map = new uchar[numTiles];
}

QVNCDirtyMap::~QVNCDirtyMap()
{
    delete[] map;
    delete[] buffer;
}

void QVNCDirtyMap::reset()
{
    memset(map, 1, numTiles);
    memset(buffer, 0, bufferHeight * bufferStride);
    numDirty = numTiles;
}

inline bool QVNCDirtyMap::dirty(int x, int y) const
{
    return map[y * mapWidth + x];
}

inline void QVNCDirtyMap::setClean(int x, int y)
{
    map[y * mapWidth + x] = 0;
    --numDirty;
}

template <class T>
void QVNCDirtyMapOptimized<T>::setDirty(int tileX, int tileY, bool force)
{
    static bool alwaysForce = qgetenv("QT_VNC_NO_COMPAREBUFFER").toInt();
    if (alwaysForce)
        force = true;

    bool changed = false;

    if (!force) {
        const int lstep = screen->linestep();
        const int startX = tileX * MAP_TILE_SIZE;
        const int startY = tileY * MAP_TILE_SIZE;
        const uchar *scrn = screen->base()
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

template <class SRC>
QRfbHextileEncoder<SRC>::QRfbHextileEncoder(QVNCServer *s)
    : QRfbEncoder(s),
      singleColorHextile(this), dualColorHextile(this), multiColorHextile(this)
{
}

/*
    \internal
    Send dirty rects using hextile encoding.
*/
template <class SRC>
void QRfbHextileEncoder<SRC>::write()
{
//    QWSDisplay::grab(true);

    QVNCDirtyMap *map = server->dirtyMap();
    QTcpSocket *socket = server->clientSocket();

    const quint32 encoding = htonl(5); // hextile encoding
    const int bytesPerPixel = server->clientBytesPerPixel();

    {
        const char tmp[2] = { 0, 0 }; // msg type, padding
        socket->write(tmp, sizeof(tmp));
    }
    {
        const quint16 count = htons(map->numDirty);
        socket->write((char *)&count, sizeof(count));
    }

    if (map->numDirty <= 0) {
//        QWSDisplay::ungrab();
        return;
    }

    newBg = true;
    newFg = true;

    const QImage screenImage = server->screenImage();
    QRfbRect rect(0, 0, MAP_TILE_SIZE, MAP_TILE_SIZE);

    QSize screenSize = server->screen()->geometry().size();

    for (int y = 0; y < map->mapHeight; ++y) {
        if (rect.y + MAP_TILE_SIZE > screenSize.height())
            rect.h = screenSize.height() - rect.y;
        rect.w = MAP_TILE_SIZE;
        for (int x = 0; x < map->mapWidth; ++x) {
            if (!map->dirty(x, y))
                continue;
            map->setClean(x, y);

            rect.x = x * MAP_TILE_SIZE;
            if (rect.x + MAP_TILE_SIZE > screenSize.width()) //###deviceWidth ???
                rect.w = screenSize.width() - rect.x;
            rect.write(socket);

            socket->write((char *)&encoding, sizeof(encoding));

            const uchar *screendata = screenImage.scanLine(rect.y)
                                      + rect.x * screenImage.depth() / 8;
            int linestep = screenImage.bytesPerLine();

#ifndef QT_NO_QWS_CURSOR
            // hardware cursors must be blended with the screen memory
            const bool doBlendCursor = qt_screencursor
                                       && !server->hasClientCursor()
                                       && qt_screencursor->isAccelerated();
            QImage tileImage;
            if (doBlendCursor) {
                const QRect tileRect(rect.x, rect.y, rect.w, rect.h);
                const QRect cursorRect = qt_screencursor->boundingRect()
                                         .translated(-server->screen()->offset());
                if (tileRect.intersects(cursorRect)) {
                    tileImage = screenImage.copy(tileRect);
                    blendCursor(tileImage,
                                tileRect.translated(server->screen()->offset()));
                    screendata = tileImage.bits();
                    linestep = tileImage.bytesPerLine();
                }
            }
#endif // QT_NO_QWS_CURSOR

            if (singleColorHextile.read(screendata, rect.w, rect.h, linestep)) {
                singleColorHextile.write(socket);
            } else if (dualColorHextile.read(screendata, rect.w, rect.h, linestep)) {
                dualColorHextile.write(socket);
            } else if (multiColorHextile.read(screendata, rect.w, rect.h, linestep)) {
                multiColorHextile.write(socket);
            } else if (server->doPixelConversion()) {
                const int bufferSize = rect.w * rect.h * bytesPerPixel + 1;
                const int padding = sizeof(quint32) - sizeof(char);
                buffer.resize(bufferSize + padding);

                buffer[padding] = 1; // Raw subencoding

                // convert pixels
                char *b = buffer.data() + padding + 1;
                const int bstep = rect.w * bytesPerPixel;
                for (int i = 0; i < rect.h; ++i) {
                    server->convertPixels(b, (const char*)screendata, rect.w);
                    screendata += linestep;
                    b += bstep;
                }
                socket->write(buffer.constData() + padding, bufferSize);
            } else {
                quint8 subenc = 1; // Raw subencoding
                socket->write((char *)&subenc, 1);

                // send pixels
                for (int i = 0; i < rect.h; ++i) {
                    socket->write((const char*)screendata,
                                  rect.w * bytesPerPixel);
                    screendata += linestep;
                }
            }
        }
        if (socket->state() == QAbstractSocket::UnconnectedState)
            break;
        rect.y += MAP_TILE_SIZE;
    }
    socket->flush();
    Q_ASSERT(map->numDirty == 0);

//    QWSDisplay::ungrab();
}

void QRfbRawEncoder::write()
{
//    QWSDisplay::grab(false);

    QVNCDirtyMap *map = server->dirtyMap();
    QTcpSocket *socket = server->clientSocket();

    const int bytesPerPixel = server->clientBytesPerPixel();
    QSize screenSize = server->screen()->geometry().size();

    // create a region from the dirty rects and send the region's merged rects.
    QRegion rgn;
    if (map) {
        for (int y = 0; y < map->mapHeight; ++y) {
            for (int x = 0; x < map->mapWidth; ++x) {
                if (!map->dirty(x, y))
                    continue;
                rgn += QRect(x * MAP_TILE_SIZE, y * MAP_TILE_SIZE,
                             MAP_TILE_SIZE, MAP_TILE_SIZE);
                map->setClean(x, y);
            }
        }

        rgn &= QRect(0, 0, screenSize.width(),
                     screenSize.height());
    }
    const QVector<QRect> rects = rgn.rects();

    {
        const char tmp[2] = { 0, 0 }; // msg type, padding
        socket->write(tmp, sizeof(tmp));
    }

    {
        const quint16 count = htons(rects.size());
        socket->write((char *)&count, sizeof(count));
    }

    if (rects.size() <= 0) {
//        QWSDisplay::ungrab();
        return;
    }

    const QImage *screenImage = server->screenImage();

    for (int i = 0; i < rects.size(); ++i) {
        const QRect tileRect = rects.at(i);
        const QRfbRect rect(tileRect.x(), tileRect.y(),
                            tileRect.width(), tileRect.height());
        rect.write(socket);

        const quint32 encoding = htonl(0); // raw encoding
        socket->write((char *)&encoding, sizeof(encoding));

        int linestep = screenImage->bytesPerLine();
        const uchar *screendata = screenImage->scanLine(rect.y)
                                  + rect.x * screenImage->depth() / 8;

#ifndef QT_NO_QWS_CURSOR
        // hardware cursors must be blended with the screen memory
        const bool doBlendCursor = qt_screencursor
                                   && !server->hasClientCursor()
                                   && qt_screencursor->isAccelerated();
        QImage tileImage;
        if (doBlendCursor) {
            const QRect cursorRect = qt_screencursor->boundingRect()
                                     .translated(-server->screen()->offset());
            if (tileRect.intersects(cursorRect)) {
                tileImage = screenImage->copy(tileRect);
                blendCursor(tileImage,
                            tileRect.translated(server->screen()->offset()));
                screendata = tileImage.bits();
                linestep = tileImage.bytesPerLine();
            }
        }
#endif // QT_NO_QWS_CURSOR

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

//    QWSDisplay::ungrab();
}

inline QImage *QVNCServer::screenImage() const
{
    return qvnc_screen->image();
}

void QVNCServer::checkUpdate()
{
    if (!wantUpdate)
        return;

    if (dirtyCursor) {
#ifndef QT_NO_QWS_CURSOR
        Q_ASSERT(qvnc_cursor);
        qvnc_cursor->write();
#endif
        cursor->sendClientCursor();
        dirtyCursor = false;
        wantUpdate = false;
        return;
    }

    if (dirtyMap()->numDirty > 0) {
        if (encoder)
            encoder->write();
        wantUpdate = false;
    }
}

void QVNCServer::discardClient()
{
    timer->stop();
    state = Unconnected;
    delete encoder;
    encoder = 0;
#ifndef QT_NO_QWS_CURSOR
    delete qvnc_cursor;
    qvnc_cursor = 0;
#endif
//     if (!qvnc_screen->screen())
//         QWSServer::instance()->enablePainting(false);
}



QVNCScreenPrivate::QVNCScreenPrivate(QVNCScreen *parent, int screenId)
    : dpiX(72), dpiY(72), doOnScreenSurface(false), refreshRate(25),
      vncServer(0), q_ptr(parent)
{
#if 0//ndef QT_NO_QWS_SIGNALHANDLER
    QWSSignalHandler::instance()->addObject(this);
#endif

    vncServer = new QVNCServer(q_ptr, screenId);
    vncServer->setRefreshRate(refreshRate);


    Q_ASSERT(q_ptr->depth() == 32);

    dirty = new QVNCDirtyMapOptimized<quint32>(q_ptr);
}

QVNCScreenPrivate::~QVNCScreenPrivate()
{
}


void QVNCScreenPrivate::setDirty(const QRect& rect, bool force)
{
    if (rect.isEmpty())
        return;

//     if (q_ptr->screen())
//         q_ptr->screen()->setDirty(rect);

    if (!vncServer || !vncServer->isConnected()) {
//        qDebug() << "QVNCScreenPrivate::setDirty() - Not connected";
        return;
    }
    const QRect r = rect; // .translated(-q_ptr->offset());
    const int x1 = r.x() / MAP_TILE_SIZE;
    int y = r.y() / MAP_TILE_SIZE;
    for (; (y <= r.bottom() / MAP_TILE_SIZE) && y < dirty->mapHeight; y++)
        for (int x = x1; (x <= r.right() / MAP_TILE_SIZE) && x < dirty->mapWidth; x++)
            dirty->setDirty(x, y, force);

    vncServer->setDirty();
}




QT_END_NAMESPACE
