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

#include "pvreglscreen.h"
#include "pvreglwindowsurface.h"
#include "pvrqwsdrawable_p.h"
#include <QRegExp>
#include <qwindowsystem_qws.h>
#ifndef QT_NO_QWS_TRANSFORMED
#include <qscreentransformed_qws.h>
#endif
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/kd.h>
#include <fcntl.h>
#include <unistd.h>

//![0]
PvrEglScreen::PvrEglScreen(int displayId)
    : QGLScreen(displayId)
{
    setOptions(NativeWindows);
    setSupportsBlitInClients(true);
    setSurfaceFunctions(new PvrEglScreenSurfaceFunctions(this, displayId));
//![0]
    fd = -1;
    ttyfd = -1;
    doGraphicsMode = true;
    oldKdMode = KD_TEXT;
    parent = 0;

    // Make sure that the EGL layer is initialized and the drivers loaded.
    EGLDisplay dpy = eglGetDisplay((EGLNativeDisplayType)EGL_DEFAULT_DISPLAY);
    if (!eglInitialize(dpy, 0, 0))
        qWarning("Could not initialize EGL display - are the drivers loaded?");

    // Make sure that screen 0 is initialized.
    pvrQwsScreenWindow(0);
}

PvrEglScreen::~PvrEglScreen()
{
    if (fd >= 0)
        ::close(fd);
}

bool PvrEglScreen::initDevice()
{
    openTty();
    return true;
}

bool PvrEglScreen::connect(const QString &displaySpec)
{
    if (!pvrQwsDisplayOpen())
        return false;

    // Initialize the QScreen properties.
    data = (uchar *)(pvrQwsDisplay.screens[0].mapped);
    w = pvrQwsDisplay.screens[0].screenRect.width;
    h = pvrQwsDisplay.screens[0].screenRect.height;
    lstep = pvrQwsDisplay.screens[0].screenStride;
    dw = w;
    dh = h;
    size = h * lstep;
    mapsize = size;
    switch (pvrQwsDisplay.screens[0].pixelFormat) {
	case PVR2D_RGB565:
            d = 16;
            setPixelFormat(QImage::Format_RGB16);
            break;
	case PVR2D_ARGB4444:
            d = 16;
            setPixelFormat(QImage::Format_ARGB4444_Premultiplied);
            break;
	case PVR2D_ARGB8888:
            d = 32;
            setPixelFormat(QImage::Format_ARGB32_Premultiplied);
            break;
        default:
            pvrQwsDisplayClose();
            qWarning("PvrEglScreen::connect: unsupported pixel format %d", (int)(pvrQwsDisplay.screens[0].pixelFormat));
            return false;
    }

    // Handle display physical size spec.
    QStringList displayArgs = displaySpec.split(QLatin1Char(':'));
    QRegExp mmWidthRx(QLatin1String("mmWidth=?(\\d+)"));
    int dimIdxW = displayArgs.indexOf(mmWidthRx);
    QRegExp mmHeightRx(QLatin1String("mmHeight=?(\\d+)"));
    int dimIdxH = displayArgs.indexOf(mmHeightRx);
    if (dimIdxW >= 0) {
        mmWidthRx.exactMatch(displayArgs.at(dimIdxW));
        physWidth = mmWidthRx.cap(1).toInt();
        if (dimIdxH < 0)
            physHeight = dh*physWidth/dw;
    }
    if (dimIdxH >= 0) {
        mmHeightRx.exactMatch(displayArgs.at(dimIdxH));
        physHeight = mmHeightRx.cap(1).toInt();
        if (dimIdxW < 0)
            physWidth = dw*physHeight/dh;
    }
    if (dimIdxW < 0 && dimIdxH < 0) {
        const int dpi = 72;
        physWidth = qRound(dw * 25.4 / dpi);
        physHeight = qRound(dh * 25.4 / dpi);
    }

    // Find the name of the tty device to use.
    QRegExp ttyRegExp(QLatin1String("tty=(.*)"));
    if (displayArgs.indexOf(ttyRegExp) != -1)
        ttyDevice = ttyRegExp.cap(1);
    if (displayArgs.contains(QLatin1String("nographicsmodeswitch")))
        doGraphicsMode = false;

    // The screen is ready.
    return true;
}

void PvrEglScreen::disconnect()
{
    pvrQwsDisplayClose();
}

void PvrEglScreen::shutdownDevice()
{
    closeTty();
}

void PvrEglScreen::blit(const QImage &img, const QPoint &topLeft, const QRegion &region)
{
    QGLScreen::blit(img, topLeft, region);
    sync();
}

void PvrEglScreen::solidFill(const QColor &color, const QRegion &region)
{
    QGLScreen::solidFill(color, region);
    sync();
}

bool PvrEglScreen::chooseContext
    (QGLContext *context, const QGLContext *shareContext)
{
    // We use PvrEglScreenSurfaceFunctions instead.
    Q_UNUSED(context);
    Q_UNUSED(shareContext);
    return false;
}

bool PvrEglScreen::hasOpenGL()
{
    return true;
}

//![1]
QWSWindowSurface* PvrEglScreen::createSurface(QWidget *widget) const
{
    if (qobject_cast<QGLWidget*>(widget))
        return new PvrEglWindowSurface(widget, (PvrEglScreen *)this, displayId);

    return QScreen::createSurface(widget);
}

QWSWindowSurface* PvrEglScreen::createSurface(const QString &key) const
{
    if (key == QLatin1String("PvrEgl"))
        return new PvrEglWindowSurface();

    return QScreen::createSurface(key);
}
//![1]

#ifndef QT_NO_QWS_TRANSFORMED

static const QScreen *parentScreen
    (const QScreen *current, const QScreen *lookingFor)
{
    if (!current)
        return 0;
    switch (current->classId()) {
    case QScreen::ProxyClass:
    case QScreen::TransformedClass: {
        const QScreen *child =
            static_cast<const QProxyScreen *>(current)->screen();
        if (child == lookingFor)
            return current;
        else
            return parentScreen(child, lookingFor);
    }
    // Not reached.

    case QScreen::MultiClass: {
        QList<QScreen *> screens = current->subScreens();
        foreach (QScreen *screen, screens) {
            if (screen == lookingFor)
                return current;
            const QScreen *parent = parentScreen(screen, lookingFor);
            if (parent)
                return parent;
        }
    }
    break;

    default: break;
    }
    return 0;
}

int PvrEglScreen::transformation() const
{
    // We need to search for our parent screen, which is assumed to be
    // "Transformed".  If it isn't, then there is no transformation.
    // There is no direct method to get the parent screen so we need
    // to search every screen until we find ourselves.
    if (!parent && qt_screen != this)
        parent = parentScreen(qt_screen, this);
    if (!parent)
        return 0;
    if (parent->classId() != QScreen::TransformedClass)
        return 0;
    return 90 * static_cast<const QTransformedScreen *>(parent)
                    ->transformOrientation();
}

#else

int PvrEglScreen::transformation() const
{
    return 0;
}

#endif

void PvrEglScreen::sync()
{
    // Put code here to synchronize 2D and 3D operations if necessary.
}

void PvrEglScreen::openTty()
{
    const char *const devs[] = {"/dev/tty0", "/dev/tty", "/dev/console", 0};

    if (ttyDevice.isEmpty()) {
        for (const char * const *dev = devs; *dev; ++dev) {
            ttyfd = ::open(*dev, O_RDWR);
            if (ttyfd != -1)
                break;
        }
    } else {
        ttyfd = ::open(ttyDevice.toAscii().constData(), O_RDWR);
    }

    if (ttyfd == -1)
        return;

    ::fcntl(ttyfd, F_SETFD, FD_CLOEXEC);

    if (doGraphicsMode) {
        ioctl(ttyfd, KDGETMODE, &oldKdMode);
        if (oldKdMode != KD_GRAPHICS) {
            int ret = ioctl(ttyfd, KDSETMODE, KD_GRAPHICS);
            if (ret == -1)
                doGraphicsMode = false;
        }
    }

    // No blankin' screen, no blinkin' cursor!, no cursor!
    const char termctl[] = "\033[9;0]\033[?33l\033[?25l\033[?1c";
    ::write(ttyfd, termctl, sizeof(termctl));
}

void PvrEglScreen::closeTty()
{
    if (ttyfd == -1)
        return;

    if (doGraphicsMode)
        ioctl(ttyfd, KDSETMODE, oldKdMode);

    // Blankin' screen, blinkin' cursor!
    const char termctl[] = "\033[9;15]\033[?33h\033[?25h\033[?0c";
    ::write(ttyfd, termctl, sizeof(termctl));

    ::close(ttyfd);
    ttyfd = -1;
}

//![2]
bool PvrEglScreenSurfaceFunctions::createNativeWindow(QWidget *widget, EGLNativeWindowType *native)
{
//![2]
    QWSWindowSurface *surface =
        static_cast<QWSWindowSurface *>(widget->windowSurface());
    if (!surface) {
        // The widget does not have a surface yet, so give it one.
        surface = new PvrEglWindowSurface(widget, screen, displayId);
        widget->setWindowSurface(surface);
    } else if (surface->key() != QLatin1String("PvrEgl")) {
        // The application has attached a QGLContext to an ordinary QWidget.
        // Replace the widget's window surface with a new one that can do GL.
        QRect geometry = widget->frameGeometry();
        geometry.moveTo(widget->mapToGlobal(QPoint(0, 0)));
        surface = new PvrEglWindowSurface(widget, screen, displayId);
        surface->setGeometry(geometry);
        widget->setWindowSurface(surface);
        widget->setAttribute(Qt::WA_NoSystemBackground, true);
    }
    PvrEglWindowSurface *nsurface = static_cast<PvrEglWindowSurface*>(surface);
    *native = (EGLNativeWindowType)(nsurface->nativeDrawable());
    return true;
}
