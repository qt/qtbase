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
#include "qkmscontext.h"
#include "qkmsbuffermanager.h"

QT_BEGIN_NAMESPACE

//Fallback mode (taken from Wayland DRM demo compositor)
static drmModeModeInfo builtin_1024x768 = {
    63500, //clock
    1024, 1072, 1176, 1328, 0,
    768, 771, 775, 798, 0,
    59920,
    DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_PVSYNC,
    0,
    "1024x768"
};

QKmsScreen::QKmsScreen(QKmsDevice *device, int connectorId)
    : m_device(device),
      m_flipReady(true),
      m_connectorId(connectorId),
      m_depth(32),
      m_format(QImage::Format_Invalid),
      m_bufferManager(this),
      m_refreshTime(16000)
{
    m_cursor = new QKmsCursor(this);
    initializeScreenMode();
}

QKmsScreen::~QKmsScreen()
{
    delete m_cursor;
}

QRect QKmsScreen::geometry() const
{
    return m_geometry;
}

int QKmsScreen::depth() const
{
    return m_depth;
}

QImage::Format QKmsScreen::format() const
{
    return m_format;
}

QSizeF QKmsScreen::physicalSize() const
{
    return m_physicalSize;
}

QPlatformCursor *QKmsScreen::cursor() const
{
    return m_cursor;
}

GLuint QKmsScreen::framebufferObject() const
{
    return m_bufferManager.framebufferObject();
}

void QKmsScreen::initializeScreenMode()
{
    //Determine optimal mode for screen
    drmModeRes *resources = drmModeGetResources(m_device->fd());
    if (!resources)
        qFatal("drmModeGetResources failed");

    drmModeConnector *connector = drmModeGetConnector(m_device->fd(), m_connectorId);
    drmModeModeInfo *mode = 0;
    if (connector->count_modes > 0)
        mode = &connector->modes[0];
    else
        mode = &builtin_1024x768;

    drmModeEncoder *encoder = drmModeGetEncoder(m_device->fd(), connector->encoders[0]);
    if (encoder == 0)
        qFatal("No encoder for connector.");

    int i;
    for (i = 0; i < resources->count_crtcs; i++) {
        if (encoder->possible_crtcs & (1 << i))
            break;
    }
    if (i == resources->count_crtcs)
        qFatal("No usable crtc for encoder.");

    m_crtcId = resources->crtcs[i];
    m_mode = *mode;
    m_geometry = QRect(0, 0, m_mode.hdisplay, m_mode.vdisplay);
    m_depth = 32;
    m_format = QImage::Format_RGB32;
    m_physicalSize = QSizeF(connector->mmWidth, connector->mmHeight);

    //Setup three buffers for current mode
    m_bufferManager.setupBuffersForMode(m_mode, 3);

    //Set the Mode of the screen.
    int ret = drmModeSetCrtc(m_device->fd(), m_crtcId, m_bufferManager.displayFramebufferId(),
                             0, 0, &m_connectorId, 1, &m_mode);
    if (ret)
        qFatal("failed to set mode");

    //Cleanup
    drmModeFreeEncoder(encoder);
    drmModeFreeConnector(connector);
    drmModeFreeResources(resources);
}

void QKmsScreen::bindFramebuffer()
{
    if (m_bufferManager.framebufferObject()) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_bufferManager.framebufferObject());

        glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                  GL_COLOR_ATTACHMENT0,
                                  GL_RENDERBUFFER,
                                  m_bufferManager.renderTargetBuffer());
    }
}

void QKmsScreen::swapBuffers()
{
    waitForPageFlipComplete();

    if ( m_flipReady )
        performPageFlip();
    //TODO: Do something with return value here
    m_bufferManager.nextBuffer();
    //qDebug() << "swapBuffers now rendering to " << m_bufferManager.renderTargetBuffer();
    bindFramebuffer();
}

void QKmsScreen::performPageFlip()
{
    quint32 displayFramebufferId = m_bufferManager.displayFramebufferId();
    //qDebug() << "Flipping to framebuffer: " << displayFramebufferId;

    int pageFlipStatus = drmModePageFlip(m_device->fd(), m_crtcId,
                                         displayFramebufferId,
                                         DRM_MODE_PAGE_FLIP_EVENT, this);
    if (pageFlipStatus)
        qWarning("Pageflip status: %d", pageFlipStatus);

    m_flipReady = false;
}

void QKmsScreen::setFlipReady(unsigned int time)
{
    m_flipReady = true;
    m_refreshTime = time;
    performPageFlip();
}

QKmsDevice * QKmsScreen::device() const
{
    return m_device;
}

void QKmsScreen::waitForPageFlipComplete()
{
    //Check manually if there is something to be read on the device
    //as there are senarios where the signal is not received (starvation)
    fd_set fdSet;
    timeval timeValue;
    int returnValue;

    FD_ZERO(&fdSet);
    FD_SET(m_device->fd(), &fdSet);
    timeValue.tv_sec = 0;
    timeValue.tv_usec = m_refreshTime;

    returnValue = select(1, &fdSet, 0, 0, &timeValue);

    if (returnValue) {
        m_device->handlePageFlipCompleted();
    }

}


QT_END_NAMESPACE
