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

#include <QtGui/qpaintdevice.h>
#include <QtGui/qpixmap.h>
#include <QtGui/qwidget.h>
#include <QtCore/qatomic.h>
#include <QtCore/qdebug.h>

#include "qegl_p.h"
#include "qeglcontext_p.h"


QT_BEGIN_NAMESPACE


/*
    QEglContextTracker is used to track the EGL contexts that we
    create internally in Qt, so that we can call eglTerminate() to
    free additional EGL resources when the last context is destroyed.
*/

class QEglContextTracker
{
public:
    static void ref() { contexts.ref(); }
    static void deref() {
        if (!contexts.deref()) {
            eglTerminate(QEgl::display());
            displayOpen = 0;
        }
    }
    static void setDisplayOpened() { displayOpen = 1; }
    static bool displayOpened() { return displayOpen; }

private:
    static QBasicAtomicInt contexts;
    static QBasicAtomicInt displayOpen;
};

QBasicAtomicInt QEglContextTracker::contexts = Q_BASIC_ATOMIC_INITIALIZER(0);
QBasicAtomicInt QEglContextTracker::displayOpen = Q_BASIC_ATOMIC_INITIALIZER(0);

// Current GL and VG contexts.  These are used to determine if
// we can avoid an eglMakeCurrent() after a call to lazyDoneCurrent().
// If a background thread modifies the value, the worst that will
// happen is a redundant eglMakeCurrent() in the foreground thread.
static QEglContext * volatile currentGLContext = 0;
static QEglContext * volatile currentVGContext = 0;

QEglContext::QEglContext()
    : apiType(QEgl::OpenGL)
    , ctx(EGL_NO_CONTEXT)
    , cfg(QEGL_NO_CONFIG)
    , currentSurface(EGL_NO_SURFACE)
    , current(false)
    , ownsContext(true)
    , sharing(false)
{
    QEglContextTracker::ref();
}

QEglContext::~QEglContext()
{
    destroyContext();

    if (currentGLContext == this)
        currentGLContext = 0;
    if (currentVGContext == this)
        currentVGContext = 0;
    QEglContextTracker::deref();
}

bool QEglContext::isValid() const
{
    return (ctx != EGL_NO_CONTEXT);
}

bool QEglContext::isCurrent() const
{
    return current;
}

EGLConfig QEgl::defaultConfig(int devType, API api, ConfigOptions options)
{
    if ( (devType != QInternal::Pixmap) && ((options & Renderable) == 0))
        qWarning("QEgl::defaultConfig() - Only configs for pixmaps make sense to be read-only!");

    EGLConfig* targetConfig = 0;

    static EGLConfig defaultVGConfigs[] = {
        QEGL_NO_CONFIG, // 0    Window  Renderable  Translucent
        QEGL_NO_CONFIG, // 1    Window  Renderable  Opaque
        QEGL_NO_CONFIG, // 2    Pixmap  Renderable  Translucent
        QEGL_NO_CONFIG, // 3    Pixmap  Renderable  Opaque
        QEGL_NO_CONFIG, // 4    Pixmap  ReadOnly    Translucent
        QEGL_NO_CONFIG  // 5    Pixmap  ReadOnly    Opaque
    };
    if (api == OpenVG) {
        if (devType == QInternal::Widget) {
            if (options & Translucent)
                targetConfig = &(defaultVGConfigs[0]);
            else
                targetConfig = &(defaultVGConfigs[1]);
        } else if (devType == QInternal::Pixmap) {
            if (options & Renderable) {
                if (options & Translucent)
                    targetConfig = &(defaultVGConfigs[2]);
                else // Opaque
                    targetConfig = &(defaultVGConfigs[3]);
            } else { // Read-only
                if (options & Translucent)
                    targetConfig = &(defaultVGConfigs[4]);
                else // Opaque
                    targetConfig = &(defaultVGConfigs[5]);
            }
        }
    }


    static EGLConfig defaultGLConfigs[] = {
        QEGL_NO_CONFIG, // 0    Window  Renderable  Translucent
        QEGL_NO_CONFIG, // 1    Window  Renderable  Opaque
        QEGL_NO_CONFIG, // 2    PBuffer Renderable  Translucent
        QEGL_NO_CONFIG, // 3    PBuffer Renderable  Opaque
        QEGL_NO_CONFIG, // 4    Pixmap  Renderable  Translucent
        QEGL_NO_CONFIG, // 5    Pixmap  Renderable  Opaque
        QEGL_NO_CONFIG, // 6    Pixmap  ReadOnly    Translucent
        QEGL_NO_CONFIG  // 7    Pixmap  ReadOnly    Opaque
    };
    if (api == OpenGL) {
        if (devType == QInternal::Widget) {
            if (options & Translucent)
                targetConfig = &(defaultGLConfigs[0]);
            else // Opaque
                targetConfig = &(defaultGLConfigs[1]);
        } else if (devType == QInternal::Pbuffer) {
            if (options & Translucent)
                targetConfig = &(defaultGLConfigs[2]);
            else // Opaque
                targetConfig = &(defaultGLConfigs[3]);
        } else if (devType == QInternal::Pixmap) {
            if (options & Renderable) {
                if (options & Translucent)
                    targetConfig = &(defaultGLConfigs[4]);
                else // Opaque
                    targetConfig = &(defaultGLConfigs[5]);
            } else { // ReadOnly
                if (options & Translucent)
                    targetConfig = &(defaultGLConfigs[6]);
                else // Opaque
                    targetConfig = &(defaultGLConfigs[7]);
            }
        }
    }

    if (!targetConfig) {
        qWarning("QEgl::defaultConfig() - No default config for device/api/options combo");
        return QEGL_NO_CONFIG;
    }
    if (*targetConfig != QEGL_NO_CONFIG)
        return *targetConfig;


    // We haven't found an EGL config for the target config yet, so do it now:


    // Allow overriding from an environment variable:
    QByteArray configId;
    if (api == OpenVG)
        configId = qgetenv("QT_VG_EGL_CONFIG");
    else
        configId = qgetenv("QT_GL_EGL_CONFIG");
    if (!configId.isEmpty()) {
        // Overridden, so get the EGLConfig for the specified config ID:
        EGLint properties[] = {
            EGL_CONFIG_ID, (EGLint)configId.toInt(),
            EGL_NONE
        };
        EGLint configCount = 0;
        eglChooseConfig(display(), properties, targetConfig, 1, &configCount);
        if (configCount > 0)
            return *targetConfig;
        qWarning() << "QEgl::defaultConfig() -" << configId << "appears to be invalid";
    }

    QEglProperties configAttribs;
    configAttribs.setRenderableType(api);

    EGLint surfaceType;
    switch (devType) {
        case QInternal::Widget:
            surfaceType = EGL_WINDOW_BIT;
            break;
        case QInternal::Pixmap:
            surfaceType = EGL_PIXMAP_BIT;
            break;
        case QInternal::Pbuffer:
            surfaceType = EGL_PBUFFER_BIT;
            break;
        default:
            qWarning("QEgl::defaultConfig() - Can't create EGL surface for %d device type", devType);
            return QEGL_NO_CONFIG;
    };
#ifdef EGL_VG_ALPHA_FORMAT_PRE_BIT
    // For OpenVG, we try to create a surface using a pre-multiplied format if
    // the surface needs to have an alpha channel:
    if (api == OpenVG && (options & Translucent))
        surfaceType |= EGL_VG_ALPHA_FORMAT_PRE_BIT;
#endif
    configAttribs.setValue(EGL_SURFACE_TYPE, surfaceType);

#ifdef EGL_BIND_TO_TEXTURE_RGBA
    if (devType == QInternal::Pixmap || devType == QInternal::Pbuffer) {
        if (options & Translucent)
            configAttribs.setValue(EGL_BIND_TO_TEXTURE_RGBA, EGL_TRUE);
        else
            configAttribs.setValue(EGL_BIND_TO_TEXTURE_RGB, EGL_TRUE);
    }
#endif

    // Add paint engine requirements
    if (api == OpenVG) {
#if !defined(QVG_SCISSOR_CLIP) && defined(EGL_ALPHA_MASK_SIZE)
        configAttribs.setValue(EGL_ALPHA_MASK_SIZE, 1);
#endif
    } else {
        // Both OpenGL paint engines need to have stencil and sample buffers
        configAttribs.setValue(EGL_STENCIL_SIZE, 1);
        configAttribs.setValue(EGL_SAMPLE_BUFFERS, 1);
#ifndef QT_OPENGL_ES_2
        // Additionally, the GL1 engine likes to have a depth buffer for clipping
        configAttribs.setValue(EGL_DEPTH_SIZE, 1);
#endif
    }

    if (options & Translucent)
        configAttribs.setValue(EGL_ALPHA_SIZE, 1);

    *targetConfig = chooseConfig(&configAttribs, QEgl::BestPixelFormat);
    return *targetConfig;
}


// Choose a configuration that matches "properties".
EGLConfig QEgl::chooseConfig(const QEglProperties* properties, QEgl::PixelFormatMatch match)
{
    QEglProperties props(*properties);
    EGLConfig cfg = QEGL_NO_CONFIG;
    do {
        // Get the number of matching configurations for this set of properties.
        EGLint matching = 0;
        EGLDisplay dpy = QEgl::display();
        if (!eglChooseConfig(dpy, props.properties(), 0, 0, &matching) || !matching)
            continue;

        // If we want the best pixel format, then return the first
        // matching configuration.
        if (match == QEgl::BestPixelFormat) {
            eglChooseConfig(display(), props.properties(), &cfg, 1, &matching);
            if (matching < 1)
                continue;
            return cfg;
        }

        // Fetch all of the matching configurations and find the
        // first that matches the pixel format we wanted.
        EGLint size = matching;
        EGLConfig *configs = new EGLConfig [size];
        eglChooseConfig(display(), props.properties(), configs, size, &matching);
        for (EGLint index = 0; index < size; ++index) {
            EGLint red, green, blue, alpha;
            eglGetConfigAttrib(display(), configs[index], EGL_RED_SIZE, &red);
            eglGetConfigAttrib(display(), configs[index], EGL_GREEN_SIZE, &green);
            eglGetConfigAttrib(display(), configs[index], EGL_BLUE_SIZE, &blue);
            eglGetConfigAttrib(display(), configs[index], EGL_ALPHA_SIZE, &alpha);
            if (red == props.value(EGL_RED_SIZE) &&
                    green == props.value(EGL_GREEN_SIZE) &&
                    blue == props.value(EGL_BLUE_SIZE) &&
                    (props.value(EGL_ALPHA_SIZE) == 0 ||
                     alpha == props.value(EGL_ALPHA_SIZE))) {
                cfg = configs[index];
                delete [] configs;
                return cfg;
            }
        }
        delete [] configs;
    } while (props.reduceConfiguration());

#ifdef EGL_BIND_TO_TEXTURE_RGBA
    // Don't report an error just yet if we failed to get a pbuffer
    // configuration with texture rendering.  Only report failure if
    // we cannot get any pbuffer configurations at all.
    if (props.value(EGL_BIND_TO_TEXTURE_RGBA) == EGL_DONT_CARE &&
        props.value(EGL_BIND_TO_TEXTURE_RGB) == EGL_DONT_CARE)
#endif
    {
        qWarning() << "QEglContext::chooseConfig(): Could not find a suitable EGL configuration";
        qWarning() << "Requested:" << props.toString();
        qWarning() << "Available:";
        QEgl::dumpAllConfigs();
    }
    return QEGL_NO_CONFIG;
}

bool QEglContext::chooseConfig(const QEglProperties& properties, QEgl::PixelFormatMatch match)
{
    cfg = QEgl::chooseConfig(&properties, match);
    return cfg != QEGL_NO_CONFIG;
}

EGLSurface QEglContext::createSurface(QPaintDevice* device, const QEglProperties *properties)
{
    return QEgl::createSurface(device, cfg, properties);
}


// Create the EGLContext.
bool QEglContext::createContext(QEglContext *shareContext, const QEglProperties *properties)
{
    // We need to select the correct API before calling eglCreateContext().
#ifdef QT_OPENGL_ES
#ifdef EGL_OPENGL_ES_API
    if (apiType == QEgl::OpenGL)
        eglBindAPI(EGL_OPENGL_ES_API);
#endif
#else
#ifdef EGL_OPENGL_API
    if (apiType == QEgl::OpenGL)
        eglBindAPI(EGL_OPENGL_API);
#endif
#endif //defined(QT_OPENGL_ES)
#ifdef EGL_OPENVG_API
    if (apiType == QEgl::OpenVG)
        eglBindAPI(EGL_OPENVG_API);
#endif

    // Create a new context for the configuration.
    QEglProperties contextProps;
    if (properties)
        contextProps = *properties;
#ifdef QT_OPENGL_ES_2
    if (apiType == QEgl::OpenGL)
        contextProps.setValue(EGL_CONTEXT_CLIENT_VERSION, 2);
#endif
    sharing = false;
    if (shareContext && shareContext->ctx == EGL_NO_CONTEXT)
        shareContext = 0;
    if (shareContext) {
        ctx = eglCreateContext(QEgl::display(), cfg, shareContext->ctx, contextProps.properties());
        if (ctx == EGL_NO_CONTEXT) {
            qWarning() << "QEglContext::createContext(): Could not share context:" << QEgl::errorString();
            shareContext = 0;
        } else {
            sharing = true;
        }
    }
    if (ctx == EGL_NO_CONTEXT) {
        ctx = eglCreateContext(display(), cfg, EGL_NO_CONTEXT, contextProps.properties());
        if (ctx == EGL_NO_CONTEXT) {
            qWarning() << "QEglContext::createContext(): Unable to create EGL context:" << QEgl::errorString();
            return false;
        }
    }
    return true;
}

// Destroy an EGL surface object.  If it was current on this context
// then call doneCurrent() for it first.
void QEglContext::destroySurface(EGLSurface surface)
{
    if (surface != EGL_NO_SURFACE) {
        if (surface == currentSurface)
            doneCurrent();
        eglDestroySurface(display(), surface);
    }
}

// Destroy the context.  Note: this does not destroy the surface.
void QEglContext::destroyContext()
{
    if (ctx != EGL_NO_CONTEXT && ownsContext)
        eglDestroyContext(display(), ctx);
    ctx = EGL_NO_CONTEXT;
    cfg = 0;
}

bool QEglContext::makeCurrent(EGLSurface surface)
{
    if (ctx == EGL_NO_CONTEXT) {
        qWarning() << "QEglContext::makeCurrent(): Cannot make invalid context current";
        return false;
    }

    if (surface == EGL_NO_SURFACE) {
        qWarning() << "QEglContext::makeCurrent(): Cannot make invalid surface current";
        return false;
    }

    // If lazyDoneCurrent() was called on the surface, then we may be able
    // to assume that it is still current within the thread.
    if (surface == currentSurface && currentContext(apiType) == this) {
        current = true;
        return true;
    }

    current = true;
    currentSurface = surface;
    setCurrentContext(apiType, this);

    // Force the right API to be bound before making the context current.
    // The EGL implementation should be able to figure this out from ctx,
    // but some systems require the API to be explicitly set anyway.
#ifdef EGL_OPENGL_ES_API
    if (apiType == QEgl::OpenGL)
        eglBindAPI(EGL_OPENGL_ES_API);
#endif
#ifdef EGL_OPENVG_API
    if (apiType == QEgl::OpenVG)
        eglBindAPI(EGL_OPENVG_API);
#endif

    bool ok = eglMakeCurrent(QEgl::display(), surface, surface, ctx);
    if (!ok)
        qWarning() << "QEglContext::makeCurrent(" << surface << "):" << QEgl::errorString();
    return ok;
}

bool QEglContext::doneCurrent()
{
    // If the context is invalid, we assume that an error was reported
    // when makeCurrent() was called.
    if (ctx == EGL_NO_CONTEXT)
        return false;

    current = false;
    currentSurface = EGL_NO_SURFACE;
    setCurrentContext(apiType, 0);

    // We need to select the correct API before calling eglMakeCurrent()
    // with EGL_NO_CONTEXT because threads can have both OpenGL and OpenVG
    // contexts active at the same time.
#ifdef EGL_OPENGL_ES_API
    if (apiType == QEgl::OpenGL)
        eglBindAPI(EGL_OPENGL_ES_API);
#endif
#ifdef EGL_OPENVG_API
    if (apiType == QEgl::OpenVG)
        eglBindAPI(EGL_OPENVG_API);
#endif

    bool ok = eglMakeCurrent(QEgl::display(), EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (!ok)
        qWarning() << "QEglContext::doneCurrent():" << QEgl::errorString();
    return ok;
}

// Act as though doneCurrent() was called, but keep the context
// and the surface active for the moment.  This allows makeCurrent()
// to skip a call to eglMakeCurrent() if we are using the same
// surface as the last set of painting operations.  We leave the
// currentContext() pointer as-is for now.
bool QEglContext::lazyDoneCurrent()
{
    current = false;
    return true;
}

bool QEglContext::swapBuffers(EGLSurface surface)
{
    if(ctx == EGL_NO_CONTEXT)
        return false;

    bool ok = eglSwapBuffers(QEgl::display(), surface);
    if (!ok)
        qWarning() << "QEglContext::swapBuffers():" << QEgl::errorString();
    return ok;
}

bool QEglContext::swapBuffersRegion2NOK(EGLSurface surface, const QRegion *region) {
    QVector<QRect> qrects = region->rects();
    EGLint *gl_rects;
    uint count;
    uint i;

    count = qrects.size();
    QVarLengthArray <EGLint> arr(4 * count);
    gl_rects = arr.data();
    for (i = 0; i < count; i++) {
      QRect qrect = qrects[i];

      gl_rects[4 * i + 0] = qrect.x();
      gl_rects[4 * i + 1] = qrect.y();
      gl_rects[4 * i + 2] = qrect.width();
      gl_rects[4 * i + 3] = qrect.height();
    }

    bool ok = QEgl::eglSwapBuffersRegion2NOK(QEgl::display(), surface, count, gl_rects);

    if (!ok)
        qWarning() << "QEglContext::swapBuffersRegion2NOK():" << QEgl::errorString();
    return ok;
}

int QEglContext::configAttrib(int name) const
{
    EGLint value;
    EGLBoolean success = eglGetConfigAttrib(QEgl::display(), cfg, name, &value);
    if (success)
        return value;
    else
        return EGL_DONT_CARE;
}

typedef EGLImageKHR (EGLAPIENTRY *_eglCreateImageKHR)(EGLDisplay, EGLContext, EGLenum, EGLClientBuffer, const EGLint*);
typedef EGLBoolean (EGLAPIENTRY *_eglDestroyImageKHR)(EGLDisplay, EGLImageKHR);

// Defined in qegl.cpp:
static _eglCreateImageKHR qt_eglCreateImageKHR = 0;
static _eglDestroyImageKHR qt_eglDestroyImageKHR = 0;

typedef EGLBoolean (EGLAPIENTRY *_eglSwapBuffersRegion2NOK)(EGLDisplay, EGLSurface, EGLint, const EGLint*);

static _eglSwapBuffersRegion2NOK qt_eglSwapBuffersRegion2NOK = 0;

EGLDisplay QEgl::display()
{
    static EGLDisplay dpy = EGL_NO_DISPLAY;
    if (!QEglContextTracker::displayOpened()) {
        dpy = eglGetDisplay(nativeDisplay());
        QEglContextTracker::setDisplayOpened();
        if (dpy == EGL_NO_DISPLAY) {
            qWarning("QEgl::display(): Falling back to EGL_DEFAULT_DISPLAY");
            dpy = eglGetDisplay(EGLNativeDisplayType(EGL_DEFAULT_DISPLAY));
        }
        if (dpy == EGL_NO_DISPLAY) {
            qWarning("QEgl::display(): Can't even open the default display");
            return EGL_NO_DISPLAY;
        }

        if (!eglInitialize(dpy, NULL, NULL)) {
            qWarning() << "QEgl::display(): Cannot initialize EGL display:" << QEgl::errorString();
            return EGL_NO_DISPLAY;
        }

        // Resolve the egl extension function pointers:
#if (defined(EGL_KHR_image) || defined(EGL_KHR_image_base)) && !defined(EGL_EGLEXT_PROTOTYPES)
        if (QEgl::hasExtension("EGL_KHR_image") || QEgl::hasExtension("EGL_KHR_image_base")) {
            qt_eglCreateImageKHR = (_eglCreateImageKHR) eglGetProcAddress("eglCreateImageKHR");
            qt_eglDestroyImageKHR = (_eglDestroyImageKHR) eglGetProcAddress("eglDestroyImageKHR");
        }
#endif

        if (QEgl::hasExtension("EGL_NOK_swap_region2")) {
            qt_eglSwapBuffersRegion2NOK = (_eglSwapBuffersRegion2NOK) eglGetProcAddress("eglSwapBuffersRegion2NOK");
        }
    }

    return dpy;
}

EGLImageKHR QEgl::eglCreateImageKHR(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list)
{
    if (qt_eglCreateImageKHR)
        return qt_eglCreateImageKHR(dpy, ctx, target, buffer, attrib_list);

    QEgl::display(); // Initialises function pointers
    if (qt_eglCreateImageKHR)
        return qt_eglCreateImageKHR(dpy, ctx, target, buffer, attrib_list);

    qWarning("QEgl::eglCreateImageKHR() called but EGL_KHR_image(_base) extension not present");
    return 0;
}

EGLBoolean QEgl::eglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR img)
{
    if (qt_eglDestroyImageKHR)
        return qt_eglDestroyImageKHR(dpy, img);

    QEgl::display(); // Initialises function pointers
    if (qt_eglDestroyImageKHR)
        return qt_eglDestroyImageKHR(dpy, img);

    qWarning("QEgl::eglDestroyImageKHR() called but EGL_KHR_image(_base) extension not present");
    return 0;
}

EGLBoolean QEgl::eglSwapBuffersRegion2NOK(EGLDisplay dpy, EGLSurface surface, EGLint count, const EGLint *rects)
{
    if (qt_eglSwapBuffersRegion2NOK)
        return qt_eglSwapBuffersRegion2NOK(dpy, surface, count, rects);

    QEgl::display(); // Initialises function pointers
    if (qt_eglSwapBuffersRegion2NOK)
        return qt_eglSwapBuffersRegion2NOK(dpy, surface, count, rects);

    qWarning("QEgl::eglSwapBuffersRegion2NOK() called but EGL_NOK_swap_region2 extension not present");
    return 0;
}

#ifndef Q_WS_X11
EGLSurface QEgl::createSurface(QPaintDevice *device, EGLConfig cfg, const QEglProperties *properties)
{
    // Create the native drawable for the paint device.
    int devType = device->devType();
    EGLNativePixmapType pixmapDrawable = 0;
    EGLNativeWindowType windowDrawable = 0;
    bool ok;
    if (devType == QInternal::Pixmap) {
        pixmapDrawable = nativePixmap(static_cast<QPixmap *>(device));
        ok = (pixmapDrawable != 0);
    } else if (devType == QInternal::Widget) {
        windowDrawable = nativeWindow(static_cast<QWidget *>(device));
        ok = (windowDrawable != 0);
    } else {
        ok = false;
    }
    if (!ok) {
        qWarning("QEglContext::createSurface(): Cannot create the native EGL drawable");
        return EGL_NO_SURFACE;
    }

    // Create the EGL surface to draw into, based on the native drawable.
    const int *props;
    if (properties)
        props = properties->properties();
    else
        props = 0;
    EGLSurface surf;
    if (devType == QInternal::Widget)
        surf = eglCreateWindowSurface(QEgl::display(), cfg, windowDrawable, props);
    else
        surf = eglCreatePixmapSurface(QEgl::display(), cfg, pixmapDrawable, props);
    if (surf == EGL_NO_SURFACE) {
        qWarning("QEglContext::createSurface(): Unable to create EGL surface, error = 0x%x", eglGetError());
    }
    return surf;
}
#endif


// Return the error string associated with a specific code.
QString QEgl::errorString(EGLint code)
{
    static const char * const errors[] = {
        "Success (0x3000)",                 // No tr
        "Not initialized (0x3001)",         // No tr
        "Bad access (0x3002)",              // No tr
        "Bad alloc (0x3003)",               // No tr
        "Bad attribute (0x3004)",           // No tr
        "Bad config (0x3005)",              // No tr
        "Bad context (0x3006)",             // No tr
        "Bad current surface (0x3007)",     // No tr
        "Bad display (0x3008)",             // No tr
        "Bad match (0x3009)",               // No tr
        "Bad native pixmap (0x300A)",       // No tr
        "Bad native window (0x300B)",       // No tr
        "Bad parameter (0x300C)",           // No tr
        "Bad surface (0x300D)",             // No tr
        "Context lost (0x300E)"             // No tr
    };
    if (code >= 0x3000 && code <= 0x300E) {
        return QString::fromLatin1(errors[code - 0x3000]);
    } else {
        return QLatin1String("0x") + QString::number(int(code), 16);
    }
}

// Dump all of the EGL configurations supported by the system.
void QEgl::dumpAllConfigs()
{
    QEglProperties props;
    EGLint count = 0;
    if (!eglGetConfigs(display(), 0, 0, &count) || count < 1)
        return;
    EGLConfig *configs = new EGLConfig [count];
    eglGetConfigs(display(), configs, count, &count);
    for (EGLint index = 0; index < count; ++index) {
        props = QEglProperties(configs[index]);
        qWarning() << props.toString();
    }
    delete [] configs;
}

QString QEgl::extensions()
{
    const char* exts = eglQueryString(QEgl::display(), EGL_EXTENSIONS);
    return QString(QLatin1String(exts));
}

bool QEgl::hasExtension(const char* extensionName)
{
    QList<QByteArray> extensions =
        QByteArray(reinterpret_cast<const char *>
            (eglQueryString(QEgl::display(), EGL_EXTENSIONS))).split(' ');
    return extensions.contains(extensionName);
}

QEglContext *QEglContext::currentContext(QEgl::API api)
{
    if (api == QEgl::OpenGL)
        return currentGLContext;
    else
        return currentVGContext;
}

void QEglContext::setCurrentContext(QEgl::API api, QEglContext *context)
{
    if (api == QEgl::OpenGL)
        currentGLContext = context;
    else
        currentVGContext = context;
}

QT_END_NAMESPACE
