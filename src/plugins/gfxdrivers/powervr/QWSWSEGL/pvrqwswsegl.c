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

#include <GLES/eglplatform.h>
#include <wsegl.h>
#include <pvr2d.h>
#include <string.h>
#include <sys/mman.h>
#include "pvrqwsdrawable_p.h"

#define WSEGL_UNUSED(x) (void)x;

// If the PVR2D version is not specified, then assume MBX-style headers.
// If the version is defined, then we assume that we have SGX-style headers.
#if !defined(PVR2D_REV_MAJOR)
#define WSEGL_CAP_WINDOWS_USE_HW_SYNC WSEGL_CAP_WINDOWS_USE_MBX_SYNC
#define WSEGL_CAP_PIXMAPS_USE_HW_SYNC WSEGL_CAP_PIXMAPS_USE_MBX_SYNC
#endif

/* Capability information for the display */
static WSEGLCaps const wseglDisplayCaps[] = {
    {WSEGL_CAP_WINDOWS_USE_HW_SYNC, 1},
    {WSEGL_CAP_PIXMAPS_USE_HW_SYNC, 1},
    {WSEGL_NO_CAPS, 0}
};

/* Configuration information for the display */
static WSEGLConfig wseglDisplayConfigs[] = {
    {WSEGL_DRAWABLE_WINDOW, WSEGL_PIXELFORMAT_565, WSEGL_FALSE,
     0, 0, 0, WSEGL_OPAQUE, 0},
    {WSEGL_DRAWABLE_PIXMAP, WSEGL_PIXELFORMAT_565, WSEGL_FALSE,
     0, 0, 0, WSEGL_OPAQUE, 0},
    {WSEGL_NO_DRAWABLE, 0, 0, 0, 0, 0, 0, 0}
};

/* Determine if nativeDisplay is a valid display handle */
static WSEGLError wseglIsDisplayValid(NativeDisplayType nativeDisplay)
{
    /* We only have the default display in this system */
    if (nativeDisplay == WSEGL_DEFAULT_DISPLAY)
        return WSEGL_SUCCESS;
    else
        return WSEGL_BAD_NATIVE_DISPLAY;
}

/* Initialize a native display for use with WSEGL */
static WSEGLError wseglInitializeDisplay
    (NativeDisplayType nativeDisplay, WSEGLDisplayHandle *display,
     const WSEGLCaps **caps, WSEGLConfig **configs)
{
    WSEGLPixelFormat pixelFormat;

    /* Bail out if the native display is incorrect */
    if (nativeDisplay != WSEGL_DEFAULT_DISPLAY)
        return WSEGL_CANNOT_INITIALISE;

    /* Open the PVR/QWS display, which will initialize the framebuffer */
    if (!pvrQwsDisplayOpen())
        return WSEGL_CANNOT_INITIALISE;

    /* Convert the PVR2D pixel format into a WSEGL pixel format */
    switch (pvrQwsDisplay.screens[0].pixelFormat) {
        case PVR2D_RGB565:
            pixelFormat = WSEGL_PIXELFORMAT_565;
            break;

	case PVR2D_ARGB4444:
            pixelFormat = WSEGL_PIXELFORMAT_4444;
            break;

	case PVR2D_ARGB8888:
            pixelFormat = WSEGL_PIXELFORMAT_8888;
            break;

        default:
            pvrQwsDisplayClose();
            return WSEGL_CANNOT_INITIALISE;
    }
    wseglDisplayConfigs[0].ePixelFormat = pixelFormat;
    wseglDisplayConfigs[1].ePixelFormat = pixelFormat;

    /* The display has been initialized */
    *display = (WSEGLDisplayHandle)&pvrQwsDisplay;
    *caps = wseglDisplayCaps;
    *configs = wseglDisplayConfigs;
    return WSEGL_SUCCESS;
}

/* Close the WSEGL display */
static WSEGLError wseglCloseDisplay(WSEGLDisplayHandle display)
{
    if (display == (WSEGLDisplayHandle)&pvrQwsDisplay)
        pvrQwsDisplayClose();
    return WSEGL_SUCCESS;
}

static WSEGLRotationAngle wseglRotationValue(int degrees)
{
    switch (degrees) {
    case 90:  return WSEGL_ROTATE_90;
    case 180: return WSEGL_ROTATE_180;
    case 270: return WSEGL_ROTATE_270;
    default:  return WSEGL_ROTATE_0;
    }
}

/* Create the WSEGL drawable version of a native window */
static WSEGLError wseglCreateWindowDrawable
    (WSEGLDisplayHandle display, WSEGLConfig *config,
     WSEGLDrawableHandle *drawable, NativeWindowType nativeWindow,
     WSEGLRotationAngle *rotationAngle)
{
    PvrQwsDrawable *draw;

    WSEGL_UNUSED(display);
    WSEGL_UNUSED(config);

    /* Check for special handles that indicate framebuffer screens */
    if (nativeWindow >= (NativeWindowType)0 &&
            nativeWindow < (NativeWindowType)PVRQWS_MAX_SCREENS) {
        PvrQwsDrawable *screen = pvrQwsScreenWindow((int)nativeWindow);
        if (!screen)
            return WSEGL_OUT_OF_MEMORY;
        *drawable = (WSEGLDrawableHandle)screen;
        if (!pvrQwsAllocBuffers(screen))
            return WSEGL_OUT_OF_MEMORY;
        *rotationAngle = wseglRotationValue(screen->rotationAngle);
        return WSEGL_SUCCESS;
    }

    /* The native window is the winId - fetch the underlying drawable */
    draw = pvrQwsFetchWindow((long)nativeWindow);
    if (!draw)
        return WSEGL_BAD_DRAWABLE;

    /* The drawable is ready to go */
    *drawable = (WSEGLDrawableHandle)draw;
    *rotationAngle = wseglRotationValue(draw->rotationAngle);
    if (!pvrQwsAllocBuffers(draw))
        return WSEGL_OUT_OF_MEMORY;
    return WSEGL_SUCCESS;
}

/* Create the WSEGL drawable version of a native pixmap */
static WSEGLError wseglCreatePixmapDrawable
    (WSEGLDisplayHandle display, WSEGLConfig *config,
     WSEGLDrawableHandle *drawable, NativePixmapType nativePixmap,
     WSEGLRotationAngle *rotationAngle)
{
    WSEGL_UNUSED(display);
    WSEGL_UNUSED(config);
    if (!nativePixmap)
        return WSEGL_BAD_NATIVE_PIXMAP;
    if (!pvrQwsAllocBuffers((PvrQwsDrawable *)nativePixmap))
        return WSEGL_OUT_OF_MEMORY;
    *drawable = (WSEGLDrawableHandle)nativePixmap;
    *rotationAngle = WSEGL_ROTATE_0;
    return WSEGL_SUCCESS;
}

/* Delete a specific drawable */
static WSEGLError wseglDeleteDrawable(WSEGLDrawableHandle _drawable)
{
    PvrQwsDrawable *drawable = (PvrQwsDrawable *)_drawable;
    if (!drawable || drawable->type == PvrQwsScreen)
        return WSEGL_SUCCESS;
    if (pvrQwsDisplay.numFlipBuffers == 0)
        pvrQwsFreeBuffers(drawable);
    if (pvrQwsReleaseWindow(drawable))
        pvrQwsDestroyDrawable(drawable);
    return WSEGL_SUCCESS;
}

/* Swap the contents of a drawable to the screen */
static WSEGLError wseglSwapDrawable
    (WSEGLDrawableHandle _drawable, unsigned long data)
{
    WSEGL_UNUSED(data);
    PvrQwsDrawable *drawable = (PvrQwsDrawable *)_drawable;
    if (drawable->type != PvrQwsPixmap && !pvrQwsSwapBuffers(drawable, 0))
        return WSEGL_BAD_DRAWABLE;
    else
        return WSEGL_SUCCESS;
}

/* Set the swap interval of a window drawable */
static WSEGLError wseglSwapControlInterval
    (WSEGLDrawableHandle drawable, unsigned long interval)
{
    WSEGL_UNUSED(drawable);
    if (pvrQwsDisplay.flipChain) {
        PVR2DSetPresentFlipProperties
            (pvrQwsDisplay.context, pvrQwsDisplay.flipChain,
             PVR2D_PRESENT_PROPERTY_INTERVAL, 0, 0, 0, NULL, interval);
    }
    return WSEGL_SUCCESS;
}

/* Flush native rendering requests on a drawable */
static WSEGLError wseglWaitNative
    (WSEGLDrawableHandle drawable, unsigned long engine)
{
    WSEGL_UNUSED(drawable);
    if (engine == WSEGL_DEFAULT_NATIVE_ENGINE)
        return WSEGL_SUCCESS;
    else
        return WSEGL_BAD_NATIVE_ENGINE;
}

/* Copy color data from a drawable to a native pixmap */
static WSEGLError wseglCopyFromDrawable
    (WSEGLDrawableHandle _drawable, NativePixmapType nativePixmap)
{
    PvrQwsDrawable *drawable = (PvrQwsDrawable *)_drawable;
    PvrQwsDrawable *pixmap = (PvrQwsDrawable *)nativePixmap;
    PVR2DBLTINFO blit;

    if (!drawable || !drawable->backBuffersValid)
        return WSEGL_BAD_NATIVE_WINDOW;
    if (!pixmap || !pixmap->backBuffersValid)
        return WSEGL_BAD_NATIVE_PIXMAP;

    memset(&blit, 0, sizeof(blit));

    blit.CopyCode = PVR2DROPcopy;
    blit.BlitFlags = PVR2D_BLIT_DISABLE_ALL;

    blit.pSrcMemInfo = drawable->backBuffers[drawable->currentBackBuffer];
    blit.SrcStride = drawable->strideBytes;
    blit.SrcX = 0;
    blit.SrcY = 0;
    blit.SizeX = drawable->rect.width;
    blit.SizeY = drawable->rect.height;
    blit.SrcFormat = drawable->pixelFormat;

    blit.pDstMemInfo = pixmap->backBuffers[pixmap->currentBackBuffer];
    blit.DstStride = pixmap->strideBytes;
    blit.DstX = 0;
    blit.DstY = 0;
    blit.DSizeX = pixmap->rect.width;
    blit.DSizeY = pixmap->rect.height;
    blit.DstFormat = pixmap->pixelFormat;

    PVR2DBlt(pvrQwsDisplay.context, &blit);
    PVR2DQueryBlitsComplete
        (pvrQwsDisplay.context, pixmap->backBuffers[pixmap->currentBackBuffer], 1);

    return WSEGL_SUCCESS;
}

/* Copy color data from a PBuffer to a native pixmap */
static WSEGLError wseglCopyFromPBuffer
    (void *address, unsigned long width, unsigned long height,
     unsigned long stride, WSEGLPixelFormat format,
     NativePixmapType nativePixmap)
{
    PvrQwsDrawable *pixmap = (PvrQwsDrawable *)nativePixmap;
    PVR2DFORMAT pixelFormat;

    if (!pixmap)
        return WSEGL_BAD_NATIVE_PIXMAP;

    /* We can only copy under certain conditions */
    switch (format) {
	case WSEGL_PIXELFORMAT_565:
            pixelFormat = PVR2D_RGB565; break;
	case WSEGL_PIXELFORMAT_4444:
            pixelFormat = PVR2D_ARGB4444; break;
	case WSEGL_PIXELFORMAT_8888:
            pixelFormat = PVR2D_ARGB8888; break;
        default:
            return WSEGL_BAD_CONFIG;
    }
    if (width > (unsigned long)(pixmap->rect.width) ||
        height > (unsigned long)(pixmap->rect.height) ||
        pixelFormat != pixmap->pixelFormat) {
        return WSEGL_BAD_CONFIG;
    }

    /* We'd like to use PVR2DBlt to do this, but there is no easy way
       to map the virtual "address" into physical space to be able
       to use the hardware assist.  Use memcpy to do the work instead.
       Note: PBuffer's are upside down, so we copy from the bottom up */
    char *srcaddr = (char *)address;
    char *dstaddr = (char *)(pixmap->backBuffers[pixmap->currentBackBuffer]->pBase);
    int dststride = pixmap->strideBytes;
    int srcwidth = ((int)width) * pvrQwsDisplay.screens[0].bytesPerPixel;
    srcaddr += height * stride;
    while (height > 0) {
        srcaddr -= (int)stride;
        memcpy(dstaddr, srcaddr, srcwidth);
        dstaddr += dststride;
        --height;
    }
    return WSEGL_SUCCESS;
}

/* Return the parameters of a drawable that are needed by the EGL layer */
static WSEGLError wseglGetDrawableParameters
    (WSEGLDrawableHandle _drawable, WSEGLDrawableParams *sourceParams,
     WSEGLDrawableParams *renderParams)
{
    PvrQwsDrawable *drawable = (PvrQwsDrawable *)_drawable;
    PVR2DMEMINFO *source, *render;
    WSEGLPixelFormat pixelFormat;

    if (!pvrQwsGetBuffers(drawable, &source, &render))
        return WSEGL_BAD_DRAWABLE;

    switch (drawable->pixelFormat) {
        case PVR2D_RGB565:
        default:
            pixelFormat = WSEGL_PIXELFORMAT_565;
            break;

	case PVR2D_ARGB4444:
            pixelFormat = WSEGL_PIXELFORMAT_4444;
            break;

	case PVR2D_ARGB8888:
            pixelFormat = WSEGL_PIXELFORMAT_8888;
            break;
    }

    sourceParams->ui32Width = drawable->rect.width;
    sourceParams->ui32Height = drawable->rect.height;
    sourceParams->ui32Stride = drawable->stridePixels;
    sourceParams->ePixelFormat = pixelFormat;
    sourceParams->pvLinearAddress = source->pBase;
    sourceParams->ui32HWAddress = source->ui32DevAddr;
    sourceParams->hPrivateData = source->hPrivateData;

    renderParams->ui32Width = drawable->rect.width;
    renderParams->ui32Height = drawable->rect.height;
    renderParams->ui32Stride = drawable->stridePixels;
    renderParams->ePixelFormat = pixelFormat;
    renderParams->pvLinearAddress = render->pBase;
    renderParams->ui32HWAddress = render->ui32DevAddr;
    renderParams->hPrivateData = render->hPrivateData;

    return WSEGL_SUCCESS;
}

static WSEGL_FunctionTable const wseglFunctions = {
    WSEGL_VERSION,
    wseglIsDisplayValid,
    wseglInitializeDisplay,
    wseglCloseDisplay,
    wseglCreateWindowDrawable,
    wseglCreatePixmapDrawable,
    wseglDeleteDrawable,
    wseglSwapDrawable,
    wseglSwapControlInterval,
    wseglWaitNative,
    wseglCopyFromDrawable,
    wseglCopyFromPBuffer,
    wseglGetDrawableParameters
};

/* Return the table of WSEGL functions to the EGL implementation */
const WSEGL_FunctionTable *WSEGL_GetFunctionTablePointer(void)
{
    return &wseglFunctions;
}
