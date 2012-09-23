/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

using namespace android;

QT_BEGIN_NAMESPACE

class QEglFSPandaHooks : public QEglFSHooks
{
public:
    virtual EGLNativeWindowType createNativeWindow(const QSize &size, const QSurfaceFormat &format);
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
    mControl->setAlpha(0.4);
    SurfaceComposerClient::closeGlobalTransaction();
    mAndroidSurface = mControl->getSurface();

    EGLNativeWindowType eglWindow = mAndroidSurface.get();
    return eglWindow;
}

static QEglFSPandaHooks eglFSPandaHooks;
QEglFSHooks *platformHooks = &eglFSPandaHooks;

QT_END_NAMESPACE
