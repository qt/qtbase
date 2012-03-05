/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/
//#include <QDebug>
#include "qkmscursor.h"
#include "qkmsscreen.h"
#include "qkmsdevice.h"

QT_BEGIN_NAMESPACE

QKmsCursor::QKmsCursor(QKmsScreen *screen)
    : m_screen(screen),
      m_graphicsBufferManager(screen->device()->gbmDevice())
{
    gbm_bo *bo = gbm_bo_create(m_graphicsBufferManager, 64, 64,
                               GBM_BO_FORMAT_ARGB8888,
                               GBM_BO_USE_CURSOR_64X64 | GBM_BO_USE_RENDERING);

    m_eglImage = eglCreateImageKHR(m_screen->device()->eglDisplay(), 0, EGL_NATIVE_PIXMAP_KHR,
                                   bo, 0);
    gbm_bo_destroy(bo);
    m_cursorImage = new QPlatformCursorImage(0, 0, 0, 0, 0, 0);
}

void QKmsCursor::pointerEvent(const QMouseEvent &event)
{
    int status = drmModeMoveCursor(m_screen->device()->fd(),
                                   m_screen->crtcId(),
                                   event.globalX(),
                                   event.globalY());
    if (status) {
        qWarning("failed to move cursor: %d", status);
    }
}

void QKmsCursor::changeCursor(QCursor *widgetCursor, QWindow *window)
{
    Q_UNUSED(window)

    if (widgetCursor->shape() != Qt::BitmapCursor) {
        m_cursorImage->set(widgetCursor->shape());
    } else {
        m_cursorImage->set(widgetCursor->pixmap().toImage(),
                           widgetCursor->hotSpot().x(),
                           widgetCursor->hotSpot().y());
    }

    if ((m_cursorImage->image()->width() > 64) || (m_cursorImage->image()->width() > 64)) {
        qWarning("failed to set hardware cursor: larger than 64x64.");
        return;
    }

    QImage cursorImage = m_cursorImage->image()->convertToFormat(QImage::Format_RGB32);

    //Load cursor image into EGLImage
    GLuint cursorTexture;
    glGenTextures(1, &cursorTexture);
    glBindTexture(GL_TEXTURE_2D, cursorTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    //TODO: Format may be wrong here, need a color icon to test.
    if (m_eglImage != EGL_NO_IMAGE_KHR) {
        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_eglImage);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cursorImage.width(),
                        cursorImage.height(), GL_RGBA,
                        GL_UNSIGNED_BYTE, cursorImage.constBits());
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cursorImage.width(),
                     cursorImage.height(), 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, cursorImage.constBits());
    }

    //EGLImage needs to contain sprite before calling this:
    gbm_bo *bufferObject = gbm_bo_create_from_egl_image(m_graphicsBufferManager,
                                                        m_screen->device()->eglDisplay(),
                                                        m_eglImage, 64, 64,
                                                        GBM_BO_USE_CURSOR_64X64);
    quint32 handle = gbm_bo_get_handle(bufferObject).u32;

    gbm_bo_destroy(bufferObject);

    int status = drmModeSetCursor(m_screen->device()->fd(),
                                  m_screen->crtcId(), handle, 64, 64);

    if (status) {
        qWarning("failed to set cursor: %d", status);
    }
}

QT_END_NAMESPACE
