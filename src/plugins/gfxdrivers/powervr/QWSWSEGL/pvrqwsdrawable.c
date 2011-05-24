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

#include "pvrqwsdrawable_p.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <unistd.h>

PvrQwsDisplay pvrQwsDisplay;

static void pvrQwsDestroyDrawableForced(PvrQwsDrawable *drawable);

/* Initialize the /dev/fbN device for a specific screen */
static int pvrQwsInitFbScreen(int screen)
{
    struct fb_var_screeninfo var;
    struct fb_fix_screeninfo fix;
    unsigned long start;
    unsigned long length;
    int width, height, stride;
    PVR2DFORMAT format;
    void *mapped;
    int fd, bytesPerPixel;
    char name[64];
    PVR2DMEMINFO *memInfo;
    unsigned long pageAddresses[2];

    /* Bail out if already initialized, or the number is incorrect */
    if (screen < 0 || screen >= PVRQWS_MAX_SCREENS)
        return 0;
    if (pvrQwsDisplay.screens[screen].initialized)
        return 1;

    /* Open the framebuffer and fetch its properties */
    sprintf(name, "/dev/fb%d", screen);
    fd = open(name, O_RDWR, 0);
    if (fd < 0) {
        perror(name);
        return 0;
    }
    if (ioctl(fd, FBIOGET_VSCREENINFO, &var) < 0) {
        perror("FBIOGET_VSCREENINFO");
        close(fd);
        return 0;
    }
    if (ioctl(fd, FBIOGET_FSCREENINFO, &fix) < 0) {
        perror("FBIOGET_FSCREENINFO");
        close(fd);
        return 0;
    }
    width = var.xres;
    height = var.yres;
    bytesPerPixel = var.bits_per_pixel / 8;
    stride = fix.line_length;
    format = PVR2D_1BPP;
    if (var.bits_per_pixel == 16) {
        if (var.red.length == 5 && var.green.length == 6 &&
            var.blue.length == 5 && var.red.offset == 11 &&
            var.green.offset == 5 && var.blue.offset == 0) {
            format = PVR2D_RGB565;
        }
        if (var.red.length == 4 && var.green.length == 4 &&
            var.blue.length == 4 && var.transp.length == 4 &&
            var.red.offset == 8 && var.green.offset == 4 &&
            var.blue.offset == 0 && var.transp.offset == 12) {
            format = PVR2D_ARGB4444;
        }
    } else if (var.bits_per_pixel == 32) {
        if (var.red.length == 8 && var.green.length == 8 &&
            var.blue.length == 8 && var.transp.length == 8 &&
            var.red.offset == 16 && var.green.offset == 8 &&
            var.blue.offset == 0 && var.transp.offset == 24) {
            format = PVR2D_ARGB8888;
        }
    }
    if (format == PVR2D_1BPP) {
        fprintf(stderr, "%s: could not find a suitable PVR2D pixel format\n", name);
        close(fd);
        return 0;
    }
    start = fix.smem_start;
    length = var.xres_virtual * var.yres_virtual * bytesPerPixel;

    if (screen == 0) {
        /* We use PVR2DGetFrameBuffer to map the first screen.
           On some chipsets it is more reliable than using PVR2DMemWrap */
        mapped = 0;
        memInfo = 0;
    } else {
        /* Other screens: map the framebuffer region into memory */
        mapped = mmap(0, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (!mapped || mapped == (void *)(-1)) {
            perror("mmap");
            close(fd);
            return 0;
        }

        /* Allocate a PVR2D memory region for the framebuffer */
        memInfo = 0;
        if (pvrQwsDisplay.context) {
            pageAddresses[0] = start & 0xFFFFF000;
            pageAddresses[1] = 0;
            if (PVR2DMemWrap
                    (pvrQwsDisplay.context, mapped, PVR2D_WRAPFLAG_CONTIGUOUS,
                     length, pageAddresses, &memInfo) != PVR2D_OK) {
                munmap(mapped, length);
                close(fd);
                return 0;
            }
        }
    }

    /* We don't need the file descriptor any more */
    close(fd);

    /* The framebuffer is ready, so initialize the PvrQwsScreenInfo */
    pvrQwsDisplay.screens[screen].screenRect.x = 0;
    pvrQwsDisplay.screens[screen].screenRect.y = 0;
    pvrQwsDisplay.screens[screen].screenRect.width = width;
    pvrQwsDisplay.screens[screen].screenRect.height = height;
    pvrQwsDisplay.screens[screen].screenStride = stride;
    pvrQwsDisplay.screens[screen].pixelFormat = format;
    pvrQwsDisplay.screens[screen].bytesPerPixel = bytesPerPixel;
    pvrQwsDisplay.screens[screen].screenDrawable = 0;
    if (mapped) {
        /* Don't set these fields if mapped is 0, because PVR2DGetFrameBuffer
           may have already been called and set them */
        pvrQwsDisplay.screens[screen].frameBuffer = memInfo;
        pvrQwsDisplay.screens[screen].mapped = mapped;
    }
    pvrQwsDisplay.screens[screen].mappedLength = length;
    pvrQwsDisplay.screens[screen].screenStart = start;
    pvrQwsDisplay.screens[screen].needsUnmap = (mapped != 0);
    pvrQwsDisplay.screens[screen].initialized = 1;
    return 1;
}

/* Called when a new drawable is added to ensure that we have a
   PVR2D context and framebuffer PVR2DMEMINFO blocks */
static int pvrQwsAddDrawable(void)
{
    int numDevs, screen;
    PVR2DDEVICEINFO *devs;
    unsigned long devId;
    unsigned long pageAddresses[2];
    PVR2DMEMINFO *memInfo;
    PVR2DDISPLAYINFO displayInfo;

    /* Bail out early if this is not the first drawable */
    if (pvrQwsDisplay.numDrawables > 0) {
        ++(pvrQwsDisplay.numDrawables);
        return 1;
    }

    /* Find the first PVR2D device in the system and open it */
    numDevs = PVR2DEnumerateDevices(0);
    if (numDevs <= 0)
        return 0;
    devs = (PVR2DDEVICEINFO *)malloc(sizeof(PVR2DDEVICEINFO) * numDevs);
    if (!devs)
        return 0;
    if (PVR2DEnumerateDevices(devs) != PVR2D_OK) {
        free(devs);
        return 0;
    }
    devId = devs[0].ulDevID;
    free(devs);
    if (PVR2DCreateDeviceContext(devId, &pvrQwsDisplay.context, 0) != PVR2D_OK)
        return 0;
    pvrQwsDisplay.numFlipBuffers = 0;
    pvrQwsDisplay.flipChain = 0;
    if (PVR2DGetDeviceInfo(pvrQwsDisplay.context, &displayInfo) == PVR2D_OK) {
        if (displayInfo.ulMaxFlipChains > 0 && displayInfo.ulMaxBuffersInChain > 0)
            pvrQwsDisplay.numFlipBuffers = displayInfo.ulMaxBuffersInChain;
        if (pvrQwsDisplay.numFlipBuffers > PVRQWS_MAX_FLIP_BUFFERS)
            pvrQwsDisplay.numFlipBuffers = PVRQWS_MAX_FLIP_BUFFERS;
    }

    /* Create the PVR2DMEMINFO blocks for the active framebuffers */
    for (screen = 0; screen < PVRQWS_MAX_SCREENS; ++screen) {
        if (screen != 0 && pvrQwsDisplay.screens[screen].mapped) {
            pageAddresses[0]
                = pvrQwsDisplay.screens[screen].screenStart & 0xFFFFF000;
            pageAddresses[1] = 0;
            if (PVR2DMemWrap
                    (pvrQwsDisplay.context,
                     pvrQwsDisplay.screens[screen].mapped,
                     PVR2D_WRAPFLAG_CONTIGUOUS,
                     pvrQwsDisplay.screens[screen].mappedLength,
                     pageAddresses, &memInfo) != PVR2D_OK) {
                PVR2DDestroyDeviceContext(pvrQwsDisplay.context);
                pvrQwsDisplay.context = 0;
                return 0;
            }
            pvrQwsDisplay.screens[screen].frameBuffer = memInfo;
        } else if (screen == 0) {
            if (PVR2DGetFrameBuffer
                    (pvrQwsDisplay.context,
                     PVR2D_FB_PRIMARY_SURFACE, &memInfo) != PVR2D_OK) {
                fprintf(stderr, "QWSWSEGL: could not get the primary framebuffer surface\n");
                PVR2DDestroyDeviceContext(pvrQwsDisplay.context);
                pvrQwsDisplay.context = 0;
                return 0;
            }
            pvrQwsDisplay.screens[screen].frameBuffer = memInfo;
            pvrQwsDisplay.screens[screen].mapped = memInfo->pBase;
        }
    }

    /* Create a flip chain for the screen if supported by the hardware */
    pvrQwsDisplay.usePresentBlit = 0;
    if (pvrQwsDisplay.numFlipBuffers > 0) {
        long stride = 0;
        unsigned long flipId = 0;
        unsigned long numBuffers;
        if (PVR2DCreateFlipChain(pvrQwsDisplay.context, 0,
                                 //PVR2D_CREATE_FLIPCHAIN_SHARED |
                                 //PVR2D_CREATE_FLIPCHAIN_QUERY,
                                 pvrQwsDisplay.numFlipBuffers,
                                 pvrQwsDisplay.screens[0].screenRect.width,
                                 pvrQwsDisplay.screens[0].screenRect.height,
                                 pvrQwsDisplay.screens[0].pixelFormat,
                                 &stride, &flipId, &(pvrQwsDisplay.flipChain))
                == PVR2D_OK) {
            pvrQwsDisplay.screens[0].screenStride = stride;
            PVR2DGetFlipChainBuffers(pvrQwsDisplay.context,
                                     pvrQwsDisplay.flipChain,
                                     &numBuffers,
                                     pvrQwsDisplay.flipBuffers);
        } else {
            pvrQwsDisplay.flipChain = 0;
            pvrQwsDisplay.numFlipBuffers = 0;
        }

        /* PVR2DPresentBlt is a little more reliable than PVR2DBlt
           when flip chains are present, even if we cannot create a
           flip chain at the moment */
        pvrQwsDisplay.usePresentBlit = 1;
    }

    /* The context is ready to go */
    ++(pvrQwsDisplay.numDrawables);
    return 1;
}

/* Called when the last drawable is destroyed.  The PVR2D context
   will be destroyed but the raw framebuffer memory will stay mapped */
static void pvrQwsDestroyContext(void)
{
    int screen;
    for (screen = 0; screen < PVRQWS_MAX_SCREENS; ++screen) {
        if (pvrQwsDisplay.screens[screen].frameBuffer) {
            PVR2DMemFree
                (pvrQwsDisplay.context, 
                 pvrQwsDisplay.screens[screen].frameBuffer);
            pvrQwsDisplay.screens[screen].frameBuffer = 0;
        }
    }

    if (pvrQwsDisplay.numFlipBuffers > 0)
        PVR2DDestroyFlipChain(pvrQwsDisplay.context, pvrQwsDisplay.flipChain);
    PVR2DDestroyDeviceContext(pvrQwsDisplay.context);
    pvrQwsDisplay.context = 0;
    pvrQwsDisplay.flipChain = 0;
    pvrQwsDisplay.numFlipBuffers = 0;
    pvrQwsDisplay.usePresentBlit = 0;
}

int pvrQwsDisplayOpen(void)
{
    int screen;

    /* If the display is already open, increase reference count and return */
    if (pvrQwsDisplay.refCount > 0) {
        ++(pvrQwsDisplay.refCount);
        return 1;
    }

    /* Open the framebuffer and map it directly */
    if (!pvrQwsInitFbScreen(0)) {
        --(pvrQwsDisplay.refCount);
        return 0;
    }

    /* Clear the other screens.  We will create them if they are referenced */
    for (screen = 1; screen < PVRQWS_MAX_SCREENS; ++screen)
        memset(&(pvrQwsDisplay.screens[screen]), 0, sizeof(PvrQwsScreenInfo));

    /* The display is open and ready */
    ++(pvrQwsDisplay.refCount);
    return 1;
}

void pvrQwsDisplayClose(void)
{
    int screen;

    if (pvrQwsDisplay.refCount == 0)
        return;
    if (--(pvrQwsDisplay.refCount) > 0)
        return;

    /* Prevent pvrQwsDestroyContext from being called for the time being */
    ++pvrQwsDisplay.numDrawables;

    /* Free the screens */
    for (screen = 0; screen < PVRQWS_MAX_SCREENS; ++screen) {
        PvrQwsScreenInfo *info = &(pvrQwsDisplay.screens[screen]);
        if (info->screenDrawable)
            pvrQwsDestroyDrawableForced(info->screenDrawable);
        if (info->frameBuffer)
            PVR2DMemFree(pvrQwsDisplay.context, info->frameBuffer);
        if (info->mapped && info->needsUnmap)
            munmap(info->mapped, info->mappedLength);
    }

    /* Now it is safe to destroy the PVR2D context */
    --pvrQwsDisplay.numDrawables;
    if (pvrQwsDisplay.context)
        PVR2DDestroyDeviceContext(pvrQwsDisplay.context);

    memset(&pvrQwsDisplay, 0, sizeof(pvrQwsDisplay));
}

int pvrQwsDisplayIsOpen(void)
{
    return (pvrQwsDisplay.refCount > 0);
}

/* Ensure that a specific screen has been initialized */
static int pvrQwsEnsureScreen(int screen)
{
    if (screen < 0 || screen >= PVRQWS_MAX_SCREENS)
        return 0;
    if (!screen)
        return 1;
    return pvrQwsInitFbScreen(screen);
}

PvrQwsDrawable *pvrQwsScreenWindow(int screen)
{
    PvrQwsDrawable *drawable;

    if (!pvrQwsEnsureScreen(screen))
        return 0;

    drawable = pvrQwsDisplay.screens[screen].screenDrawable;
    if (drawable)
        return drawable;

    drawable = (PvrQwsDrawable *)calloc(1, sizeof(PvrQwsDrawable));
    if (!drawable)
        return 0;

    drawable->type = PvrQwsScreen;
    drawable->screen = screen;
    drawable->pixelFormat = pvrQwsDisplay.screens[screen].pixelFormat;
    drawable->rect = pvrQwsDisplay.screens[screen].screenRect;
    drawable->visibleRects[0] = drawable->rect;
    drawable->numVisibleRects = 1;
    drawable->isFullScreen = 1;

    if (!pvrQwsAddDrawable()) {
        free(drawable);
        return 0;
    }

    pvrQwsDisplay.screens[screen].screenDrawable = drawable;

    return drawable;
}

PvrQwsDrawable *pvrQwsCreateWindow(int screen, long winId, const PvrQwsRect *rect)
{
    PvrQwsDrawable *drawable;

    if (!pvrQwsEnsureScreen(screen))
        return 0;

    drawable = (PvrQwsDrawable *)calloc(1, sizeof(PvrQwsDrawable));
    if (!drawable)
        return 0;

    drawable->type = PvrQwsWindow;
    drawable->winId = winId;
    drawable->refCount = 1;
    drawable->screen = screen;
    drawable->pixelFormat = pvrQwsDisplay.screens[screen].pixelFormat;
    drawable->rect = *rect;

    if (!pvrQwsAddDrawable()) {
        free(drawable);
        return 0;
    }

    drawable->nextWinId = pvrQwsDisplay.firstWinId;
    pvrQwsDisplay.firstWinId = drawable;

    return drawable;
}

PvrQwsDrawable *pvrQwsFetchWindow(long winId)
{
    PvrQwsDrawable *drawable = pvrQwsDisplay.firstWinId;
    while (drawable != 0 && drawable->winId != winId)
        drawable = drawable->nextWinId;

    if (drawable)
        ++(drawable->refCount);
    return drawable;
}

int pvrQwsReleaseWindow(PvrQwsDrawable *drawable)
{
    if (drawable->type == PvrQwsWindow)
        return (--(drawable->refCount) <= 0);
    else
        return 0;
}

PvrQwsDrawable *pvrQwsCreatePixmap(int width, int height, int screen)
{
    PvrQwsDrawable *drawable;

    if (!pvrQwsEnsureScreen(screen))
        return 0;

    drawable = (PvrQwsDrawable *)calloc(1, sizeof(PvrQwsDrawable));
    if (!drawable)
        return 0;

    drawable->type = PvrQwsPixmap;
    drawable->screen = screen;
    drawable->pixelFormat = pvrQwsDisplay.screens[screen].pixelFormat;
    drawable->rect.x = 0;
    drawable->rect.y = 0;
    drawable->rect.width = width;
    drawable->rect.height = height;

    if (!pvrQwsAddDrawable()) {
        free(drawable);
        return 0;
    }

    return drawable;
}

static void pvrQwsDestroyDrawableForced(PvrQwsDrawable *drawable)
{
    /* Remove the drawable from the display's winId list */
    PvrQwsDrawable *current = pvrQwsDisplay.firstWinId;
    PvrQwsDrawable *prev = 0;
    while (current != 0 && current != drawable) {
        prev = current;
        current = current->nextWinId;
    }
    if (current != 0) {
        if (prev)
            prev->nextWinId = current->nextWinId;
        else
            pvrQwsDisplay.firstWinId = current->nextWinId;
    }

    pvrQwsFreeBuffers(drawable);
    free(drawable);

    --pvrQwsDisplay.numDrawables;
    if (pvrQwsDisplay.numDrawables == 0)
        pvrQwsDestroyContext();
}

void pvrQwsDestroyDrawable(PvrQwsDrawable *drawable)
{
    if (drawable && drawable->type != PvrQwsScreen)
        pvrQwsDestroyDrawableForced(drawable);
}

PvrQwsDrawableType pvrQwsGetDrawableType(PvrQwsDrawable *drawable)
{
    return drawable->type;
}

void pvrQwsSetVisibleRegion
        (PvrQwsDrawable *drawable, const PvrQwsRect *rects, int numRects)
{
    int index, indexOut;
    PvrQwsRect *rect;
    PvrQwsRect *screenRect;

    /* Visible regions don't make sense for pixmaps */
    if (drawable->type == PvrQwsPixmap)
        return;

    /* Restrict the number of rectangles to prevent buffer overflow */
    if (numRects > PVRQWS_MAX_VISIBLE_RECTS)
        numRects = PVRQWS_MAX_VISIBLE_RECTS;
    if (numRects > 0)
        memcpy(drawable->visibleRects, rects, numRects * sizeof(PvrQwsRect));

    /* Convert the rectangles into screen-relative co-ordinates and
       then clamp them to the screen boundaries.  If any of the
       clamped rectangles are empty, remove them from the list */
    screenRect = &(pvrQwsDisplay.screens[drawable->screen].screenRect);
    indexOut = 0;
    for (index = 0, rect = drawable->visibleRects; index < numRects; ++index, ++rect) {
        if (rect->x < 0) {
            rect->width += rect->x;
            rect->x = 0;
            if (rect->width < 0)
                rect->width = 0;
        } else if (rect->x >= screenRect->width) {
            rect->x = screenRect->width;
            rect->width = 0;
        }
        if ((rect->x + rect->width) > screenRect->width) {
            rect->width = screenRect->width - rect->x;
        }
        if (rect->y < 0) {
            rect->height += rect->y;
            rect->y = 0;
            if (rect->height < 0)
                rect->height = 0;
        } else if (rect->y >= screenRect->height) {
            rect->y = screenRect->height;
            rect->height = 0;
        }
        if ((rect->y + rect->height) > screenRect->height) {
            rect->height = screenRect->height - rect->y;
        }
        if (rect->width > 0 && rect->height > 0) {
            if (index != indexOut)
                drawable->visibleRects[indexOut] = *rect;
            ++indexOut;
        }
    }
    drawable->numVisibleRects = indexOut;
}

void pvrQwsClearVisibleRegion(PvrQwsDrawable *drawable)
{
    if (drawable->type != PvrQwsPixmap)
        drawable->numVisibleRects = 0;
}

void pvrQwsSetGeometry(PvrQwsDrawable *drawable, const PvrQwsRect *rect)
{
    /* We can only change the geometry of window drawables */
    if (drawable->type != PvrQwsWindow)
        return;

    /* If the position has changed, then clear the visible region */
    if (drawable->rect.x != rect->x || drawable->rect.y != rect->y) {
        drawable->rect.x = rect->x;
        drawable->rect.y = rect->y;
        drawable->numVisibleRects = 0;
    }

    /* If the size has changed, then clear the visible region and
       invalidate the drawable's buffers.  Invalidating the buffers
       will force EGL to recreate the drawable, which will then
       allocate new buffers for the new size */
    if (drawable->rect.width != rect->width ||
            drawable->rect.height != rect->height) {
        drawable->rect.width = rect->width;
        drawable->rect.height = rect->height;
        drawable->numVisibleRects = 0;
        pvrQwsInvalidateBuffers(drawable);
    }
}

void pvrQwsGetGeometry(PvrQwsDrawable *drawable, PvrQwsRect *rect)
{
    *rect = drawable->rect;
}

void pvrQwsSetRotation(PvrQwsDrawable *drawable, int angle)
{
    if (drawable->rotationAngle != angle) {
        drawable->rotationAngle = angle;

        /* Force the buffers to be recreated if the rotation angle changes */
        pvrQwsInvalidateBuffers(drawable);
    }
}

int pvrQwsGetStride(PvrQwsDrawable *drawable)
{
    if (drawable->backBuffersValid)
        return drawable->strideBytes;
    else
        return 0;
}

PvrQwsPixelFormat pvrQwsGetPixelFormat(PvrQwsDrawable *drawable)
{
    return (PvrQwsPixelFormat)(drawable->pixelFormat);
}

void *pvrQwsGetRenderBuffer(PvrQwsDrawable *drawable)
{
    if (drawable->backBuffersValid)
        return drawable->backBuffers[drawable->currentBackBuffer]->pBase;
    else
        return 0;
}

int pvrQwsAllocBuffers(PvrQwsDrawable *drawable)
{
    int index;
    int numBuffers = PVRQWS_MAX_BACK_BUFFERS;
    if (drawable->type == PvrQwsPixmap)
        numBuffers = 1;
    if (drawable->backBuffers[0]) {
        if (drawable->backBuffersValid)
            return 1;
        if (!drawable->usingFlipBuffers) {
            for (index = 0; index < numBuffers; ++index)
                PVR2DMemFree(pvrQwsDisplay.context, drawable->backBuffers[index]);
        }
    }
    drawable->stridePixels = (drawable->rect.width + 31) & ~31;
    drawable->strideBytes =
        drawable->stridePixels *
        pvrQwsDisplay.screens[drawable->screen].bytesPerPixel;
    drawable->usingFlipBuffers =
        (pvrQwsDisplay.numFlipBuffers > 0 && drawable->isFullScreen);
    if (drawable->usingFlipBuffers) {
        if (numBuffers > (int)(pvrQwsDisplay.numFlipBuffers))
            numBuffers = pvrQwsDisplay.numFlipBuffers;
        for (index = 0; index < numBuffers; ++index)
            drawable->backBuffers[index] = pvrQwsDisplay.flipBuffers[index];
    } else {
        for (index = 0; index < numBuffers; ++index) {
            if (PVR2DMemAlloc(pvrQwsDisplay.context,
                              drawable->strideBytes * drawable->rect.height,
                              128, 0,
                              &(drawable->backBuffers[index])) != PVR2D_OK) {
                while (--index >= 0)
                    PVR2DMemFree(pvrQwsDisplay.context, drawable->backBuffers[index]);
                memset(drawable->backBuffers, 0, sizeof(drawable->backBuffers));
                drawable->backBuffersValid = 0;
                return 0;
            }
        }
    }
    for (index = numBuffers; index < PVRQWS_MAX_BACK_BUFFERS; ++index) {
        drawable->backBuffers[index] = drawable->backBuffers[0];
    }
    drawable->backBuffersValid = 1;
    drawable->currentBackBuffer = 0;
    return 1;
}

void pvrQwsFreeBuffers(PvrQwsDrawable *drawable)
{
    int index;
    int numBuffers = PVRQWS_MAX_BACK_BUFFERS;
    if (drawable->type == PvrQwsPixmap)
        numBuffers = 1;
    if (!drawable->usingFlipBuffers) {
        for (index = 0; index < numBuffers; ++index) {
            if (drawable->backBuffers[index])
                PVR2DMemFree(pvrQwsDisplay.context, drawable->backBuffers[index]);
        }
    }
    memset(drawable->backBuffers, 0, sizeof(drawable->backBuffers));
    drawable->backBuffersValid = 0;
    drawable->usingFlipBuffers = 0;
}

void pvrQwsInvalidateBuffers(PvrQwsDrawable *drawable)
{
    drawable->backBuffersValid = 0;
}

int pvrQwsGetBuffers
    (PvrQwsDrawable *drawable, PVR2DMEMINFO **source, PVR2DMEMINFO **render)
{
    if (!drawable->backBuffersValid)
        return 0;
    *render = drawable->backBuffers[drawable->currentBackBuffer];
    *source = drawable->backBuffers
        [(drawable->currentBackBuffer + PVRQWS_MAX_BACK_BUFFERS - 1) %
                PVRQWS_MAX_BACK_BUFFERS];
    return 1;
}

int pvrQwsSwapBuffers(PvrQwsDrawable *drawable, int repaintOnly)
{
    PVR2DMEMINFO *buffer;
    PvrQwsRect *rect;
    int index;

    /* Bail out if the back buffers have been invalidated */
    if (!drawable->backBuffersValid)
        return 0;

    /* If there is a swap function, then use that instead */
    if (drawable->swapFunction) {
        (*(drawable->swapFunction))(drawable, drawable->userData, repaintOnly);
        if (!repaintOnly) {
            drawable->currentBackBuffer
                = (drawable->currentBackBuffer + 1) % PVRQWS_MAX_BACK_BUFFERS;
        }
        return 1;
    }

    /* Iterate through the visible rectangles and blit them to the screen */
    if (!repaintOnly) {
        index = drawable->currentBackBuffer;
    } else {
        index = (drawable->currentBackBuffer + PVRQWS_MAX_BACK_BUFFERS - 1)
                        % PVRQWS_MAX_BACK_BUFFERS;
    }
    buffer = drawable->backBuffers[index];
    rect = drawable->visibleRects;
    if (drawable->usingFlipBuffers) {
        PVR2DPresentFlip(pvrQwsDisplay.context, pvrQwsDisplay.flipChain, buffer, 0);
    } else if (pvrQwsDisplay.usePresentBlit && drawable->numVisibleRects > 0) {
        PVR2DRECT pvrRects[PVRQWS_MAX_VISIBLE_RECTS];
        for (index = 0; index < drawable->numVisibleRects; ++index, ++rect) {
            pvrRects[index].left = rect->x;
            pvrRects[index].top = rect->y;
            pvrRects[index].right = rect->x + rect->width;
            pvrRects[index].bottom = rect->y + rect->height;
        }
        for (index = 0; index < drawable->numVisibleRects; index += 4) {
            int numClip = drawable->numVisibleRects - index;
            if (numClip > 4)    /* No more than 4 clip rects at a time */
                numClip = 4;
            PVR2DSetPresentBltProperties
                (pvrQwsDisplay.context,
                 PVR2D_PRESENT_PROPERTY_SRCSTRIDE |
                 PVR2D_PRESENT_PROPERTY_DSTSIZE |
                 PVR2D_PRESENT_PROPERTY_DSTPOS |
                 PVR2D_PRESENT_PROPERTY_CLIPRECTS,
                 drawable->strideBytes,
                 drawable->rect.width, drawable->rect.height,
                 drawable->rect.x, drawable->rect.y,
                 numClip, pvrRects + index, 0);
            PVR2DPresentBlt(pvrQwsDisplay.context, buffer, 0);
        }
        PVR2DQueryBlitsComplete(pvrQwsDisplay.context, buffer, 1);
    } else {
        /* TODO: use PVR2DBltClipped for faster transfers of clipped windows */
        PVR2DBLTINFO blit;
        for (index = 0; index < drawable->numVisibleRects; ++index, ++rect) {
            memset(&blit, 0, sizeof(blit));

            blit.CopyCode = PVR2DROPcopy;
            blit.BlitFlags = PVR2D_BLIT_DISABLE_ALL;

            blit.pSrcMemInfo = buffer;
            blit.SrcStride = drawable->strideBytes;
            blit.SrcX = rect->x - drawable->rect.x;
            blit.SrcY = rect->y - drawable->rect.y;
            blit.SizeX = rect->width;
            blit.SizeY = rect->height;
            blit.SrcFormat = drawable->pixelFormat;

            blit.pDstMemInfo = pvrQwsDisplay.screens[drawable->screen].frameBuffer;
            blit.DstStride = pvrQwsDisplay.screens[drawable->screen].screenStride;
            blit.DstX = rect->x;
            blit.DstY = rect->y;
            blit.DSizeX = rect->width;
            blit.DSizeY = rect->height;
            blit.DstFormat = pvrQwsDisplay.screens[drawable->screen].pixelFormat;

            PVR2DBlt(pvrQwsDisplay.context, &blit);
        }
    }

    /* Swap the buffers */
    if (!repaintOnly) {
        drawable->currentBackBuffer
            = (drawable->currentBackBuffer + 1) % PVRQWS_MAX_BACK_BUFFERS;
    }
    return 1;
}

void pvrQwsSetSwapFunction
    (PvrQwsDrawable *drawable, PvrQwsSwapFunction func, void *userData)
{
    drawable->swapFunction = func;
    drawable->userData = userData;
}
