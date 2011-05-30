/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qmousepc_qws.h"

#ifndef QT_NO_QWS_MOUSE_PC

#include "qwindowsystem_qws.h"
#include "qsocketnotifier.h"
#include "qwsevent_qws.h"
#include "qwscommand_qws_p.h"
#include "qwsutils_qws.h"

#include "qapplication.h"
#include "qpolygon.h"
#include "qtimer.h"
#include "qfile.h"
#include "qtextstream.h"
#include "qstringlist.h"
#include <private/qcore_unix_p.h> // overrides QT_OPEN

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

#include <qscreen_qws.h>

QT_BEGIN_NAMESPACE

//#define QWS_MOUSE_DEBUG

/*
 * Automatic-detection mouse driver
 */

class QWSPcMouseSubHandler {
protected:
    enum { max_buf=32 };

    int fd;

    uchar buffer[max_buf];
    int nbuf;

    QPoint motion;
    int bstate;
    int wheel;

    int goodness;
    int badness;

    virtual int tryData()=0;

public:
    QWSPcMouseSubHandler(int f) : fd(f)
    {
        initState();
    }
    virtual ~QWSPcMouseSubHandler() {}

    int file() const { return fd; }

    void closeIfNot(int& f)
    {
        if (fd != f) {
            f = fd;
            QT_CLOSE(fd);
        }
    }

    void initState() { nbuf = bstate = goodness = badness = 0; }

    void worse(int by=1) { badness+=by; }
    bool reliable() const { return goodness >= 5 && badness < 50; }
    int buttonState() const { return bstate; }
    bool motionPending() const { return motion!=QPoint(0,0); }
    QPoint takeMotion() { QPoint r=motion; motion=QPoint(0,0); return r; }
    int takeWheel() { int result = wheel; wheel = 0; return result; }

    void appendData(uchar* data, int length)
    {
        memcpy(buffer+nbuf, data, length);
        nbuf += length;
    }

    enum UsageResult { Insufficient, Motion, Button };

    UsageResult useData()
    {
        int pbstate = bstate;
        int n = tryData();
#ifdef QWS_MOUSE_DEBUG
        if (n) {
            fprintf(stderr, "QWSPcMouseSubHandler tryData read %d bytes:", n);
            for (int i=0; i<n; ++i)
                fprintf(stderr, " %02x", buffer[i]);
            fprintf(stderr, "\n");
        }
#endif
        if (n > 0) {
            if (n<nbuf)
                memmove(buffer, buffer+n, nbuf-n);
            nbuf -= n;
            return (wheel || pbstate != bstate) ? Button : Motion;
        }
        return Insufficient;
    }
};

class QWSPcMouseSubHandler_intellimouse : public QWSPcMouseSubHandler {
    int packetsize;
public:
    QWSPcMouseSubHandler_intellimouse(int f) : QWSPcMouseSubHandler(f)
    {
        init();
    }

    void init()
    {
        int n;
        uchar reply[20];

        if (tcflush(fd,TCIOFLUSH) == -1) {
#ifdef QWS_MOUSE_DEBUG
            perror("QWSPcMouseSubHandler_intellimouse: pre-init tcflush");
#endif
        }
        static const uchar initseq[] = { 243, 200, 243, 100, 243, 80 };
        static const uchar query[] = { 0xf2 };
        if (QT_WRITE(fd, initseq, sizeof(initseq))!=sizeof(initseq)) {
            badness = 100;
            return;
        }
        usleep(10000);
        if (tcflush(fd,TCIOFLUSH) == -1) {
#ifdef QWS_MOUSE_DEBUG
            perror("QWSPcMouseSubHandler_intellimouse: post-init tcflush");
#endif
        }
        if (QT_WRITE(fd, query, sizeof(query))!=sizeof(query)) {
            badness = 100;
            return;
        }
        usleep(10000);
        n = QT_READ(fd, reply, 20);
        if (n > 0) {
            goodness = 10;
            switch (reply[n-1]) {
              case 3:
              case 4:
                packetsize = 4;
                break;
             default:
                packetsize = 3;
            }
        } else {
            badness = 100;
        }
    }

    int tryData()
    {
        if (nbuf >= packetsize) {
            //int overflow = (buffer[0]>>6)& 0x03;

            if (/*overflow ||*/ !(buffer[0] & 8)) {
#ifdef QWS_MOUSE_DEBUG
                qDebug("Intellimouse: skipping (overflow)");
#endif
                badness++;
                return 1;
            } else {
                QPoint delta((buffer[0] & 0x10) ? buffer[1]-256 : buffer[1],
                       (buffer[0] & 0x20) ? 256-buffer[2] : -buffer[2]);
                motion += delta;
                int nbstate = buffer[0] & 0x7;
#ifdef QWS_MOUSE_DEBUG
                int debugwheel =
#endif
                wheel = packetsize > 3 ? -(signed char)buffer[3] : 0;
                if (wheel < -2 || wheel > 2)
                    wheel = 0;
                wheel *= 120; // WHEEL_DELTA?
#ifdef QWS_MOUSE_DEBUG
                qDebug("Intellimouse: motion %d,%d, state %d, raw wheel %d, wheel %d", motion.x(), motion.y(), nbstate, debugwheel, wheel);
#endif
                if (motion.x() || motion.y() || bstate != nbstate || wheel) {
                    bstate = nbstate;
                    goodness++;
                } else {
                    badness++;
                    return 1;
                }
            }
            return packetsize;
        }
        return 0;
    }
};

class QWSPcMouseSubHandler_mouseman : public QWSPcMouseSubHandler {
    int packetsize;
public:
    QWSPcMouseSubHandler_mouseman(int f) : QWSPcMouseSubHandler(f)
    {
        init();
    }

    void init()
    {
        if (tcflush(fd,TCIOFLUSH) == -1) {
#ifdef QWS_MOUSE_DEBUG
            perror("QWSPcMouseSubHandler_mouseman: initial tcflush");
#endif
        }
        QT_WRITE(fd,"",1);
        usleep(50000);
        QT_WRITE(fd,"@EeI!",5);
        usleep(10000);
        static const unsigned char ibuf[] = { 246, 244 };
        QT_WRITE(fd,ibuf,1);
        QT_WRITE(fd,ibuf+1,1);
        if (tcflush(fd,TCIOFLUSH) == -1) {
#ifdef QWS_MOUSE_DEBUG
            perror("QWSPcMouseSubHandler_mouseman: tcflush");
#endif
        }
        usleep(10000);

        char buf[100];
        while (QT_READ(fd, buf, 100) > 0) { }  // eat unwanted replies
    }

    int tryData()
    {
        if (nbuf >= 3) {
            int nbstate = 0;
            if (buffer[0] & 0x01)
                nbstate |= Qt::LeftButton;
            if (buffer[0] & 0x02)
                nbstate |= Qt::RightButton;
            if (buffer[0] & 0x04)
                nbstate |= Qt::MidButton;

            int overflow = (buffer[0]>>6)& 0x03;
            if (overflow) {
                //### wheel events signalled with overflow bit, ignore for now
                badness++;
                return 1;
            } else {
                bool xs = buffer[0] & 0x10;
                bool ys = buffer[0] & 0x20;
                int dx = xs ? buffer[1]-256 : buffer[1];
                int dy = ys ? buffer[2]-256 : buffer[2];

                motion += QPoint(dx, -dy);
                if (motion.x() || motion.y() || bstate != nbstate) {
                    bstate = nbstate;
                    goodness++;
                } else {
                    badness++;
                    return 1;
                }
            }
            return 3;
        }
        return 0;
    }
};

class QWSPcMouseSubHandler_serial : public QWSPcMouseSubHandler {
public:
    QWSPcMouseSubHandler_serial(int f) : QWSPcMouseSubHandler(f)
    {
        initSerial();
    }

protected:
    void setflags(int f)
    {
        termios tty;
        if (tcgetattr(fd, &tty) == -1) {
#ifdef QWS_MOUSE_DEBUG
            perror("QWSPcMouseSubHandler_serial: tcgetattr");
#endif
        }
        tty.c_iflag     = IGNBRK | IGNPAR;
        tty.c_oflag     = 0;
        tty.c_lflag     = 0;
        tty.c_cflag     = f | CREAD | CLOCAL | HUPCL;
#ifdef Q_OS_LINUX
        tty.c_line      = 0;
#endif
        tty.c_cc[VTIME] = 0;
        tty.c_cc[VMIN]  = 1;
        if (tcsetattr(fd, TCSANOW, &tty) == -1) {
#ifdef QWS_MOUSE_DEBUG
            perror("QWSPcMouseSubHandler_serial: tcgetattr");
#endif
        }
    }

private:
    void initSerial()
    {
        int speed[4] = { B9600, B4800, B2400, B1200 };

        for (int n = 0; n < 4; n++) {
            setflags(CSTOPB | speed[n]);
            QT_WRITE(fd, "*q", 2);
            usleep(10000);
        }
    }
};

class QWSPcMouseSubHandler_mousesystems : public QWSPcMouseSubHandler_serial {
public:
    // ##### This driver has not been tested

    QWSPcMouseSubHandler_mousesystems(int f) : QWSPcMouseSubHandler_serial(f)
    {
        init();
    }

    void init()
    {
        setflags(B1200|CS8|CSTOPB);
        // 60Hz
        if (QT_WRITE(fd, "R", 1)!=1) {
            badness = 100;
            return;
        }
        if (tcflush(fd,TCIOFLUSH) == -1) {
#ifdef QT_QWS_VNC_DEBUG
            perror("QWSPcMouseSubHandler_mousesystems: tcflush");
#endif
        }
    }

    int tryData()
    {
        if (nbuf >= 5) {
            if ((buffer[0] & 0xf8) != 0x80) {
                badness++;
                return 1;
            }
            motion +=
                QPoint((signed char)buffer[1] + (signed char)buffer[3],
                       -(signed char)buffer[2] + (signed char)buffer[4]);
            int t = ~buffer[0];
            int nbstate = ((t&3) << 1) | ((t&4) >> 2);
            if (motion.x() || motion.y() || bstate != nbstate) {
                bstate = nbstate;
                goodness++;
            } else {
                badness++;
                return 1;
            }
            return 5;
        }
        return 0;
    }
};

class QWSPcMouseSubHandler_ms : public QWSPcMouseSubHandler_serial {
    int mman;
public:
    QWSPcMouseSubHandler_ms(int f) : QWSPcMouseSubHandler_serial(f)
    {
        mman=0;
        init();
    }

    void init()
    {
        setflags(B1200|CS7);
        // 60Hz
        if (QT_WRITE(fd, "R", 1)!=1) {
            badness = 100;
            return;
        }
        if (tcflush(fd,TCIOFLUSH) == -1) {
#ifdef QWS_MOUSE_DEBUG
            perror("QWSPcMouseSubHandler_ms: tcflush");
#endif
        }
    }

    int tryData()
    {
        if (!(buffer[0] & 0x40)) {
            if (buffer[0] == 0x20 && (bstate & Qt::MidButton)) {
                mman=1; // mouseman extension
            }
            return 1;
        }
        int extra = mman&&(bstate & Qt::MidButton);
        if (nbuf >= 3+extra) {
            int nbstate = 0;
            if (buffer[0] == 0x40 && !bstate && !buffer[1] && !buffer[2]) {
                nbstate = Qt::MidButton;
            } else {
                nbstate = ((buffer[0] & 0x20) >> 5)
                        | ((buffer[0] & 0x10) >> 3);
                if (extra && buffer[3] == 0x20)
                    nbstate = Qt::MidButton;
            }

            if (buffer[1] & 0x40) {
                badness++;
                return 1;
            } else {
                motion +=
                    QPoint((signed char)((buffer[0]&0x3)<<6)
                            |(signed char)(buffer[1]&0x3f),
                           (signed char)((buffer[0]&0xc)<<4)
                            |(signed char)(buffer[2]&0x3f));
                if (motion.x() || motion.y() || bstate != nbstate) {
                    bstate = nbstate;
                    goodness++;
                } else {
                    badness++;
                    return 1;
                }
                return 3+extra;
            }
        }
        return 0;
    }
};

//===========================================================================

class QWSPcMouseHandlerPrivate : public QObject
{
    Q_OBJECT
public:
    QWSPcMouseHandlerPrivate(QWSPcMouseHandler *h, const QString &, const QString &);
    ~QWSPcMouseHandlerPrivate();

    void suspend();
    void resume();

private:
    enum { max_dev=32 };
    QWSPcMouseSubHandler *sub[max_dev];
    QList<QSocketNotifier*> notifiers;
    int nsub;
    int retries;

private slots:
    void readMouseData(int);

private:
    void openDevices();
    void closeDevices();
    void notify(int fd);
    bool sendEvent(QWSPcMouseSubHandler& h);

private:
    QWSPcMouseHandler *handler;
    QString driver;
    QString device;
    qreal accel;
    int accel_limit;
};

QWSPcMouseHandler::QWSPcMouseHandler(const QString &driver, const QString &device)
    : QWSMouseHandler(driver, device)
{
    d = new QWSPcMouseHandlerPrivate(this, driver, device);
}

QWSPcMouseHandler::~QWSPcMouseHandler()
{
    delete d;
}

void QWSPcMouseHandler::suspend()
{
    d->suspend();
}

void QWSPcMouseHandler::resume()
{
    d->resume();
}


QWSPcMouseHandlerPrivate::QWSPcMouseHandlerPrivate(QWSPcMouseHandler *h,
    const QString &drv, const QString &arg)
    : handler(h), driver(drv)
{
    QStringList args = arg.split(QLatin1Char(':'), QString::SkipEmptyParts);

    int index;

    accel = qreal(2.0);
    QRegExp accelRegex(QLatin1String("^accel=(\\d+\\.?\\d*)$"));
    index = args.indexOf(accelRegex);
    if (index >= 0) {
        accel = qreal(accelRegex.cap(1).toDouble());
        args.removeAt(index);
    }

    accel_limit = 5;
    QRegExp accelLimitRegex(QLatin1String("^accel_limit=(\\d+)$"));
    index = args.indexOf(accelLimitRegex);
    if (index >= 0) {
        accel_limit = accelLimitRegex.cap(1).toInt();
        args.removeAt(index);
    }

    device = args.join(QString());

    retries = 0;
    openDevices();
}

QWSPcMouseHandlerPrivate::~QWSPcMouseHandlerPrivate()
{
    closeDevices();
}

/*
QWSPcMouseHandler::UsageResult QWSPcMouseHandler::useDev(Dev& d)
{
    if (d.nbuf >= mouseData[d.protocol].bytesPerPacket) {
        uchar *mb = d.buf;
        int bstate = 0;
        int dx = 0;
        int dy = 0;

        switch (mouseProtocol) {
            case MouseMan:
            case IntelliMouse:
            {
                bstate = mb[0] & 0x7; // assuming Qt::*Button order

                int overflow = (mb[0]>>6)& 0x03;
                if (mouseProtocol == MouseMan && overflow) {
                    //### wheel events signalled with overflow bit, ignore for now
                }
                else {
                    bool xs = mb[0] & 0x10;
                    bool ys = mb[0] & 0x20;
                    dx = xs ? mb[1]-256 : mb[1];
                    dy = ys ? mb[2]-256 : mb[2];
                }
                break;
            }
            case Microsoft:
                if (((mb[0] & 0x20) >> 3)) {
                    bstate |= Qt::LeftButton;
                }
                if (((mb[0] & 0x10) >> 4)) {
                    bstate |= Qt::RightButton;
                }

                dx=(signed char)(((mb[0] & 0x03) << 6) | (mb[1] & 0x3f));
                dy=-(signed char)(((mb[0] & 0x0c) << 4) | (mb[2] & 0x3f));

                break;
        }
    }
    }
*/


bool QWSPcMouseHandlerPrivate::sendEvent(QWSPcMouseSubHandler& h)
{
    if (h.reliable()) {
        QPoint motion = h.takeMotion();
        if (qAbs(motion.x()) > accel_limit || qAbs(motion.y()) > accel_limit)
            motion *= accel;
        QPoint newPos = handler->pos() + motion;
        if (qt_screen->isTransformed()) {
            QSize s = QSize(qt_screen->width(), qt_screen->height());
            newPos = qt_screen->mapToDevice(newPos, s);
        }
        handler->limitToScreen(newPos);

        handler->mouseChanged(newPos, h.buttonState(), h.takeWheel());
        return true;
    } else {
        h.takeMotion();
        if (h.buttonState() & (Qt::RightButton|Qt::MidButton)) {
            // Strange for the user to press right or middle without
            // a moving mouse!
            h.worse();
        }
        return false;
    }
}

void QWSPcMouseHandlerPrivate::openDevices()
{
    nsub=0;
    int fd = -1;

    QString drv = driver.toLower();
    if (!drv.isEmpty() && drv != QLatin1String("auto")) {
        // Manually specified mouse
        QByteArray dev = device.toLatin1();
        if (drv == QLatin1String("intellimouse")) {
            if (dev.isEmpty())
                dev = "/dev/psaux";
            fd = QT_OPEN(dev, O_RDWR | O_NDELAY);
            if (fd >= 0)
                sub[nsub++] = new QWSPcMouseSubHandler_intellimouse(fd);
        } else if (drv == QLatin1String("microsoft")) {
            if (dev.isEmpty())
                dev = "/dev/ttyS0";
            fd = QT_OPEN(dev, O_RDWR | O_NDELAY);
            if (fd >= 0)
                sub[nsub++] = new QWSPcMouseSubHandler_ms(fd);
        } else if (drv == QLatin1String("mousesystems")) {
            if (dev.isEmpty())
                dev = "/dev/ttyS0";
            fd = QT_OPEN(dev, O_RDWR | O_NDELAY);
            if (fd >= 0)
                sub[nsub++] = new QWSPcMouseSubHandler_mousesystems(fd);
        } else if (drv == QLatin1String("mouseman")) {
            if (dev.isEmpty())
                dev = "/dev/psaux";
            fd = QT_OPEN(dev, O_RDWR | O_NDELAY);
            if (fd >= 0)
                sub[nsub++] = new QWSPcMouseSubHandler_mouseman(fd);
        }
        if (fd >= 0)
            notify(fd);
        else
            qCritical("Error opening mouse device '%s': %s",
                      dev.constData(), strerror(errno));
    } else {
        // Try automatically
        fd = QT_OPEN("/dev/psaux", O_RDWR | O_NDELAY);
        if (fd >= 0) {
            sub[nsub++] = new QWSPcMouseSubHandler_intellimouse(fd);
            notify(fd);
        }
        fd = QT_OPEN("/dev/input/mice", O_RDWR | O_NDELAY);
        if (fd >= 0) {
            sub[nsub++] = new QWSPcMouseSubHandler_intellimouse(fd);
            notify(fd);
            //qDebug("/dev/input/mice fd %d #%d", fd, nsub-1);
        }

// include the code below to auto-detect serial mice, and to mess up
// any sort of serial communication
#if 0
        const char fn[4][11] = { "/dev/ttyS0", "/dev/ttyS1", "/dev/ttyS2", "/dev/ttyS3" };
        for (int ch = 0; ch < 4; ++ch) {
            fd = QT_OPEN(fn[ch], O_RDWR | O_NDELAY);
            if (fd >= 0) {
                //sub[nsub++] = new QWSPcMouseSubHandler_intellimouse(fd);
                sub[nsub++] = new QWSPcMouseSubHandler_mousesystems(fd);
                sub[nsub++] = new QWSPcMouseSubHandler_ms(fd);
                notify(fd);
            }
        }
#endif
    }
}

void QWSPcMouseHandlerPrivate::closeDevices()
{
    int pfd=-1;
    for (int i=0; i<nsub; i++) {
        sub[i]->closeIfNot(pfd);
        delete sub[i];
    }
    qDeleteAll(notifiers);
    notifiers.clear();
}

void QWSPcMouseHandlerPrivate::suspend()
{
    for (int i=0; i<notifiers.size(); ++i)
        notifiers.at(i)->setEnabled(false);
}

void QWSPcMouseHandlerPrivate::resume()
{
    for (int i=0; i<nsub; i++)
        sub[i]->initState();

    for (int i=0; i<notifiers.size(); ++i)
        notifiers.at(i)->setEnabled(true);
}



void QWSPcMouseHandlerPrivate::notify(int fd)
{
    QSocketNotifier *mouseNotifier
        = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(mouseNotifier, SIGNAL(activated(int)),this, SLOT(readMouseData(int)));
    notifiers.append(mouseNotifier);
}

void QWSPcMouseHandlerPrivate::readMouseData(int fd)
{
    for (;;) {
        uchar buf[8];
        int n = read(fd, buf, 8);
        if (n<=0)
            break;
        for (int i=0; i<nsub; i++) {
            QWSPcMouseSubHandler& h = *sub[i];
            if (h.file() == fd) {
                h.appendData(buf,n);
                for (;;) {
                    switch (h.useData()) {
                      case QWSPcMouseSubHandler::Button:
                        sendEvent(h);
                        break;
                      case QWSPcMouseSubHandler::Insufficient:
                        goto breakbreak;
                      case QWSPcMouseSubHandler::Motion:
                        break;
                    }
                }
                breakbreak:
                    ;
            }
        }
    }
    bool any_reliable=false;
    for (int i=0; i<nsub; i++) {
        QWSPcMouseSubHandler& h = *sub[i];
        if (h.motionPending())
            sendEvent(h);
        any_reliable = any_reliable || h.reliable();
    }
    if (any_reliable) {
        // ... get rid of all unreliable ones?  All bad ones?
    } else if (retries < 2) {
        // Try again - maybe the mouse was being moved when we tried to init.
        closeDevices();
        openDevices();
        retries++;
    }
}

QT_END_NAMESPACE

#include "qmousepc_qws.moc"

#endif // QT_NO_MOUSE_PC
