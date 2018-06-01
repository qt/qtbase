/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QDebug>
#include <QtEglSupport/private/qeglconvenience_p.h>
#include <EGL/egl.h>
#include "INTEGRITY.h"
#include "qeglfsrcarintegration.h"

#define RCAR_DEFAULT_DISPLAY 1
#define RCAR_DEFAULT_WM_LAYER 2

extern "C" unsigned long PVRGrfxServerInit(void);

QT_BEGIN_NAMESPACE

void QEglFSRcarIntegration::platformInit()
{
    bool ok;

    QEglFSDeviceIntegration::platformInit();

    PVRGrfxServerInit();

    mScreenSize = q_screenSizeFromFb(0);
    mNativeDisplay = (NativeDisplayType)EGL_DEFAULT_DISPLAY;

    mNativeDisplayID = qEnvironmentVariableIntValue("QT_QPA_WM_DISP_ID", &ok);
    if (!ok)
        mNativeDisplayID = RCAR_DEFAULT_DISPLAY;

    r_wm_Error_t wm_err = R_WM_DevInit(mNativeDisplayID);
    if (wm_err != R_WM_ERR_OK)
        qFatal("Failed to init WM Dev: %d, error: %d", mNativeDisplayID, wm_err);
    wm_err = R_WM_ScreenBgColorSet(mNativeDisplayID, 0x20, 0x20, 0x20); // Grey
    if (wm_err != R_WM_ERR_OK)
        qFatal("Failed to set screen background: %d", wm_err);
    wm_err = R_WM_ScreenEnable(mNativeDisplayID);
    if (wm_err != R_WM_ERR_OK)
        qFatal("Failed to enable screen: %d", wm_err);
}

QSize QEglFSRcarIntegration::screenSize() const
{
    return mScreenSize;
}

EGLNativeDisplayType QEglFSRcarIntegration::platformDisplay() const
{
    return mNativeDisplay;
}

static r_wm_WinColorFmt_t getWMColorFormat(const QSurfaceFormat &format)
{
    const int a = format.alphaBufferSize();
    const int r = format.redBufferSize();
    const int g = format.greenBufferSize();
    const int b = format.blueBufferSize();

    switch (r) {
    case 4:
        if (g == 4 && b == 4 && a == 4)
            return R_WM_COLORFMT_ARGB4444;
        break;
    case 5:
        if (g == 6 && b == 5 && a == 0)
            return R_WM_COLORFMT_RGB565;
        else if (g == 5 && b == 5 && a == 1)
            return R_WM_COLORFMT_ARGB1555;
        break;
    case 8:
        if (g == 8 && b == 8 && a == 0)
            return R_WM_COLORFMT_RGB0888;
        else if (g == 8 && b == 8 && a == 8)
            return R_WM_COLORFMT_ARGB8888;
        break;
    }

    qFatal("Unsupported color format: R:%d G:%d B:%d A:%d", r, g, b, a);
    return R_WM_COLORFMT_LAST;
}

EGLNativeWindowType QEglFSRcarIntegration::createNativeWindow(QPlatformWindow *window, const QSize &size, const QSurfaceFormat &format)
{
    bool ok;

    mNativeWindow = (EGLNativeWindowTypeREL*)malloc(sizeof(EGLNativeWindowTypeREL));
    memset(mNativeWindow, 0, sizeof(EGLNativeWindowTypeREL));

    mNativeWindow->ColorFmt = getWMColorFormat(format);
    mNativeWindow->PosX = 0;
    mNativeWindow->PosY = 0;
    mNativeWindow->PosZ = qEnvironmentVariableIntValue("QT_QPA_WM_LAYER", &ok);
    if (!ok)
        mNativeWindow->PosZ = RCAR_DEFAULT_WM_LAYER;
    mNativeWindow->Pitch = size.width();
    mNativeWindow->Width = size.width();
    mNativeWindow->Height = size.height();
    mNativeWindow->Alpha = format.alphaBufferSize();

    if (format.swapBehavior() == QSurfaceFormat::DefaultSwapBehavior)
        mNativeWindow->Surface.BufNum = 3;
    else
        mNativeWindow->Surface.BufNum = format.swapBehavior();

    mNativeWindow->Surface.Type = R_WM_SURFACE_FB;
    mNativeWindow->Surface.BufMode = R_WM_WINBUF_ALLOC_INTERNAL;

    r_wm_Error_t wm_err = R_WM_WindowCreate(mNativeDisplayID, mNativeWindow);
    if (wm_err != R_WM_ERR_OK)
        qFatal("Failed to create window layer: %d", wm_err);
    wm_err = R_WM_DevEventRegister(mNativeDisplayID, R_WM_EVENT_VBLANK, 0);
    if (wm_err != R_WM_ERR_OK)
        qFatal("Failed to Register vsync event: %d", wm_err);
    wm_err = R_WM_WindowEnable(mNativeDisplayID, mNativeWindow);
    if (wm_err != R_WM_ERR_OK)
        qFatal("Failed to Enable window surface: %d", wm_err);

    return static_cast<EGLNativeWindowType>(mNativeWindow);
}

void QEglFSRcarIntegration::destroyNativeWindow(EGLNativeWindowType window)
{
    R_WM_WindowDisable(mNativeDisplayID, mNativeWindow);
    usleep(100000); //Needed to allow Window Manager make the window transparent
    R_WM_WindowDelete(mNativeDisplayID, mNativeWindow);
    R_WM_DevDeinit(mNativeDisplayID);
    free(mNativeWindow);
}

QT_END_NAMESPACE
