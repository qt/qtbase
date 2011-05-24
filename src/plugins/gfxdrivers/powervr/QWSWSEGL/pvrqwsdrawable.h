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

#ifndef PVRQWSDRAWABLE_H
#define PVRQWSDRAWABLE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int x, y, width, height;
} PvrQwsRect;

typedef enum
{
    PvrQwsScreen,
    PvrQwsWindow,
    PvrQwsPixmap

} PvrQwsDrawableType;

typedef enum
{
    PvrQws_1BPP = 0,
    PvrQws_RGB565,
    PvrQws_ARGB4444,
    PvrQws_RGB888,
    PvrQws_ARGB8888,
    PvrQws_VGAEMU

} PvrQwsPixelFormat;

typedef struct _PvrQwsDrawable PvrQwsDrawable;

typedef void (*PvrQwsSwapFunction)
    (PvrQwsDrawable *drawable, void *userData, int repaintOnly);

/* Open the display and prepare for window operations.  The display
   can be opened multiple times and each time is reference counted.
   The display will be finally closed when the same number of
   calls to pvrQwsDisplayClose() have been encountered */
int pvrQwsDisplayOpen(void);

/* Close the display */
void pvrQwsDisplayClose(void);

/* Determine if the display is already open */
int pvrQwsDisplayIsOpen(void);

/* Create a window that represents a particular framebuffer screen.
   Initially the visible region will be the whole screen.  If the screen
   window has already been created, then will return the same value */
PvrQwsDrawable *pvrQwsScreenWindow(int screen);

/* Create a top-level window on a particular framebuffer screen.
   Initially the window will not have a visible region */
PvrQwsDrawable *pvrQwsCreateWindow(int screen, long winId, const PvrQwsRect *rect);

/* Fetch an existing window for a window id and increase its refcount */
PvrQwsDrawable *pvrQwsFetchWindow(long winId);

/* Release the refcount on a window.  Returns 1 if refcount is zero */
int pvrQwsReleaseWindow(PvrQwsDrawable *drawable);

/* Create an off-screen pixmap */
PvrQwsDrawable *pvrQwsCreatePixmap(int width, int height, int screen);

/* Destroy a previously-created drawable.  Will not destroy screens. */
void pvrQwsDestroyDrawable(PvrQwsDrawable *drawable);

/* Get a drawable's type */
PvrQwsDrawableType pvrQwsGetDrawableType(PvrQwsDrawable *drawable);

/* Sets the visible region for a window or screen drawable.  Pixels within
   the specified rectangles will be copied to the framebuffer when the window
   or screen is swapped.  The rectangles should be in global co-ordinates */
void pvrQwsSetVisibleRegion
        (PvrQwsDrawable *drawable, const PvrQwsRect *rects, int numRects);

/* Clear the visible region for a window or screen drawable,
   effectively removing it from the screen */
void pvrQwsClearVisibleRegion(PvrQwsDrawable *drawable);

/* Set the geometry for a drawable.  This can only be used on windows */
void pvrQwsSetGeometry(PvrQwsDrawable *drawable, const PvrQwsRect *rect);

/* Get the current geometry for a drawable */
void pvrQwsGetGeometry(PvrQwsDrawable *drawable, PvrQwsRect *rect);

/* Set the rotation angle in degrees */
void pvrQwsSetRotation(PvrQwsDrawable *drawable, int angle);

/* Get the line stride for a drawable.  Returns zero if the buffers
   are not allocated or have been invalidated */
int pvrQwsGetStride(PvrQwsDrawable *drawable);

/* Get the pixel format for a drawable */
PvrQwsPixelFormat pvrQwsGetPixelFormat(PvrQwsDrawable *drawable);

/* Get a pointer to the beginning of a drawable's current render buffer.
   Returns null if the buffers are not allocated or have been invalidated */
void *pvrQwsGetRenderBuffer(PvrQwsDrawable *drawable);

/* Allocate the buffers associated with a drawable.  We allocate one buffer
   for pixmaps, and several for windows and screens */
int pvrQwsAllocBuffers(PvrQwsDrawable *drawable);

/* Free the buffers associated with a drawable */
void pvrQwsFreeBuffers(PvrQwsDrawable *drawable);

/* Invalidate the buffers associated with a drawable.  The buffers will
   still be allocated but the next attempt to swap the buffers will fail */
void pvrQwsInvalidateBuffers(PvrQwsDrawable *drawable);

/* Swap the back buffers for a window or screen and copy to the framebuffer */
int pvrQwsSwapBuffers(PvrQwsDrawable *drawable, int repaintOnly);

/* Set the swap function for a drawable.  When pvrQwsSwapBuffers()
   is called on the drawable, the supplied function will be called
   instead of copying the drawable contents to the screen.  This allows
   higher-level compositors to know when a drawable has changed.
   The swap function can be set to null to return to normal processing */
void pvrQwsSetSwapFunction
    (PvrQwsDrawable *drawable, PvrQwsSwapFunction func, void *userData);

#ifdef __cplusplus
};
#endif

#endif
