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

#include "qvncclient.h"
#include "qvnc_p.h"

#include <QtNetwork/QTcpSocket>
#include <QtCore/QCoreApplication>

#include <qpa/qwindowsysteminterface.h>
#include <QtGui/qguiapplication.h>

#ifdef Q_OS_WIN
#include <Winsock2.h>
#else
#include <arpa/inet.h>
#endif

QT_BEGIN_NAMESPACE

QVncClient::QVncClient(QTcpSocket *clientSocket, QVncServer *server)
    : QObject(server)
    , m_server(server)
    , m_clientSocket(clientSocket)
    , m_encoder(nullptr)
    , m_msgType(0)
    , m_handleMsg(false)
    , m_encodingsPending(0)
    , m_cutTextPending(0)
    , m_supportHextile(false)
    , m_wantUpdate(false)
    , m_keymod(0)
    , m_dirtyCursor(false)
    , m_updatePending(false)
    , m_protocolVersion(V3_3)
{
    connect(m_clientSocket,SIGNAL(readyRead()),this,SLOT(readClient()));
    connect(m_clientSocket,SIGNAL(disconnected()),this,SLOT(discardClient()));

    // send protocol version
    const char *proto = "RFB 003.003\n";
    m_clientSocket->write(proto, 12);
    m_state = Protocol;
}

QVncClient::~QVncClient()
{
    delete m_encoder;
}

QTcpSocket *QVncClient::clientSocket() const
{
    return m_clientSocket;
}

void QVncClient::setDirty(const QRegion &region)
{
    m_dirtyRegion += region;
    if (m_state == Connected &&
        ((m_server->dirtyMap()->numDirty > 0) || m_dirtyCursor)) {
        scheduleUpdate();
    }
}

void QVncClient::convertPixels(char *dst, const char *src, int count) const
{
    const int screendepth = m_server->screen()->depth();

    // cutoffs
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    if (!m_swapBytes)
#endif
    if (m_sameEndian) {
        if (screendepth == m_pixelFormat.bitsPerPixel) { // memcpy cutoffs

            switch (screendepth) {
            case 32:
                memcpy(dst, src, count * sizeof(quint32));
                return;
            case 16:
                if (m_pixelFormat.redBits == 5
                    && m_pixelFormat.greenBits == 6
                    && m_pixelFormat.blueBits == 5)
                {
                    memcpy(dst, src, count * sizeof(quint16));
                    return;
                }
            }
        }
    }

    const int bytesPerPixel = (m_pixelFormat.bitsPerPixel + 7) / 8;

    for (int i = 0; i < count; ++i) {
        int r, g, b;

        switch (screendepth) {
        case 8: {
            QRgb rgb = m_server->screen()->image()->colorTable()[int(*src)];
            r = qRed(rgb);
            g = qGreen(rgb);
            b = qBlue(rgb);
            src++;
            break;
        }
        case 16: {
            quint16 p = *reinterpret_cast<const quint16*>(src);
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
            if (m_swapBytes)
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
            qWarning("QVNCServer: don't support %dbpp display", screendepth);
            return;
        }
        }

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
        if (m_swapBytes)
            qSwap(r, b);
#endif

        r >>= (8 - m_pixelFormat.redBits);
        g >>= (8 - m_pixelFormat.greenBits);
        b >>= (8 - m_pixelFormat.blueBits);

        int pixel = (r << m_pixelFormat.redShift) |
                    (g << m_pixelFormat.greenShift) |
                    (b << m_pixelFormat.blueShift);

        if (m_sameEndian || m_pixelFormat.bitsPerPixel == 8) {
            memcpy(dst, &pixel, bytesPerPixel);
            dst += bytesPerPixel;
            continue;
        }


        if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
            switch (m_pixelFormat.bitsPerPixel) {
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
                qWarning("Cannot handle %d bpp client", m_pixelFormat.bitsPerPixel);
            }
        } else { // QSysInfo::ByteOrder == QSysInfo::LittleEndian
            switch (m_pixelFormat.bitsPerPixel) {
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
                qWarning("Cannot handle %d bpp client",
                       m_pixelFormat.bitsPerPixel);
                break;
            }
        }
        memcpy(dst, &pixel, bytesPerPixel);
        dst += bytesPerPixel;
    }
}

void QVncClient::readClient()
{
    qCDebug(lcVnc) << "readClient" << m_state;
    switch (m_state) {
        case Disconnected:

            break;
        case Protocol:
            if (m_clientSocket->bytesAvailable() >= 12) {
                char proto[13];
                m_clientSocket->read(proto, 12);
                proto[12] = '\0';
                qCDebug(lcVnc, "Client protocol version %s", proto);
                if (!strcmp(proto, "RFB 003.008\n")) {
                    m_protocolVersion = V3_8;
                } else if (!strcmp(proto, "RFB 003.007\n")) {
                    m_protocolVersion = V3_7;
                } else {
                    m_protocolVersion = V3_3;
                }

                if (m_protocolVersion == V3_3) {
                    // No authentication
                    quint32 auth = htonl(1);
                    m_clientSocket->write((char *) &auth, sizeof(auth));
                    m_state = Init;
                }
            }
            break;
        case Authentication:

            break;
        case Init:
            if (m_clientSocket->bytesAvailable() >= 1) {
                quint8 shared;
                m_clientSocket->read((char *) &shared, 1);

                // Server Init msg
                QRfbServerInit sim;
                QRfbPixelFormat &format = sim.format;
                switch (m_server->screen()->depth()) {
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
                    qWarning("QVNC cannot drive depth %d", m_server->screen()->depth());
                    discardClient();
                    return;
                }
                sim.width = m_server->screen()->geometry().width();
                sim.height = m_server->screen()->geometry().height();
                sim.setName("Qt for Embedded Linux VNC Server");
                sim.write(m_clientSocket);
                m_state = Connected;
            }
            break;

        case Connected:
            do {
                if (!m_handleMsg) {
                    m_clientSocket->read((char *)&m_msgType, 1);
                    m_handleMsg = true;
                }
                if (m_handleMsg) {
                    switch (m_msgType ) {
                    case SetPixelFormat:
                        setPixelFormat();
                        break;
                    case FixColourMapEntries:
                        qWarning("Not supported: FixColourMapEntries");
                        m_handleMsg = false;
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
                        qWarning("Unknown message type: %d", (int)m_msgType);
                        m_handleMsg = false;
                    }
                }
            } while (!m_handleMsg && m_clientSocket->bytesAvailable());
            break;
    default:
        break;
    }
}

void QVncClient::discardClient()
{
    m_state = Disconnected;
    m_server->discardClient(this);
}

void QVncClient::checkUpdate()
{
    if (!m_wantUpdate)
        return;
#if QT_CONFIG(cursor)
    if (m_dirtyCursor) {
        m_server->screen()->clientCursor->write(this);
        m_dirtyCursor = false;
        m_wantUpdate = false;
        return;
    }
#endif
    if (!m_dirtyRegion.isEmpty()) {
        if (m_encoder)
            m_encoder->write();
        m_wantUpdate = false;
        m_dirtyRegion = QRegion();
    }
}

void QVncClient::scheduleUpdate()
{
    if (!m_updatePending) {
        m_updatePending = true;
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }
}

bool QVncClient::event(QEvent *event)
{
    if (event->type() == QEvent::UpdateRequest) {
        m_updatePending = false;
        checkUpdate();
        return true;
    }
    return QObject::event(event);
}

void QVncClient::setPixelFormat()
{
    if (m_clientSocket->bytesAvailable() >= 19) {
        char buf[3];
        m_clientSocket->read(buf, 3); // just padding
        m_pixelFormat.read(m_clientSocket);
        qCDebug(lcVnc, "Want format: %d %d %d %d %d %d %d %d %d %d",
            int(m_pixelFormat.bitsPerPixel),
            int(m_pixelFormat.depth),
            int(m_pixelFormat.bigEndian),
            int(m_pixelFormat.trueColor),
            int(m_pixelFormat.redBits),
            int(m_pixelFormat.greenBits),
            int(m_pixelFormat.blueBits),
            int(m_pixelFormat.redShift),
            int(m_pixelFormat.greenShift),
            int(m_pixelFormat.blueShift));
        if (!m_pixelFormat.trueColor) {
            qWarning("Can only handle true color clients");
            discardClient();
        }
        m_handleMsg = false;
        m_sameEndian = (QSysInfo::ByteOrder == QSysInfo::BigEndian) == !!m_pixelFormat.bigEndian;
        m_needConversion = pixelConversionNeeded();
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
        m_swapBytes = server()->screen()->swapBytes();
#endif
    }
}

void QVncClient::setEncodings()
{
    QRfbSetEncodings enc;

    if (!m_encodingsPending && enc.read(m_clientSocket)) {
        m_encodingsPending = enc.count;
        if (!m_encodingsPending)
            m_handleMsg = false;
    }

    if (m_encoder) {
        delete m_encoder;
        m_encoder = nullptr;
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

    if (m_encodingsPending && (unsigned)m_clientSocket->bytesAvailable() >=
                                m_encodingsPending * sizeof(quint32)) {
        for (int i = 0; i < m_encodingsPending; ++i) {
            qint32 enc;
            m_clientSocket->read((char *)&enc, sizeof(qint32));
            enc = ntohl(enc);
            qCDebug(lcVnc, "QVncServer::setEncodings: %d", enc);
            switch (enc) {
            case Raw:
                if (!m_encoder) {
                    m_encoder = new QRfbRawEncoder(this);
                    qCDebug(lcVnc, "QVncServer::setEncodings: using raw");
                }
               break;
            case CopyRect:
                m_supportCopyRect = true;
                break;
            case RRE:
                m_supportRRE = true;
                break;
            case CoRRE:
                m_supportCoRRE = true;
                break;
            case Hextile:
                m_supportHextile = true;
                if (m_encoder)
                    break;
                break;
            case ZRLE:
                m_supportZRLE = true;
                break;
            case Cursor:
                m_supportCursor = true;
                m_server->screen()->enableClientCursor(this);
                break;
            case DesktopSize:
                m_supportDesktopSize = true;
                break;
            default:
                break;
            }
        }
        m_handleMsg = false;
        m_encodingsPending = 0;
    }

    if (!m_encoder) {
        m_encoder = new QRfbRawEncoder(this);
        qCDebug(lcVnc, "QVncServer::setEncodings: fallback using raw");
    }
}

void QVncClient::frameBufferUpdateRequest()
{
    qCDebug(lcVnc) << "FramebufferUpdateRequest";
    QRfbFrameBufferUpdateRequest ev;

    if (ev.read(m_clientSocket)) {
        if (!ev.incremental) {
            QRect r(ev.rect.x, ev.rect.y, ev.rect.w, ev.rect.h);
            r.translate(m_server->screen()->geometry().topLeft());
            setDirty(r);
        }
        m_wantUpdate = true;
        checkUpdate();
        m_handleMsg = false;
    }
}

void QVncClient::pointerEvent()
{
    QRfbPointerEvent ev;
    if (ev.read(m_clientSocket)) {
        const QPoint pos = m_server->screen()->geometry().topLeft() + QPoint(ev.x, ev.y);
        QWindowSystemInterface::handleMouseEvent(0, pos, pos, ev.buttons, QGuiApplication::keyboardModifiers());
        m_handleMsg = false;
    }
}

void QVncClient::keyEvent()
{
    QRfbKeyEvent ev;

    if (ev.read(m_clientSocket)) {
        if (ev.keycode == Qt::Key_Shift)
            m_keymod = ev.down ? m_keymod | Qt::ShiftModifier :
                                 m_keymod & ~Qt::ShiftModifier;
        else if (ev.keycode == Qt::Key_Control)
            m_keymod = ev.down ? m_keymod | Qt::ControlModifier :
                                 m_keymod & ~Qt::ControlModifier;
        else if (ev.keycode == Qt::Key_Alt)
            m_keymod = ev.down ? m_keymod | Qt::AltModifier :
                                 m_keymod & ~Qt::AltModifier;
        if (ev.unicode || ev.keycode)
            QWindowSystemInterface::handleKeyEvent(0, ev.down ? QEvent::KeyPress : QEvent::KeyRelease, ev.keycode, m_keymod, QString(ev.unicode));
        m_handleMsg = false;
    }
}

void QVncClient::clientCutText()
{
    QRfbClientCutText ev;

    if (m_cutTextPending == 0 && ev.read(m_clientSocket)) {
        m_cutTextPending = ev.length;
        if (!m_cutTextPending)
            m_handleMsg = false;
    }

    if (m_cutTextPending && m_clientSocket->bytesAvailable() >= m_cutTextPending) {
        char *text = new char [m_cutTextPending+1];
        m_clientSocket->read(text, m_cutTextPending);
        delete [] text;
        m_cutTextPending = 0;
        m_handleMsg = false;
    }
}

bool QVncClient::pixelConversionNeeded() const
{
    if (!m_sameEndian)
        return true;

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    if (server()->screen()->swapBytes())
        return true;
#endif

    const int screendepth = m_server->screen()->depth();
    if (screendepth != m_pixelFormat.bitsPerPixel)
        return true;

    switch (screendepth) {
    case 32:
    case 24:
        return false;
    case 16:
        return (m_pixelFormat.redBits == 5
                && m_pixelFormat.greenBits == 6
                && m_pixelFormat.blueBits == 5);
    }
    return true;
}

QT_END_NAMESPACE
