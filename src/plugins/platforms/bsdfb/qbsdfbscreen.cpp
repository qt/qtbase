/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2015-2016 Oleksandr Tymoshenko <gonzo@bluezbox.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qbsdfbscreen.h"
#include <QtFbSupport/private/qfbcursor_p.h>
#include <QtFbSupport/private/qfbwindow_p.h>
#include <QtCore/QRegularExpression>
#include <QtGui/QPainter>

#include <private/qcore_unix_p.h> // overrides QT_OPEN
#include <qimage.h>
#include <qdebug.h>

#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <limits.h>
#include <signal.h>

#include <sys/consio.h>
#include <sys/fbio.h>

QT_BEGIN_NAMESPACE

enum {
    DefaultDPI = 100
};

static int openFramebufferDevice(const QString &dev)
{
    const QByteArray devPath = QFile::encodeName(dev);

    int fd = QT_OPEN(devPath.constData(), O_RDWR);

    if (fd == -1)
        fd = QT_OPEN(devPath.constData(), O_RDONLY);

    return fd;
}

static QRect determineGeometry(const struct fbtype &fb, const QRect &userGeometry)
{
    int xoff = 0;
    int yoff = 0;
    int w = 0;
    int h = 0;

    if (userGeometry.isValid()) {
        w = qMin(userGeometry.width(), fb.fb_width);
        h = qMin(userGeometry.height(), fb.fb_height);

        int xxoff = userGeometry.x(), yyoff = userGeometry.y();
        if (xxoff != 0 || yyoff != 0) {
            if (xxoff < 0 || xxoff + w > fb.fb_width)
                xxoff = fb.fb_width - w;
            if (yyoff < 0 || yyoff + h > fb.fb_height)
                yyoff = fb.fb_height - h;
            xoff += xxoff;
            yoff += yyoff;
        } else {
            xoff += (fb.fb_width - w)/2;
            yoff += (fb.fb_height - h)/2;
        }
    } else {
        w = fb.fb_width;
        h = fb.fb_height;
    }

    if (w == 0 || h == 0) {
        qWarning("Unable to find screen geometry, using 320x240");
        w = 320;
        h = 240;
    }

    return QRect(xoff, yoff, w, h);
}

static QSizeF determinePhysicalSize(const QSize &mmSize, const QSize &res)
{
    int mmWidth = mmSize.width();
    int mmHeight = mmSize.height();

    if (mmWidth <= 0 && mmHeight <= 0) {
        const int dpi = DefaultDPI;
        mmWidth = qRound(res.width() * 25.4 / dpi);
        mmHeight = qRound(res.height() * 25.4 / dpi);
    } else if (mmWidth > 0 && mmHeight <= 0) {
        mmHeight = res.height() * mmWidth/res.width();
    } else if (mmHeight > 0 && mmWidth <= 0) {
        mmWidth = res.width() * mmHeight/res.height();
    }

    return QSize(mmWidth, mmHeight);
}

QBsdFbScreen::QBsdFbScreen(const QStringList &args)
    : m_arguments(args)
{
}

QBsdFbScreen::~QBsdFbScreen()
{
    if (m_framebufferFd != -1) {
        munmap(m_mmap.data - m_mmap.offset, m_mmap.size);
        qt_safe_close(m_framebufferFd);
    }
}

bool QBsdFbScreen::initialize()
{
    QRegularExpression fbRx(QLatin1String("fb=(.*)"));
    QRegularExpression mmSizeRx(QLatin1String("mmsize=(\\d+)x(\\d+)"));
    QRegularExpression sizeRx(QLatin1String("size=(\\d+)x(\\d+)"));
    QRegularExpression offsetRx(QLatin1String("offset=(\\d+)x(\\d+)"));

    QString fbDevice;
    QSize userMmSize;
    QRect userGeometry;

    // Parse arguments
    for (const QString &arg : qAsConst(m_arguments)) {
        QRegularExpressionMatch match;
        if (arg.contains(mmSizeRx, &match))
            userMmSize = QSize(match.captured(1).toInt(), match.captured(2).toInt());
        else if (arg.contains(sizeRx, &match))
            userGeometry.setSize(QSize(match.captured(1).toInt(), match.captured(2).toInt()));
        else if (arg.contains(offsetRx, &match))
            userGeometry.setTopLeft(QPoint(match.captured(1).toInt(), match.captured(2).toInt()));
        else if (arg.contains(fbRx, &match))
            fbDevice = match.captured(1);
    }

    if (!fbDevice.isEmpty()) {
        // Open the device
        m_framebufferFd = openFramebufferDevice(fbDevice);
    } else {
        m_framebufferFd = STDIN_FILENO;
    }

    if (m_framebufferFd == -1) {
        qErrnoWarning(errno, "Failed to open framebuffer %s", qPrintable(fbDevice));
        return false;
    }

    struct fbtype fb;
    if (ioctl(m_framebufferFd, FBIOGTYPE, &fb) != 0) {
        qErrnoWarning(errno, "Error reading framebuffer information");
        return false;
    }

    int line_length = 0;
    if (ioctl(m_framebufferFd, FBIO_GETLINEWIDTH, &line_length) != 0) {
        qErrnoWarning(errno, "Error reading line length information");
        return false;
    }

    mDepth = fb.fb_depth;

    m_bytesPerLine = line_length;
    const QRect geometry = determineGeometry(fb, userGeometry);
    mGeometry = QRect(QPoint(0, 0), geometry.size());
    switch (mDepth) {
    case 32:
        mFormat = QImage::Format_RGB32;
        break;
    case 24:
        mFormat = QImage::Format_RGB888;
        break;
    case 16:
        // falling back
    default:
        mFormat = QImage::Format_RGB16;
        break;
    }
    mPhysicalSize = determinePhysicalSize(userMmSize, geometry.size());

    // mmap the framebuffer
    const size_t pagemask = getpagesize() - 1;
    m_mmap.size = (m_bytesPerLine * fb.fb_height + pagemask) & ~pagemask;
    uchar *data = static_cast<uchar*>(mmap(nullptr, m_mmap.size, PROT_READ | PROT_WRITE, MAP_SHARED, m_framebufferFd, 0));
    if (data == MAP_FAILED) {
        qErrnoWarning(errno, "Failed to mmap framebuffer");
        return false;
    }

    m_mmap.offset = geometry.y() * m_bytesPerLine + geometry.x() * mDepth / 8;
    m_mmap.data = data + m_mmap.offset;

    QFbScreen::initializeCompositor();
    m_onscreenImage = QImage(m_mmap.data, geometry.width(), geometry.height(), m_bytesPerLine, mFormat);

    mCursor = new QFbCursor(this);

    return true;
}

QRegion QBsdFbScreen::doRedraw()
{
    const QRegion touched = QFbScreen::doRedraw();

    if (touched.isEmpty())
        return touched;

    if (!m_blitter)
        m_blitter.reset(new QPainter(&m_onscreenImage));

    for (const QRect &rect : touched)
        m_blitter->drawImage(rect, mScreenImage, rect);
    return touched;
}

// grabWindow() grabs "from the screen" not from the backingstores.
QPixmap QBsdFbScreen::grabWindow(WId wid, int x, int y, int width, int height) const
{
    if (!wid) {
        if (width < 0)
            width = m_onscreenImage.width() - x;
        if (height < 0)
            height = m_onscreenImage.height() - y;
        return QPixmap::fromImage(m_onscreenImage).copy(x, y, width, height);
    }

    const QFbWindow *window = windowForId(wid);
    if (window) {
        const QRect geom = window->geometry();
        if (width < 0)
            width = geom.width() - x;
        if (height < 0)
            height = geom.height() - y;
        QRect rect(geom.topLeft() + QPoint(x, y), QSize(width, height));
        rect &= window->geometry();
        return QPixmap::fromImage(m_onscreenImage).copy(rect);
    }

    return QPixmap();
}

QT_END_NAMESPACE
