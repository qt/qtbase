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

#ifndef QKMSSCREEN_H
#define QKMSSCREEN_H

#include <QtGui/QPlatformScreen>
#include "qkmsbuffermanager.h"

QT_BEGIN_NAMESPACE

class QKmsCursor;
class QKmsDevice;
class QKmsContext;

class QKmsScreen : public QPlatformScreen
{
public:
    QKmsScreen(QKmsDevice *device, int connectorId);
    ~QKmsScreen();

    QRect geometry() const;
    int depth() const;
    QImage::Format format() const;
    QSizeF physicalSize() const;
    QPlatformCursor *cursor() const;

    GLuint framebufferObject() const;
    quint32 crtcId() const { return m_crtcId; }
    QKmsDevice *device() const;

    //Called by context for each screen
    void bindFramebuffer();
    void swapBuffers();
    void setFlipReady(unsigned int time);

private:
    void performPageFlip();
    void initializeScreenMode();
    void waitForPageFlipComplete();

    QKmsDevice *m_device;
    bool m_flipReady;
    quint32 m_connectorId;

    quint32 m_crtcId;
    drmModeModeInfo m_mode;
    QRect m_geometry;
    QSizeF m_physicalSize;
    int m_depth;
    QImage::Format m_format;

    QKmsCursor *m_cursor;
    QKmsBufferManager m_bufferManager;
    unsigned int m_refreshTime;
};

QT_END_NAMESPACE

#endif // QKMSSCREEN_H
