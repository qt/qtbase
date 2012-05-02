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
#include "qkmsscreen.h"
#include "qkmsdevice.h"

#include "qkmsintegration.h"

#include <QtCore/QSocketNotifier>
#include <QtCore/private/qcore_unix_p.h>

QT_BEGIN_NAMESPACE

QKmsDevice::QKmsDevice(const QString &path, QKmsIntegration *parent) :
    QObject(0), m_integration(parent)
{
    m_fd = QT_OPEN(path.toLatin1().constData(), O_RDWR);
    if (m_fd < 0) {
        qWarning("Could not open %s.", path.toLatin1().constData());
        qFatal("No DRM display device");
    }

    m_graphicsBufferManager = gbm_create_device(m_fd);
    m_eglDisplay = eglGetDisplay(m_graphicsBufferManager);

    if (m_eglDisplay == EGL_NO_DISPLAY) {
        qWarning("Could not open EGL display");
        qFatal("EGL error");
    }

    EGLint major;
    EGLint minor;
    if (!eglInitialize(m_eglDisplay, &major, &minor)) {
        qWarning("Could not initialize EGL display");
        qFatal("EGL error");
    }

    QString extensions = eglQueryString(m_eglDisplay, EGL_EXTENSIONS);
    if (!extensions.contains(QString::fromLatin1("EGL_KHR_surfaceless_opengl"))) {
        qFatal("EGL_KHR_surfaceless_opengl extension not available");
    }

    //Initialize EGLContext
    static EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    eglBindAPI(EGL_OPENGL_ES_API);
    m_eglContext = eglCreateContext(m_eglDisplay, 0, 0, contextAttribs);
    if (m_eglContext == EGL_NO_CONTEXT) {
        qWarning("Could not create the EGL context.");
        eglTerminate(m_eglDisplay);
        qFatal("EGL error");
    }

    createScreens();

    QSocketNotifier *notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
    connect(notifier, SIGNAL(activated(int)), this, SLOT(handlePageFlipCompleted()));
}

QKmsDevice::~QKmsDevice()
{
    if (m_eglContext != EGL_NO_CONTEXT) {
        eglDestroyContext(m_eglDisplay, m_eglContext);
        m_eglContext = EGL_NO_CONTEXT;
    }
}

void QKmsDevice::createScreens()
{
    drmModeRes *resources = 0;
    resources = drmModeGetResources(m_fd);
    if (!resources)
        qFatal("drmModeGetResources failed");

    //Iterate connectors and create screens on each one active
    for (int i = 0; i < resources->count_connectors; i++) {
        drmModeConnector *connector = 0;
        connector = drmModeGetConnector(m_fd, resources->connectors[i]);
        if (connector && connector->connection == DRM_MODE_CONNECTED) {
            m_integration->addScreen(new QKmsScreen(this, connector->connector_id));
        }
        drmModeFreeConnector(connector);
    }
    drmModeFreeResources(resources);
}

void QKmsDevice::handlePageFlipCompleted()
{
    //qDebug() << "Display signal received";
    drmEventContext eventContext;

    memset(&eventContext, 0, sizeof eventContext);
    eventContext.version = DRM_EVENT_CONTEXT_VERSION;
    eventContext.page_flip_handler = QKmsDevice::pageFlipHandler;
    drmHandleEvent(m_fd, &eventContext);

}

void QKmsDevice::pageFlipHandler(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data)
{
    Q_UNUSED(fd)
    Q_UNUSED(frame)
    Q_UNUSED(sec)
    Q_UNUSED(usec)
    static unsigned int previousTime = 0;

    unsigned int currentTime = sec * 1000000 + usec;
    unsigned int refreshTime = 0;
//    qDebug() << "fd: " << fd << " frame: " << frame << " sec: "
//             << sec << " usec: " << usec << " data: " << data
//             << "msecs" << sec * 1000 + usec / 1000;
    QKmsScreen *screen = static_cast<QKmsScreen *>(data);

    if (previousTime == 0)
        refreshTime = 16000;
    else
        refreshTime = currentTime - previousTime;

    screen->setFlipReady(refreshTime);
}

QT_END_NAMESPACE
