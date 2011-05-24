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

#include "qmeegolivepixmapdata.h"
#include "qmeegorasterpixmapdata.h"
#include <private/qimage_p.h>
#include <private/qwindowsurface_gl_p.h>
#include <private/qeglcontext_p.h>
#include <private/qapplication_p.h>
#include <private/qgraphicssystem_runtime_p.h>
#include <private/qpixmap_x11_p.h>
#include <stdio.h>

static QMeeGoLivePixmapDataList all_live_pixmaps;

static EGLint lock_attribs[] = {
    EGL_MAP_PRESERVE_PIXELS_KHR, EGL_TRUE,
    EGL_LOCK_USAGE_HINT_KHR, EGL_READ_SURFACE_BIT_KHR | EGL_WRITE_SURFACE_BIT_KHR,
    EGL_NONE
};

static EGLint preserved_attribs[] = {
    EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
    EGL_NONE
};

// as copied from qwindowsurface.cpp
void qt_scrollRectInImage(QImage &img, const QRect &rect, const QPoint &offset)
{
    // make sure we don't detach
    uchar *mem = const_cast<uchar*>(const_cast<const QImage &>(img).bits());

    int lineskip = img.bytesPerLine();
    int depth = img.depth() >> 3;

    const QRect imageRect(0, 0, img.width(), img.height());
    const QRect r = rect & imageRect & imageRect.translated(-offset);
    const QPoint p = rect.topLeft() + offset;

    if (r.isEmpty())
        return;

    const uchar *src;
    uchar *dest;

    if (r.top() < p.y()) {
        src = mem + r.bottom() * lineskip + r.left() * depth;
        dest = mem + (p.y() + r.height() - 1) * lineskip + p.x() * depth;
        lineskip = -lineskip;
    } else {
        src = mem + r.top() * lineskip + r.left() * depth;
        dest = mem + p.y() * lineskip + p.x() * depth;
    }

    const int w = r.width();
    int h = r.height();
    const int bytes = w * depth;

    // overlapping segments?
    if (offset.y() == 0 && qAbs(offset.x()) < w) {
        do {
            ::memmove(dest, src, bytes);
            dest += lineskip;
            src += lineskip;
        } while (--h);
    } else {
        do {
            ::memcpy(dest, src, bytes);
            dest += lineskip;
            src += lineskip;
        } while (--h);
    }
}

/* Public */

QMeeGoLivePixmapData::QMeeGoLivePixmapData(int w, int h, QImage::Format format) : QGLPixmapData(QPixmapData::PixmapType)
{
    QImage image(w, h, format);
    QX11PixmapData *pmd = new QX11PixmapData(QPixmapData::PixmapType);
    pmd->fromImage(image, Qt::NoOpaqueDetection);
    backingX11Pixmap = new QPixmap(pmd);

    initializeThroughEGLImage();

    pos = all_live_pixmaps.insert(all_live_pixmaps.begin(), this);
}

QMeeGoLivePixmapData::QMeeGoLivePixmapData(Qt::HANDLE h) : QGLPixmapData(QPixmapData::PixmapType)
{
    backingX11Pixmap = new QPixmap(QPixmap::fromX11Pixmap(h));
    initializeThroughEGLImage();

    pos = all_live_pixmaps.insert(all_live_pixmaps.begin(), this);
}

QMeeGoLivePixmapData::~QMeeGoLivePixmapData()
{
    delete backingX11Pixmap;
    all_live_pixmaps.erase(pos);
}

void QMeeGoLivePixmapData::initializeThroughEGLImage()
{
    if (texture()->id != 0)
        return;

    QGLShareContextScope ctx(qt_gl_share_widget()->context());
    QMeeGoExtensions::ensureInitialized();

    EGLImageKHR eglImage = EGL_NO_IMAGE_KHR;
    GLuint newTextureId = 0;

    eglImage = QEgl::eglCreateImageKHR(QEgl::display(), EGL_NO_CONTEXT, EGL_NATIVE_PIXMAP_KHR,
                                       (EGLClientBuffer) backingX11Pixmap->handle(), preserved_attribs);

    if (eglImage == EGL_NO_IMAGE_KHR) {
        qWarning("eglCreateImageKHR failed (live texture)!");
        return;
    }

    glGenTextures(1, &newTextureId);
    glBindTexture(GL_TEXTURE_2D, newTextureId);

    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (EGLImageKHR) eglImage);
    if (glGetError() == GL_NO_ERROR) {
        resize(backingX11Pixmap->width(), backingX11Pixmap->height());
        texture()->id = newTextureId;
        texture()->options &= ~QGLContext::InvertedYBindOption;
        m_hasAlpha = backingX11Pixmap->hasAlphaChannel();
    } else {
        qWarning("Failed to create a texture from an egl image (live texture)!");
        glDeleteTextures(1, &newTextureId);
    }

    QEgl::eglDestroyImageKHR(QEgl::display(), eglImage);
}

QPixmapData *QMeeGoLivePixmapData::createCompatiblePixmapData() const
{
    qWarning("Create compatible called on live pixmap! Expect fail soon...");
    return new QMeeGoRasterPixmapData(pixelType());
}

QImage* QMeeGoLivePixmapData::lock(EGLSyncKHR fenceSync)
{
    QGLShareContextScope ctx(qt_gl_share_widget()->context());
    QMeeGoExtensions::ensureInitialized();

    if (fenceSync) {
        QMeeGoExtensions::eglClientWaitSyncKHR(QEgl::display(),
                                               fenceSync,
                                               EGL_SYNC_FLUSH_COMMANDS_BIT_KHR,
                                               EGL_FOREVER_KHR);
    }

    void *data = 0;
    int pitch = 0;
    int surfaceWidth = 0;
    int surfaceHeight = 0;
    EGLSurface surface = 0;
    QImage::Format format;
    lockedImage = QImage();

    surface = getSurfaceForBackingPixmap();
    if (! QMeeGoExtensions::eglLockSurfaceKHR(QEgl::display(), surface, lock_attribs)) {
        qWarning("Failed to lock surface (live texture)!");
        return &lockedImage;
    }

    eglQuerySurface(QEgl::display(), surface, EGL_BITMAP_POINTER_KHR, (EGLint*) &data);
    eglQuerySurface(QEgl::display(), surface, EGL_BITMAP_PITCH_KHR, (EGLint*) &pitch);
    eglQuerySurface(QEgl::display(), surface, EGL_WIDTH, (EGLint*) &surfaceWidth);
    eglQuerySurface(QEgl::display(), surface, EGL_HEIGHT, (EGLint*) &surfaceHeight);

    // Ok, here we know we just support those two formats. Real solution would be:
    // query also the format.
    if (backingX11Pixmap->depth() > 16)
        format = QImage::Format_ARGB32_Premultiplied;
    else
        format = QImage::Format_RGB16;

    if (data == NULL || pitch == 0) {
        qWarning("Failed to query the live texture!");
        return &lockedImage;
    }

    if (width() != surfaceWidth || height() != surfaceHeight) {
        qWarning("Live texture dimensions don't match!");
        QMeeGoExtensions::eglUnlockSurfaceKHR(QEgl::display(), surface);
        return &lockedImage;
    }

    lockedImage = QImage((uchar *) data, width(), height(), pitch, format);
    return &lockedImage;
}

bool QMeeGoLivePixmapData::release(QImage* /*img*/)
{
    QGLShareContextScope ctx(qt_gl_share_widget()->context());
    QMeeGoExtensions::ensureInitialized();

    if (QMeeGoExtensions::eglUnlockSurfaceKHR(QEgl::display(), getSurfaceForBackingPixmap())) {
        lockedImage = QImage();
        return true;
    } else {
        lockedImage = QImage();
        return false;
    }
}

Qt::HANDLE QMeeGoLivePixmapData::handle()
{
    return backingX11Pixmap->handle();
}

bool QMeeGoLivePixmapData::scroll(int dx, int dy, const QRect &rect)
{
    lock(NULL);

    if (!lockedImage.isNull())
        qt_scrollRectInImage(lockedImage, rect, QPoint(dx, dy));

    release(&lockedImage);
    return true;
}

EGLSurface QMeeGoLivePixmapData::getSurfaceForBackingPixmap()
{
    initializeThroughEGLImage();

    // This code is a crative remix of the stuff that can be found in the
    // Qt's TFP implementation in /src/opengl/qgl_x11egl.cpp ::bindiTextureFromNativePixmap
    QX11PixmapData *pixmapData = static_cast<QX11PixmapData*>(backingX11Pixmap->data_ptr().data());
    Q_ASSERT(pixmapData->classId() == QPixmapData::X11Class);
    bool hasAlpha = pixmapData->hasAlphaChannel();

    if (pixmapData->gl_surface &&
        hasAlpha == (pixmapData->flags & QX11PixmapData::GlSurfaceCreatedWithAlpha))
        return pixmapData->gl_surface;

    // Check to see if the surface is still valid
    if (pixmapData->gl_surface &&
        hasAlpha != ((pixmapData->flags & QX11PixmapData::GlSurfaceCreatedWithAlpha) > 0)) {
        // Surface is invalid!
        destroySurfaceForPixmapData(pixmapData);
    }

    if (pixmapData->gl_surface == 0) {
        EGLConfig config = QEgl::defaultConfig(QInternal::Pixmap,
                                               QEgl::OpenGL,
                                               hasAlpha ? QEgl::Translucent : QEgl::NoOptions);

        pixmapData->gl_surface = (void*)QEgl::createSurface(backingX11Pixmap, config);

        if (hasAlpha)
            pixmapData->flags |= QX11PixmapData::GlSurfaceCreatedWithAlpha;
        else
            pixmapData->flags &= ~QX11PixmapData::GlSurfaceCreatedWithAlpha;

        if (pixmapData->gl_surface == (void*)EGL_NO_SURFACE)
            return NULL;
    }

    return pixmapData->gl_surface;
}

void QMeeGoLivePixmapData::destroySurfaceForPixmapData(QPixmapData* pmd)
{
    Q_ASSERT(pmd->classId() == QPixmapData::X11Class);
    QX11PixmapData *pixmapData = static_cast<QX11PixmapData*>(pmd);
    if (pixmapData->gl_surface) {
        eglDestroySurface(QEgl::display(), (EGLSurface)pixmapData->gl_surface);
        pixmapData->gl_surface = 0;
    }
}

void QMeeGoLivePixmapData::invalidateSurfaces()
{
    foreach (QMeeGoLivePixmapData *data, all_live_pixmaps) {
        QX11PixmapData *pixmapData = static_cast<QX11PixmapData*>(data->backingX11Pixmap->data_ptr().data());
        *data->texture() = QGLTexture();
        pixmapData->gl_surface = 0;
    }
}
