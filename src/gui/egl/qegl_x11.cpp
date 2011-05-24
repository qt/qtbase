/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include <QtCore/qdebug.h>

#include <QtGui/private/qt_x11_p.h>
#include <QtGui/qx11info_x11.h>
#include <QtGui/private/qpixmapdata_p.h>
#include <QtGui/private/qpixmap_x11_p.h>
#include <QtGui/private/qimagepixmapcleanuphooks_p.h>

#include <QtGui/qpaintdevice.h>
#include <QtGui/qpixmap.h>
#include <QtGui/qwidget.h>
#include <QtGui/qcolormap.h>

#include "QtGui/private/qegl_p.h"
#include "QtGui/private/qeglcontext_p.h"

QT_BEGIN_NAMESPACE


EGLNativeDisplayType QEgl::nativeDisplay()
{
    Display *xdpy = QX11Info::display();
    if (!xdpy) {
        qWarning("QEglContext::getDisplay(): X11 display is not open");
        return EGLNativeDisplayType(EGL_DEFAULT_DISPLAY);
    }
    return EGLNativeDisplayType(xdpy);
}

EGLNativeWindowType QEgl::nativeWindow(QWidget* widget)
{
    return (EGLNativeWindowType)(widget->winId());
}

EGLNativePixmapType QEgl::nativePixmap(QPixmap* pixmap)
{
    return (EGLNativePixmapType)(pixmap->handle());
}

static int countBits(unsigned long mask)
{
    int count = 0;
    while (mask != 0) {
        if (mask & 1)
            ++count;
        mask >>= 1;
    }
    return count;
}

// Set the pixel format parameters from the visual in "xinfo".
void QEglProperties::setVisualFormat(const QX11Info *xinfo)
{
    if (!xinfo)
        return;
    Visual *visual = (Visual*)xinfo->visual();
    if (!visual)
        return;
    if (visual->c_class != TrueColor && visual->c_class != DirectColor)
        return;
    setValue(EGL_RED_SIZE, countBits(visual->red_mask));
    setValue(EGL_GREEN_SIZE, countBits(visual->green_mask));
    setValue(EGL_BLUE_SIZE, countBits(visual->blue_mask));

    EGLint alphaBits = 0;
#if !defined(QT_NO_XRENDER)
    XRenderPictFormat *format;
    format = XRenderFindVisualFormat(xinfo->display(), visual);
    if (format && (format->type == PictTypeDirect) && format->direct.alphaMask) {
        alphaBits = countBits(format->direct.alphaMask);
        qDebug("QEglProperties::setVisualFormat() - visual's alphaMask is %d", alphaBits);
    }
#endif
    setValue(EGL_ALPHA_SIZE, alphaBits);
}

extern const QX11Info *qt_x11Info(const QPaintDevice *pd);

// Set pixel format and other properties based on a paint device.
void QEglProperties::setPaintDeviceFormat(QPaintDevice *dev)
{
    if (!dev)
        return;
    if (dev->devType() == QInternal::Image)
        setPixelFormat(static_cast<QImage *>(dev)->format());
    else
        setVisualFormat(qt_x11Info(dev));
}

//#define QT_DEBUG_X11_VISUAL_SELECTION 1

VisualID QEgl::getCompatibleVisualId(EGLConfig config)
{
    VisualID    visualId = 0;
    EGLint      eglValue = 0;

    EGLint configRedSize = 0;
    eglGetConfigAttrib(display(), config, EGL_RED_SIZE, &configRedSize);

    EGLint configGreenSize = 0;
    eglGetConfigAttrib(display(), config, EGL_GREEN_SIZE, &configGreenSize);

    EGLint configBlueSize = 0;
    eglGetConfigAttrib(display(), config, EGL_BLUE_SIZE, &configBlueSize);

    EGLint configAlphaSize = 0;
    eglGetConfigAttrib(display(), config, EGL_ALPHA_SIZE, &configAlphaSize);

    eglGetConfigAttrib(display(), config, EGL_CONFIG_ID, &eglValue);
    int configId = eglValue;

    // See if EGL provided a valid VisualID:
    eglGetConfigAttrib(display(), config, EGL_NATIVE_VISUAL_ID, &eglValue);
    visualId = (VisualID)eglValue;
    if (visualId) {
        // EGL has suggested a visual id, so get the rest of the visual info for that id:
        XVisualInfo visualInfoTemplate;
        memset(&visualInfoTemplate, 0, sizeof(XVisualInfo));
        visualInfoTemplate.visualid = visualId;

        XVisualInfo *chosenVisualInfo;
        int matchingCount = 0;
        chosenVisualInfo = XGetVisualInfo(X11->display, VisualIDMask, &visualInfoTemplate, &matchingCount);
        if (chosenVisualInfo) {
            // Skip size checks if implementation supports non-matching visual
            // and config (http://bugreports.qt.nokia.com/browse/QTBUG-9444).
            if (QEgl::hasExtension("EGL_NV_post_convert_rounding")) {
                XFree(chosenVisualInfo);
                return visualId;
            }

            int visualRedSize = countBits(chosenVisualInfo->red_mask);
            int visualGreenSize = countBits(chosenVisualInfo->green_mask);
            int visualBlueSize = countBits(chosenVisualInfo->blue_mask);
            int visualAlphaSize = -1; // Need XRender to tell us the alpha channel size

#if !defined(QT_NO_XRENDER)
            if (X11->use_xrender) {
                // If we have XRender, actually check the visual supplied by EGL is ARGB
                XRenderPictFormat *format;
                format = XRenderFindVisualFormat(X11->display, chosenVisualInfo->visual);
                if (format && (format->type == PictTypeDirect))
                    visualAlphaSize = countBits(format->direct.alphaMask);
            }
#endif

            bool visualMatchesConfig = false;
            if ( visualRedSize == configRedSize &&
                 visualGreenSize == configGreenSize &&
                 visualBlueSize == configBlueSize )
            {
                // We need XRender to check the alpha channel size of the visual. If we don't have
                // the alpha size, we don't check it against the EGL config's alpha size.
                if (visualAlphaSize >= 0)
                    visualMatchesConfig = visualAlphaSize == configAlphaSize;
                else
                    visualMatchesConfig = true;
            }

            if (!visualMatchesConfig) {
                if (visualAlphaSize >= 0) {
                    qWarning("Warning: EGL suggested using X Visual ID %d (ARGB%d%d%d%d) for EGL config %d (ARGB%d%d%d%d), but this is incompatable",
                             (int)visualId, visualAlphaSize, visualRedSize, visualGreenSize, visualBlueSize,
                             configId, configAlphaSize, configRedSize, configGreenSize, configBlueSize);
                } else {
                    qWarning("Warning: EGL suggested using X Visual ID %d (RGB%d%d%d) for EGL config %d (RGB%d%d%d), but this is incompatable",
                             (int)visualId, visualRedSize, visualGreenSize, visualBlueSize,
                             configId, configRedSize, configGreenSize, configBlueSize);
                }
                visualId = 0;
            }
        } else {
            qWarning("Warning: EGL suggested using X Visual ID %d for EGL config %d, but that isn't a valid ID",
                     (int)visualId, configId);
            visualId = 0;
        }
        XFree(chosenVisualInfo);
    }
#ifdef QT_DEBUG_X11_VISUAL_SELECTION
    else
        qDebug("EGL did not suggest a VisualID (EGL_NATIVE_VISUAL_ID was zero) for EGLConfig %d", configId);
#endif

    if (visualId) {
#ifdef QT_DEBUG_X11_VISUAL_SELECTION
        if (configAlphaSize > 0)
            qDebug("Using ARGB Visual ID %d provided by EGL for config %d", (int)visualId, configId);
        else
            qDebug("Using Opaque Visual ID %d provided by EGL for config %d", (int)visualId, configId);
#endif
        return visualId;
    }


    // If EGL didn't give us a valid visual ID, try XRender
#if !defined(QT_NO_XRENDER)
    if (!visualId && X11->use_xrender) {
        XVisualInfo visualInfoTemplate;
        memset(&visualInfoTemplate, 0, sizeof(XVisualInfo));

        visualInfoTemplate.c_class = TrueColor;

        XVisualInfo *matchingVisuals;
        int matchingCount = 0;
        matchingVisuals = XGetVisualInfo(X11->display,
                                         VisualClassMask,
                                         &visualInfoTemplate,
                                         &matchingCount);

        for (int i = 0; i < matchingCount; ++i) {
            XRenderPictFormat *format;
            format = XRenderFindVisualFormat(X11->display, matchingVisuals[i].visual);

            // Check the format for the visual matches the EGL config
            if ( (countBits(format->direct.redMask) == configRedSize) &&
                 (countBits(format->direct.greenMask) == configGreenSize) &&
                 (countBits(format->direct.blueMask) == configBlueSize) &&
                 (countBits(format->direct.alphaMask) == configAlphaSize) )
            {
                visualId = matchingVisuals[i].visualid;
                break;
            }
        }
        if (matchingVisuals)
            XFree(matchingVisuals);

    }
    if (visualId) {
# ifdef QT_DEBUG_X11_VISUAL_SELECTION
        if (configAlphaSize > 0)
            qDebug("Using ARGB Visual ID %d provided by XRender for EGL config %d", (int)visualId, configId);
        else
            qDebug("Using Opaque Visual ID %d provided by XRender for EGL config %d", (int)visualId, configId);
# endif // QT_DEBUG_X11_VISUAL_SELECTION
        return visualId;
    }
# ifdef QT_DEBUG_X11_VISUAL_SELECTION
    else
        qDebug("Failed to find an XVisual which matches EGL config %d using XRender", configId);
# endif // QT_DEBUG_X11_VISUAL_SELECTION

#endif //!defined(QT_NO_XRENDER)


    // Finally, if XRender also failed to find a visual (or isn't present), try to
    // use XGetVisualInfo and only use the bit depths to match on:
    if (!visualId) {
        XVisualInfo visualInfoTemplate;
        memset(&visualInfoTemplate, 0, sizeof(XVisualInfo));
        XVisualInfo *matchingVisuals;
        int matchingCount = 0;

        visualInfoTemplate.depth = configRedSize + configGreenSize + configBlueSize + configAlphaSize;
        matchingVisuals = XGetVisualInfo(X11->display,
                                         VisualDepthMask,
                                         &visualInfoTemplate,
                                         &matchingCount);
        if (!matchingVisuals) {
            // Try again without taking the alpha channel into account:
            visualInfoTemplate.depth = configRedSize + configGreenSize + configBlueSize;
            matchingVisuals = XGetVisualInfo(X11->display,
                                             VisualDepthMask,
                                             &visualInfoTemplate,
                                             &matchingCount);
        }

        if (matchingVisuals) {
            visualId = matchingVisuals[0].visualid;
            XFree(matchingVisuals);
        }
    }

    if (visualId) {
#ifdef QT_DEBUG_X11_VISUAL_SELECTION
        qDebug("Using Visual ID %d provided by XGetVisualInfo for EGL config %d", (int)visualId, configId);
#endif
        return visualId;
    }

    qWarning("Unable to find an X11 visual which matches EGL config %d", configId);
    return (VisualID)0;
}

void qt_set_winid_on_widget(QWidget* w, Qt::HANDLE id)
{
    w->create(id);
}


// NOTE: The X11 version of createSurface will re-create the native drawable if it's visual doesn't
// match the one for the passed in EGLConfig
EGLSurface QEgl::createSurface(QPaintDevice *device, EGLConfig config, const QEglProperties *properties)
{
    int devType = device->devType();

    if (devType == QInternal::Pbuffer) {
        // TODO
        return EGL_NO_SURFACE;
    }

    QX11PixmapData *x11PixmapData = 0;
    if (devType == QInternal::Pixmap) {
        QPixmapData *pmd = static_cast<QPixmap*>(device)->data_ptr().data();
        if (pmd->classId() == QPixmapData::X11Class)
            x11PixmapData = static_cast<QX11PixmapData*>(pmd);
        else {
            // TODO: Replace the pixmap's data with a new QX11PixmapData
            qWarning("WARNING: Creating an EGL surface on a QPixmap is only supported for QX11PixmapData");
            return EGL_NO_SURFACE;
        }
    } else if ((devType != QInternal::Widget) && (devType != QInternal::Pbuffer)) {
        qWarning("WARNING: Creating an EGLSurface for device type %d isn't supported", devType);
        return EGL_NO_SURFACE;
    }

    VisualID visualId = QEgl::getCompatibleVisualId(config);
    EGLint alphaSize;
    eglGetConfigAttrib(QEgl::display(), config, EGL_ALPHA_SIZE, &alphaSize);

    if (devType == QInternal::Widget) {
        QWidget *widget = static_cast<QWidget*>(device);

        VisualID currentVisualId = 0;
        if (widget->testAttribute(Qt::WA_WState_Created))
            currentVisualId = XVisualIDFromVisual((Visual*)widget->x11Info().visual());

        if (currentVisualId != visualId) {
            // The window is either not created or has the wrong visual. Either way, we need
            // to create a window with the correct visual and call create() on the widget:

            bool visible = widget->isVisible();
            if (visible)
                widget->hide();

            XVisualInfo visualInfo;
            visualInfo.visualid = visualId;
            {
                XVisualInfo *visualInfoPtr;
                int matchingCount = 0;
                visualInfoPtr = XGetVisualInfo(widget->x11Info().display(), VisualIDMask,
                                               &visualInfo, &matchingCount);
                Q_ASSERT(visualInfoPtr); // visualId really should be valid!
                visualInfo = *visualInfoPtr;
                XFree(visualInfoPtr);
            }

            Window parentWindow = RootWindow(widget->x11Info().display(), widget->x11Info().screen());
            if (widget->parentWidget())
                parentWindow = widget->parentWidget()->winId();

            XSetWindowAttributes windowAttribs;
            QColormap colmap = QColormap::instance(widget->x11Info().screen());
            windowAttribs.background_pixel = colmap.pixel(widget->palette().color(widget->backgroundRole()));
            windowAttribs.border_pixel = colmap.pixel(Qt::black);

            unsigned int valueMask = CWBackPixel|CWBorderPixel;
            if (alphaSize > 0) {
                windowAttribs.colormap = XCreateColormap(widget->x11Info().display(), parentWindow,
                                                         visualInfo.visual, AllocNone);
                valueMask |= CWColormap;
            }

            Window window = XCreateWindow(widget->x11Info().display(), parentWindow,
                                          widget->x(), widget->y(), widget->width(), widget->height(),
                                          0, visualInfo.depth, InputOutput, visualInfo.visual,
                                          valueMask, &windowAttribs);

            // This is a nasty hack to get round the fact that we can't be a friend of QWidget:
            qt_set_winid_on_widget(widget, window);

            if (visible)
                widget->show();
        }

        // At this point, the widget's window should be created and have the correct visual. Now we
        // just need to create the EGL surface for it:
        const int *props;
        if (properties)
            props = properties->properties();
        else
            props = 0;
        EGLSurface surf = eglCreateWindowSurface(QEgl::display(), config, (EGLNativeWindowType)widget->winId(), props);
        if (surf == EGL_NO_SURFACE)
            qWarning("QEglContext::createSurface(): Unable to create EGL surface, error = 0x%x", eglGetError());
        return surf;
    }

    if (x11PixmapData) {
        // X11 Pixmaps are only created with a depth, so that's all we need to check
        EGLint configDepth;
        eglGetConfigAttrib(QEgl::display(), config, EGL_BUFFER_SIZE , &configDepth);
        if (x11PixmapData->depth() != configDepth) {
            // The bit depths are wrong which means the EGLConfig isn't compatable with
            // this pixmap. So we need to replace the pixmap's existing data with a new
            // one which is created with the correct depth:

#ifndef QT_NO_XRENDER
            if (configDepth == 32) {
                qWarning("Warning: EGLConfig's depth (32) != pixmap's depth (%d), converting to ARGB32",
                         x11PixmapData->depth());
                x11PixmapData->convertToARGB32(true);
            } else
#endif
            {
                qWarning("Warning: EGLConfig's depth (%d) != pixmap's depth (%d)",
                         configDepth, x11PixmapData->depth());
            }
        }

        QEglProperties surfaceAttribs;

        // If the pixmap can't be bound to a texture, it's pretty useless
        surfaceAttribs.setValue(EGL_TEXTURE_TARGET, EGL_TEXTURE_2D);
        if (alphaSize > 0)
            surfaceAttribs.setValue(EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGBA);
        else
            surfaceAttribs.setValue(EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGB);

        EGLSurface surf = eglCreatePixmapSurface(QEgl::display(), config,
                                                 (EGLNativePixmapType) x11PixmapData->handle(),
                                                 surfaceAttribs.properties());
        x11PixmapData->gl_surface = (void*)surf;
        QImagePixmapCleanupHooks::enableCleanupHooks(x11PixmapData);
        return surf;
    }

    return EGL_NO_SURFACE;
}

QT_END_NAMESPACE
