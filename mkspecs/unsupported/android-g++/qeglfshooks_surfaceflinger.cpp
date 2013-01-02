/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the qmake spec of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeglfshooks.h"

#include <ui/DisplayInfo.h>
#include <ui/FramebufferNativeWindow.h>
#include <gui/SurfaceComposerClient.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>

using namespace android;

QT_BEGIN_NAMESPACE

class QEglFSPandaHooks : public QEglFSHooks
{
public:
    virtual EGLNativeWindowType createNativeWindow(const QSize &size, const QSurfaceFormat &format);
    virtual QSize screenSize() const;
    virtual int screenDepth() const;
private:
    // androidy things
    sp<android::SurfaceComposerClient> mSession;
    sp<android::SurfaceControl> mControl;
    sp<android::Surface> mAndroidSurface;
};

EGLNativeWindowType QEglFSPandaHooks::createNativeWindow(const QSize &size, const QSurfaceFormat &)
{
    Q_UNUSED(size);

    mSession = new SurfaceComposerClient();
    DisplayInfo dinfo;
    int status=0;
    status = mSession->getDisplayInfo(0, &dinfo);
    mControl = mSession->createSurface(
            0, dinfo.w, dinfo.h, PIXEL_FORMAT_RGB_888);
    SurfaceComposerClient::openGlobalTransaction();
    mControl->setLayer(0x40000000);
//    mControl->setAlpha(1);
    SurfaceComposerClient::closeGlobalTransaction();
    mAndroidSurface = mControl->getSurface();

    EGLNativeWindowType eglWindow = mAndroidSurface.get();
    return eglWindow;
}
QSize  QEglFSPandaHooks::screenSize() const
{
    static QSize size;

    if (size.isEmpty()) {
        int width = qgetenv("QT_QPA_EGLFS_WIDTH").toInt();
        int height = qgetenv("QT_QPA_EGLFS_HEIGHT").toInt();

        if (width && height) {
            // no need to read fb0
            size.setWidth(width);
            size.setHeight(height);
            return size;
        }

        struct fb_var_screeninfo vinfo;
        int fd = open("/dev/graphics/fb0", O_RDONLY);

        if (fd != -1) {
            if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) == -1)
                qWarning("Could not query variable screen info.");
            else
                size = QSize(vinfo.xres, vinfo.yres);

            close(fd);
        } else {
            qWarning("Failed to open /dev/graphics/fb0 to detect screen resolution.");
        }

        // override fb0 from environment var setting
        if (width)
            size.setWidth(width);
        if (height)
            size.setHeight(height);
    }

    return size;
}

int QEglFSPandaHooks::screenDepth() const
{
    static int depth = qgetenv("QT_QPA_EGLFS_DEPTH").toInt();

    if (depth == 0) {
        struct fb_var_screeninfo vinfo;
        int fd = open("/dev/graphics/fb0", O_RDONLY);

        if (fd != -1) {
            if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) == -1)
                qWarning("Could not query variable screen info.");
            else
                depth = vinfo.bits_per_pixel;

            close(fd);
        } else {
            qWarning("Failed to open /dev/graphics/fb0 to detect screen depth.");
        }
    }

    return depth == 0 ? 32 : depth;
}

static QEglFSPandaHooks eglFSPandaHooks;
QEglFSHooks *platformHooks = &eglFSPandaHooks;

QT_END_NAMESPACE
