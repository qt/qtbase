/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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

#ifdef Q_OS_WINCE_WM

#include <Ddraw.h>
#include <QDebug>

static LPDIRECTDRAW                g_pDD = NULL;            // DirectDraw object
static LPDIRECTDRAWSURFACE         g_pDDSSurface = NULL;    // DirectDraw primary surface

static DDSCAPS ddsCaps;
static DDSURFACEDESC ddsSurfaceDesc;
static void *buffer = NULL;

static int width = 0;
static int height = 0;
static int pitch = 0;
static int bitCount = 0;
static int windowId = 0;

static bool initialized = false;
static bool locked = false;

void q_lock()
{
    if (locked) {
        qWarning("Direct Painter already locked (QDirectPainter::lock())");
        return;
    }
    locked = true;


    memset(&ddsSurfaceDesc, 0, sizeof(ddsSurfaceDesc));
    ddsSurfaceDesc.dwSize = sizeof(ddsSurfaceDesc);

    HRESULT h = g_pDDSSurface->Lock(0, &ddsSurfaceDesc, DDLOCK_WRITEONLY, 0);
    if (h != DD_OK)
        qDebug() << "GetSurfaceDesc failed!";

    width = ddsSurfaceDesc.dwWidth;
    height = ddsSurfaceDesc.dwHeight;
    bitCount = ddsSurfaceDesc.ddpfPixelFormat.dwRGBBitCount;
    pitch = ddsSurfaceDesc.lPitch;
    buffer = ddsSurfaceDesc.lpSurface;
}

void q_unlock()
{
    if( !locked) {
        qWarning("Direct Painter not locked (QDirectPainter::unlock()");
        return;
    }
    g_pDDSSurface->Unlock(0);
    locked = false;
}

void q_initDD()
{
    if (initialized)
        return;

    DirectDrawCreate(NULL, &g_pDD, NULL);

    HRESULT h;
    h = g_pDD->SetCooperativeLevel(0, DDSCL_NORMAL);

    if (h != DD_OK)
        qDebug() << "cooperation level failed";

    h = g_pDD->TestCooperativeLevel();
    if (h != DD_OK)
        qDebug() << "cooperation level failed test";

    DDSURFACEDESC ddsd;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);

    ddsd.dwFlags = DDSD_CAPS;

    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    h = g_pDD->CreateSurface(&ddsd, &g_pDDSSurface, NULL);

    if (h != DD_OK)
        qDebug() << "CreateSurface failed!";

    if (g_pDDSSurface->GetCaps(&ddsCaps) != DD_OK)
        qDebug() << "GetCaps failed";

    q_lock();
    q_unlock();
    initialized = true;
}

uchar* q_frameBuffer()
{
    return (uchar*) buffer;
}

int q_screenDepth()
{
    return bitCount;
}

int q_screenWidth()
{
    return width;
}

int q_screenHeight()
{
    return height;
}

int q_linestep()
{
    return pitch;
}

#endif //Q_OS_WINCE_WM


