/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
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

#include "qgl.h"

// There are functions that are deprecated in 10.5, but really there's no way around them
// for Carbon, so just undefine them.
#undef DEPRECATED_ATTRIBUTE
#define DEPRECATED_ATTRIBUTE
#if defined(Q_WS_MAC)
#ifndef QT_MAC_USE_COCOA
#ifdef qDebug
#    undef qDebug
#    include <AGL/agl.h>
#    include <AGL/aglRenderers.h>
#    include <OpenGL/gl.h>
#    ifdef QT_NO_DEBUG
#        define qDebug qt_noop(),1?(void)0:qDebug
#    endif
#else
#    include <AGL/agl.h>
#    include <AGL/aglRenderers.h>
#    include <OpenGL/gl.h>
#endif
#else
#include <private/qcocoaview_mac_p.h>
#endif


#include <OpenGL/gl.h>
#include <CoreServices/CoreServices.h>
#include <private/qfont_p.h>
#include <private/qfontengine_p.h>
#include <private/qgl_p.h>
#include <private/qpaintengine_opengl_p.h>
#include <private/qt_mac_p.h>
#include <qpixmap.h>
#include <qtimer.h>
#include <qapplication.h>
#include <qstack.h>
#include <qdesktopwidget.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE
#ifdef QT_MAC_USE_COCOA
QT_END_NAMESPACE

QT_FORWARD_DECLARE_CLASS(QWidget)
QT_FORWARD_DECLARE_CLASS(QWidgetPrivate)
QT_FORWARD_DECLARE_CLASS(QGLWidgetPrivate)

QT_BEGIN_NAMESPACE

void *qt_current_nsopengl_context()
{
    return [NSOpenGLContext currentContext];
}

static GLint attribValue(NSOpenGLPixelFormat *fmt, NSOpenGLPixelFormatAttribute attrib)
{
    GLint res;
    [fmt getValues:&res forAttribute:attrib forVirtualScreen:0];
    return res;
}

static int def(int val, int defVal)
{
    return val != -1 ? val : defVal;
}
#else
QRegion qt_mac_get_widget_rgn(const QWidget *widget);
#endif

extern quint32 *qt_mac_pixmap_get_base(const QPixmap *);
extern int qt_mac_pixmap_get_bytes_per_line(const QPixmap *);
extern RgnHandle qt_mac_get_rgn(); //qregion_mac.cpp
extern void qt_mac_dispose_rgn(RgnHandle); //qregion_mac.cpp
extern QRegion qt_mac_convert_mac_region(RgnHandle); //qregion_mac.cpp
extern void qt_mac_to_pascal_string(QString s, Str255 str, TextEncoding encoding=0, int len=-1);  //qglobal.cpp

/*
    QGLTemporaryContext implementation
*/

class QGLTemporaryContextPrivate
{
public:
#ifndef QT_MAC_USE_COCOA
    AGLContext ctx;
#else
    NSOpenGLContext *ctx;
#endif
};

QGLTemporaryContext::QGLTemporaryContext(bool, QWidget *)
    : d(new QGLTemporaryContextPrivate)
{
    d->ctx = 0;
#ifndef QT_MAC_USE_COCOA
    GLint attribs[] = {AGL_RGBA, AGL_NONE};
    AGLPixelFormat fmt = aglChoosePixelFormat(0, 0, attribs);
    if (!fmt) {
        qDebug("QGLTemporaryContext: Couldn't find any RGB visuals");
        return;
    }
    d->ctx = aglCreateContext(fmt, 0);
    if (!d->ctx)
        qDebug("QGLTemporaryContext: Unable to create context");
    else
        aglSetCurrentContext(d->ctx);
    aglDestroyPixelFormat(fmt);
#else
    QMacCocoaAutoReleasePool pool;
    NSOpenGLPixelFormatAttribute attribs[] = { 0 };
    NSOpenGLPixelFormat *fmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
    if (!fmt) {
        qWarning("QGLTemporaryContext: Cannot find any visuals");
        return;
    }

    d->ctx = [[NSOpenGLContext alloc] initWithFormat:fmt shareContext:0];
    if (!d->ctx)
        qWarning("QGLTemporaryContext: Cannot create context");
    else
        [d->ctx makeCurrentContext];
    [fmt release];
#endif
}

QGLTemporaryContext::~QGLTemporaryContext()
{
    if (d->ctx) {
#ifndef QT_MAC_USE_COCOA
        aglSetCurrentContext(0);
        aglDestroyContext(d->ctx);
#else
        [NSOpenGLContext clearCurrentContext];
        [d->ctx release];
#endif
    }
}

bool QGLFormat::hasOpenGL()
{
    return true;
}

bool QGLFormat::hasOpenGLOverlays()
{
    return false;
}

bool QGLContext::chooseContext(const QGLContext *shareContext)
{
    QMacCocoaAutoReleasePool pool;
    Q_D(QGLContext);
    d->cx = 0;
    d->vi = chooseMacVisual(0);
    if (!d->vi)
        return false;

#ifndef QT_MAC_USE_COCOA
    AGLPixelFormat fmt = (AGLPixelFormat)d->vi;
    GLint res;
    aglDescribePixelFormat(fmt, AGL_LEVEL, &res);
    d->glFormat.setPlane(res);
    if (deviceIsPixmap())
        res = 0;
    else
        aglDescribePixelFormat(fmt, AGL_DOUBLEBUFFER, &res);
    d->glFormat.setDoubleBuffer(res);
    aglDescribePixelFormat(fmt, AGL_DEPTH_SIZE, &res);
    d->glFormat.setDepth(res);
    if (d->glFormat.depth())
        d->glFormat.setDepthBufferSize(res);
    aglDescribePixelFormat(fmt, AGL_RGBA, &res);
    d->glFormat.setRgba(res);
    aglDescribePixelFormat(fmt, AGL_RED_SIZE, &res);
    d->glFormat.setRedBufferSize(res);
    aglDescribePixelFormat(fmt, AGL_GREEN_SIZE, &res);
    d->glFormat.setGreenBufferSize(res);
    aglDescribePixelFormat(fmt, AGL_BLUE_SIZE, &res);
    d->glFormat.setBlueBufferSize(res);
    aglDescribePixelFormat(fmt, AGL_ALPHA_SIZE, &res);
    d->glFormat.setAlpha(res);
    if (d->glFormat.alpha())
        d->glFormat.setAlphaBufferSize(res);
    aglDescribePixelFormat(fmt, AGL_ACCUM_RED_SIZE, &res);
    // Bug in Apple OpenGL (rdr://5015603), when we don't have an accumulation
    // buffer, it still claims that we have a 16-bit one (which is pretty rare).
    // So, we just assume we can never have a buffer that small.
    d->glFormat.setAccum(res > 5);
    if (d->glFormat.accum())
        d->glFormat.setAccumBufferSize(res);
    aglDescribePixelFormat(fmt, AGL_STENCIL_SIZE, &res);
    d->glFormat.setStencil(res);
    if (d->glFormat.stencil())
        d->glFormat.setStencilBufferSize(res);
    aglDescribePixelFormat(fmt, AGL_STEREO, &res);
    d->glFormat.setStereo(res);
    aglDescribePixelFormat(fmt, AGL_SAMPLE_BUFFERS_ARB, &res);
    d->glFormat.setSampleBuffers(res);
    if (d->glFormat.sampleBuffers()) {
        aglDescribePixelFormat(fmt, AGL_SAMPLES_ARB, &res);
        d->glFormat.setSamples(res);
    }
#else
    NSOpenGLPixelFormat *fmt = static_cast<NSOpenGLPixelFormat *>(d->vi);

    d->glFormat = QGLFormat();

    // ### make sure to reset other options
    d->glFormat.setDoubleBuffer(attribValue(fmt, NSOpenGLPFADoubleBuffer));

    int depthSize = attribValue(fmt, NSOpenGLPFADepthSize);
    d->glFormat.setDepth(depthSize > 0);
    if (depthSize > 0)
        d->glFormat.setDepthBufferSize(depthSize);

    int alphaSize = attribValue(fmt, NSOpenGLPFAAlphaSize);
    d->glFormat.setAlpha(alphaSize > 0);
    if (alphaSize > 0)
        d->glFormat.setAlphaBufferSize(alphaSize);

    int accumSize = attribValue(fmt, NSOpenGLPFAAccumSize);
    d->glFormat.setAccum(accumSize > 0);
    if (accumSize > 0)
        d->glFormat.setAccumBufferSize(accumSize);

    int stencilSize = attribValue(fmt, NSOpenGLPFAStencilSize);
    d->glFormat.setStencil(stencilSize > 0);
    if (stencilSize > 0)
        d->glFormat.setStencilBufferSize(stencilSize);

    d->glFormat.setStereo(attribValue(fmt, NSOpenGLPFAStereo));

    int sampleBuffers = attribValue(fmt, NSOpenGLPFASampleBuffers);
    d->glFormat.setSampleBuffers(sampleBuffers);
    if (sampleBuffers > 0)
        d->glFormat.setSamples(attribValue(fmt, NSOpenGLPFASamples));
#endif
    if (shareContext && (!shareContext->isValid() || !shareContext->d_func()->cx)) {
        qWarning("QGLContext::chooseContext: Cannot share with invalid context");
        shareContext = 0;
    }

    // sharing between rgba and color-index will give wrong colors
    if (shareContext && (format().rgba() != shareContext->format().rgba()))
        shareContext = 0;

#ifndef QT_MAC_USE_COCOA
    AGLContext ctx = aglCreateContext(fmt, (AGLContext) (shareContext ? shareContext->d_func()->cx : 0));
#else
    NSOpenGLContext *ctx = [[NSOpenGLContext alloc] initWithFormat:fmt
        shareContext:(shareContext ? static_cast<NSOpenGLContext *>(shareContext->d_func()->cx)
                                   : 0)];
#endif
    if (!ctx) {
#ifndef QT_MAC_USE_COCOA
        GLenum err = aglGetError();
        if (err == AGL_BAD_MATCH || err == AGL_BAD_CONTEXT) {
            if (shareContext && shareContext->d_func()->cx) {
                qWarning("QGLContext::chooseContext(): Context sharing mismatch!");
                if (!(ctx = aglCreateContext(fmt, 0)))
                    return false;
                shareContext = 0;
            }
        }
#else
        if (shareContext) {
            ctx = [[NSOpenGLContext alloc] initWithFormat:fmt shareContext:0];
            if (ctx) {
                qWarning("QGLContext::chooseContext: Context sharing mismatch");
                shareContext = 0;
            }
        }
#endif
        if (!ctx) {
            qWarning("QGLContext::chooseContext: Unable to create QGLContext");
            return false;
        }
    }
    d->cx = ctx;
    if (shareContext && shareContext->d_func()->cx) {
        QGLContext *share = const_cast<QGLContext *>(shareContext);
        d->sharing = true;
        share->d_func()->sharing = true;
    }
    if (deviceIsPixmap())
        updatePaintDevice();

    // vblank syncing
    GLint interval = d->reqFormat.swapInterval();
    if (interval != -1) {
#ifndef QT_MAC_USE_COCOA
        aglSetInteger((AGLContext)d->cx, AGL_SWAP_INTERVAL, &interval);
        if (interval != 0)
            aglEnable((AGLContext)d->cx, AGL_SWAP_INTERVAL);
        else
            aglDisable((AGLContext)d->cx, AGL_SWAP_INTERVAL);
#else
        [ctx setValues:&interval forParameter:NSOpenGLCPSwapInterval];
#endif
    }
#ifndef QT_MAC_USE_COCOA
    aglGetInteger((AGLContext)d->cx, AGL_SWAP_INTERVAL, &interval);
#else
    [ctx getValues:&interval forParameter:NSOpenGLCPSwapInterval];
#endif
    d->glFormat.setSwapInterval(interval);
    return true;
}

void *QGLContextPrivate::tryFormat(const QGLFormat &format)
{
    static const int Max = 40;
#ifndef QT_MAC_USE_COCOA
    GLint attribs[Max], cnt = 0;
    bool device_is_pixmap = (paintDevice->devType() == QInternal::Pixmap);

    attribs[cnt++] = AGL_RGBA;
    attribs[cnt++] = AGL_BUFFER_SIZE;
    attribs[cnt++] = device_is_pixmap ? static_cast<QPixmap *>(paintDevice)->depth() : 32;
    attribs[cnt++] = AGL_LEVEL;
    attribs[cnt++] = format.plane();

    if (format.redBufferSize() != -1) {
        attribs[cnt++] = AGL_RED_SIZE;
        attribs[cnt++] = format.redBufferSize();
    }
    if (format.greenBufferSize() != -1) {
        attribs[cnt++] = AGL_GREEN_SIZE;
        attribs[cnt++] = format.greenBufferSize();
    }
    if (format.blueBufferSize() != -1) {
        attribs[cnt++] = AGL_BLUE_SIZE;
        attribs[cnt++] = format.blueBufferSize();
    }
    if (device_is_pixmap) {
        attribs[cnt++] = AGL_PIXEL_SIZE;
        attribs[cnt++] = static_cast<QPixmap *>(paintDevice)->depth();
        attribs[cnt++] = AGL_OFFSCREEN;
        if (!format.alpha()) {
            attribs[cnt++] = AGL_ALPHA_SIZE;
            attribs[cnt++] = 8;
        }
    } else {
        if (format.doubleBuffer())
            attribs[cnt++] = AGL_DOUBLEBUFFER;
    }

    if (format.stereo())
        attribs[cnt++] = AGL_STEREO;
    if (format.alpha()) {
        attribs[cnt++] = AGL_ALPHA_SIZE;
        attribs[cnt++] = format.alphaBufferSize() == -1 ? 8 : format.alphaBufferSize();
    }
    if (format.stencil()) {
        attribs[cnt++] = AGL_STENCIL_SIZE;
        attribs[cnt++] = format.stencilBufferSize() == -1 ? 8 : format.stencilBufferSize();
    }
    if (format.depth()) {
        attribs[cnt++] = AGL_DEPTH_SIZE;
        attribs[cnt++] = format.depthBufferSize() == -1 ? 32 : format.depthBufferSize();
    }
    if (format.accum()) {
        attribs[cnt++] = AGL_ACCUM_RED_SIZE;
        attribs[cnt++] = format.accumBufferSize() == -1 ? 1 : format.accumBufferSize();
        attribs[cnt++] = AGL_ACCUM_BLUE_SIZE;
        attribs[cnt++] = format.accumBufferSize() == -1 ? 1 : format.accumBufferSize();
        attribs[cnt++] = AGL_ACCUM_GREEN_SIZE;
        attribs[cnt++] = format.accumBufferSize() == -1 ? 1 : format.accumBufferSize();
        attribs[cnt++] = AGL_ACCUM_ALPHA_SIZE;
        attribs[cnt++] = format.accumBufferSize() == -1 ? 1 : format.accumBufferSize();
    }
    if (format.sampleBuffers()) {
        attribs[cnt++] = AGL_SAMPLE_BUFFERS_ARB;
        attribs[cnt++] = 1;
        attribs[cnt++] = AGL_SAMPLES_ARB;
        attribs[cnt++] = format.samples() == -1 ? 4 : format.samples();
    }

    attribs[cnt] = AGL_NONE;
    Q_ASSERT(cnt < Max);
    return aglChoosePixelFormat(0, 0, attribs);
#else
    NSOpenGLPixelFormatAttribute attribs[Max];
    int cnt = 0;
    int devType = paintDevice->devType();
    bool device_is_pixmap = (devType == QInternal::Pixmap);
    int depth = device_is_pixmap ? static_cast<QPixmap *>(paintDevice)->depth() : 32;

    attribs[cnt++] = NSOpenGLPFAColorSize;
    attribs[cnt++] = depth;

    if (device_is_pixmap) {
        attribs[cnt++] = NSOpenGLPFAOffScreen;
    } else {
        if (format.doubleBuffer())
            attribs[cnt++] = NSOpenGLPFADoubleBuffer;
    }
    if (glFormat.stereo())
        attribs[cnt++] = NSOpenGLPFAStereo;
    if (device_is_pixmap || format.alpha()) {
        attribs[cnt++] = NSOpenGLPFAAlphaSize;
        attribs[cnt++] = def(format.alphaBufferSize(), 8);
    }
    if (format.stencil()) {
        attribs[cnt++] = NSOpenGLPFAStencilSize;
        attribs[cnt++] = def(format.stencilBufferSize(), 8);
    }
    if (format.depth()) {
        attribs[cnt++] = NSOpenGLPFADepthSize;
        attribs[cnt++] = def(format.depthBufferSize(), 32);
    }
    if (format.accum()) {
        attribs[cnt++] = NSOpenGLPFAAccumSize;
        attribs[cnt++] = def(format.accumBufferSize(), 1);
    }
    if (format.sampleBuffers()) {
        attribs[cnt++] = NSOpenGLPFASampleBuffers;
        attribs[cnt++] = 1;
        attribs[cnt++] = NSOpenGLPFASamples;
        attribs[cnt++] = def(format.samples(), 4);
    }

    if (format.directRendering())
        attribs[cnt++] = NSOpenGLPFAAccelerated;

    if (devType == QInternal::Pbuffer)
        attribs[cnt++] = NSOpenGLPFAPixelBuffer;

    attribs[cnt] = 0;
    Q_ASSERT(cnt < Max);
    return [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
#endif
}

void QGLContextPrivate::clearDrawable()
{
    [static_cast<NSOpenGLContext *>(cx) clearDrawable];
}

/*!
    \bold{Mac OS X only:} This virtual function tries to find a visual that
    matches the format, reducing the demands if the original request
    cannot be met.

    The algorithm for reducing the demands of the format is quite
    simple-minded, so override this method in your subclass if your
    application has spcific requirements on visual selection.

    The \a handle argument is always zero and is not used

    \sa chooseContext()
*/

void *QGLContext::chooseMacVisual(GDHandle /* handle */)
{
    Q_D(QGLContext);

    void *fmt = d->tryFormat(d->glFormat);
    if (!fmt && d->glFormat.stereo()) {
        d->glFormat.setStereo(false);
        fmt = d->tryFormat(d->glFormat);
    }
    if (!fmt && d->glFormat.sampleBuffers()) {
        d->glFormat.setSampleBuffers(false);
        fmt = d->tryFormat(d->glFormat);
    }
    if (!fmt)
        qWarning("QGLContext::chooseMacVisual: Unable to choose a pixel format");
    return fmt;
}

void QGLContext::reset()
{
    Q_D(QGLContext);
    if (!d->valid)
        return;
    d->cleanup();
    doneCurrent();
#ifndef QT_MAC_USE_COCOA
    if (d->cx)
        aglDestroyContext((AGLContext)d->cx);
#else
    QMacCocoaAutoReleasePool pool;
    [static_cast<NSOpenGLContext *>(d->cx) release];
#endif
    d->cx = 0;
#ifndef QT_MAC_USE_COCOA
    if (d->vi)
        aglDestroyPixelFormat((AGLPixelFormat)d->vi);
#else
    [static_cast<NSOpenGLPixelFormat *>(d->vi) release];
#endif
    d->vi = 0;
    d->crWin = false;
    d->sharing = false;
    d->valid = false;
    d->transpColor = QColor();
    d->initDone = false;
    QGLContextGroup::removeShare(this);
}

void QGLContext::makeCurrent()
{
    Q_D(QGLContext);

    if (!d->valid) {
        qWarning("QGLContext::makeCurrent: Cannot make invalid context current");
        return;
    }
#ifndef QT_MAC_USE_COCOA
    aglSetCurrentContext((AGLContext)d->cx);
    if (d->update)
        updatePaintDevice();
#else
    [static_cast<NSOpenGLContext *>(d->cx) makeCurrentContext];
#endif
    QGLContextPrivate::setCurrentContext(this);
}

#ifndef QT_MAC_USE_COCOA
/*
    Returns the effective scale factor for a widget. For this value to be
    different than 1, the following must be true:
    - The system scale factor must be greater than 1.
    - The widget window must have WA_MacFrameworkScaled set.
*/
float qt_mac_get_scale_factor(QWidget *widget)
{
    if (!widget | !widget->window())
        return 1;

    if (widget->window()->testAttribute(Qt::WA_MacFrameworkScaled) == false)
        return 1;

    float systemScale = QSysInfo::MacintoshVersion >= QSysInfo::MV_10_4 ? HIGetScaleFactor() : 1;
    if (systemScale == float(1))
        return 1;

    return systemScale;
}
#endif

/*! \internal
*/
void QGLContext::updatePaintDevice()
{
    Q_D(QGLContext);
#ifndef QT_MAC_USE_COCOA
    d->update = false;
    if (d->paintDevice->devType() == QInternal::Widget) {
        //get control information
        QWidget *w = (QWidget *)d->paintDevice;
        HIViewRef hiview = (HIViewRef)w->winId();
        WindowRef window = HIViewGetWindow(hiview);
#ifdef DEBUG_OPENGL_REGION_UPDATE
        static int serial_no_gl = 0;
        qDebug("[%d] %p setting on %s::%s %p/%p [%s]", ++serial_no_gl, w,
                w->metaObject()->className(), w->objectName().toLatin1().constData(),
                hiview, window, w->handle() ? "Inside" : "Outside");
#endif

        //update drawable
        if (0 && w->isWindow() && w->isFullScreen()) {
            aglSetDrawable((AGLContext)d->cx, 0);
            aglSetFullScreen((AGLContext)d->cx, w->width(), w->height(), 0, QApplication::desktop()->screenNumber(w));
            w->hide();
        } else {
            AGLDrawable old_draw = aglGetDrawable((AGLContext)d->cx), new_draw = GetWindowPort(window);
            if (old_draw != new_draw)
                aglSetDrawable((AGLContext)d->cx, new_draw);
        }

        float scale  = qt_mac_get_scale_factor(w);

        if (!w->isWindow()) {
            QRegion clp = qt_mac_get_widget_rgn(w); //get drawable area

#ifdef DEBUG_OPENGL_REGION_UPDATE
            if (clp.isEmpty()) {
                qDebug("  Empty area!");
            } else {
                QVector<QRect> rs = clp.rects();
                for(int i = 0; i < rs.count(); i++)
                    qDebug("  %d %d %d %d", rs[i].x(), rs[i].y(), rs[i].width(), rs[i].height());
            }
#endif
            //update the clip
            if (!aglIsEnabled((AGLContext)d->cx, AGL_BUFFER_RECT))
                aglEnable((AGLContext)d->cx, AGL_BUFFER_RECT);
            if (clp.isEmpty()) {
                GLint offs[4] = { 0, 0, 0, 0 };
                aglSetInteger((AGLContext)d->cx, AGL_BUFFER_RECT, offs);
                if (aglIsEnabled((AGLContext)d->cx, AGL_CLIP_REGION))
                    aglDisable((AGLContext)d->cx, AGL_CLIP_REGION);
            } else {
                HIPoint origin = { 0., 0. };
                HIViewConvertPoint(&origin, HIViewRef(w->winId()), 0);
                const GLint offs[4] = { qRound(origin.x),
                    w->window()->frameGeometry().height() * scale
                        - (qRound(origin.y) + w->height() * scale),
                    w->width() * scale, w->height() * scale};

                RgnHandle region = clp.handle(true);

                if (scale != float(1)) {
                    // Sacle the clip region by the scale factor
                    Rect regionBounds;
                    GetRegionBounds(region, &regionBounds);
                    Rect regionBoundsDest = regionBounds;
                    regionBoundsDest.bottom *= scale;
                    regionBoundsDest.right *= scale;
                    MapRgn(region, &regionBounds, &regionBoundsDest);
                }

                aglSetInteger((AGLContext)d->cx, AGL_BUFFER_RECT, offs);
                aglSetInteger((AGLContext)d->cx, AGL_CLIP_REGION, (const GLint *)region);
                if (!aglIsEnabled((AGLContext)d->cx, AGL_CLIP_REGION))
                    aglEnable((AGLContext)d->cx, AGL_CLIP_REGION);
            }
        } else {
            // Set the buffer rect for top-level gl contexts when scaled.
            if (scale != float(1)) {
                aglEnable((AGLContext)d->cx, AGL_BUFFER_RECT);
                const GLint offs[4] = { 0, 0,  w->width() * scale , w->height() * scale};
                aglSetInteger((AGLContext)d->cx, AGL_BUFFER_RECT, offs);
            }
        }
    } else if (d->paintDevice->devType() == QInternal::Pixmap) {
        QPixmap *pm = reinterpret_cast<QPixmap *>(d->paintDevice);

        unsigned long qdformat = k32ARGBPixelFormat;
        if (QSysInfo::ByteOrder == QSysInfo::LittleEndian)
            qdformat = k32BGRAPixelFormat;
        Rect rect;
        SetRect(&rect, 0, 0, pm->width(), pm->height());

        GWorldPtr gworld;
        NewGWorldFromPtr(&gworld, qdformat, &rect, 0, 0, 0,
                         reinterpret_cast<char *>(qt_mac_pixmap_get_base(pm)), 
                         qt_mac_pixmap_get_bytes_per_line(pm));

        PixMapHandle pixmapHandle = GetGWorldPixMap(gworld);
        aglSetOffScreen(reinterpret_cast<AGLContext>(d->cx), pm->width(), pm->height(),
                        GetPixRowBytes(pixmapHandle), GetPixBaseAddr(pixmapHandle));
    } else {
        qWarning("QGLContext::updatePaintDevice(): Not sure how to render OpenGL on this device!");
    }
    aglUpdateContext((AGLContext)d->cx);

#else
    QMacCocoaAutoReleasePool pool;

    if (d->paintDevice->devType() == QInternal::Widget) {
        //get control information
        QWidget *w = (QWidget *)d->paintDevice;
        NSView *view = qt_mac_nativeview_for(w);

        // Trying to attach the GL context to the NSView will fail with
        // "invalid drawable" if done too soon, but we have to make sure
        // the connection is made before the first paint event. Using
        // the NSView do to this check fails as the NSView is visible
        // before it's safe to connect, and using the NSWindow fails as
        // the NSWindow will become visible after the first paint event.
        // This leaves us with the QWidget, who's visible state seems
        // to match the point in time when it's safe to connect.
        if (!w || !w->isVisible())
            return; // Not safe to attach GL context to view yet

        if ([static_cast<NSOpenGLContext *>(d->cx) view] != view && ![view isHidden])
            [static_cast<NSOpenGLContext *>(d->cx) setView:view];
    } else if (d->paintDevice->devType() == QInternal::Pixmap) {
        const QPixmap *pm = static_cast<const QPixmap *>(d->paintDevice);
        [static_cast<NSOpenGLContext *>(d->cx) setOffScreen:qt_mac_pixmap_get_base(pm)
                                                      width:pm->width()
                                                     height:pm->height()
                                                   rowbytes:qt_mac_pixmap_get_bytes_per_line(pm)];
    } else {
        qWarning("QGLContext::updatePaintDevice: Not sure how to render OpenGL on this device");
    }
    [static_cast<NSOpenGLContext *>(d->cx) update];
#endif
}

void QGLContext::doneCurrent()
{

    if (
#ifndef QT_MAC_USE_COCOA
        aglGetCurrentContext() != (AGLContext) d_func()->cx
#else
        [NSOpenGLContext currentContext] != d_func()->cx
#endif
       )
        return;

    QGLContextPrivate::setCurrentContext(0);
#ifndef QT_MAC_USE_COCOA
    aglSetCurrentContext(0);
#else
    [NSOpenGLContext clearCurrentContext];
#endif
}

void QGLContext::swapBuffers() const
{
    Q_D(const QGLContext);
    if (!d->valid)
        return;
#ifndef QT_MAC_USE_COCOA
    aglSwapBuffers((AGLContext)d->cx);
#else
    [static_cast<NSOpenGLContext *>(d->cx) flushBuffer];
#endif
}

QColor QGLContext::overlayTransparentColor() const
{
    return QColor(0, 0, 0);                // Invalid color
}

#ifndef QT_MAC_USE_COCOA
static QColor cmap[256];
static bool cmap_init = false;
#endif
uint QGLContext::colorIndex(const QColor &c) const
{
#ifndef QT_MAC_USE_COCOA
    int ret = -1;
    if(!cmap_init) {
        cmap_init = true;
        for(int i = 0; i < 256; i++)
            cmap[i] = QColor();
    } else {
        for(int i = 0; i < 256; i++) {
            if(cmap[i].isValid() && cmap[i] == c) {
                ret = i;
                break;
            }
        }
    }
    if(ret == -1) {
        for(ret = 0; ret < 256; ret++)
            if(!cmap[ret].isValid())
                break;
        if(ret == 256) {
            ret = -1;
            qWarning("QGLContext::colorIndex(): Internal error!");
        } else {
            cmap[ret] = c;

            GLint vals[4];
            vals[0] = ret;
            vals[1] = c.red();
            vals[2] = c.green();
            vals[3] = c.blue();
            aglSetInteger((AGLContext)d_func()->cx, AGL_COLORMAP_ENTRY, vals);
        }
    }
    return (uint)(ret == -1 ? 0 : ret);
#else
    Q_UNUSED(c);
    return 0;
#endif
}

void QGLContext::generateFontDisplayLists(const QFont & /* fnt */, int /* listBase */)
{
}

static CFBundleRef qt_getOpenGLBundle()
{
    CFBundleRef bundle = 0;
    CFStringRef urlString = QCFString::toCFStringRef(QLatin1String("/System/Library/Frameworks/OpenGL.framework"));
    QCFType<CFURLRef> url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
                 urlString, kCFURLPOSIXPathStyle, false);
    if (url)
        bundle = CFBundleCreate(kCFAllocatorDefault, url);
    CFRelease(urlString);
    return bundle;
}

void *QGLContext::getProcAddress(const QString &proc) const
{
    CFStringRef procName = QCFString(proc).toCFStringRef(proc);
    void *result = CFBundleGetFunctionPointerForName(QCFType<CFBundleRef>(qt_getOpenGLBundle()),
                                             procName);
    CFRelease(procName);
    return result;
}
#ifndef QT_MAC_USE_COCOA
/*****************************************************************************
  QGLWidget AGL-specific code
 *****************************************************************************/

/****************************************************************************
  Hacks to glue AGL to an HIView
 ***************************************************************************/
QRegion qt_mac_get_widget_rgn(const QWidget *widget)
{
    if(!widget->isVisible() || widget->isMinimized())
        return QRegion();
    const QRect wrect = QRect(qt_mac_posInWindow(widget), widget->size());
    if(!wrect.isValid())
        return QRegion();

    RgnHandle macr = qt_mac_get_rgn();
    GetControlRegion((HIViewRef)widget->winId(), kControlStructureMetaPart, macr);
    OffsetRgn(macr, wrect.x(), wrect.y());
    QRegion ret = qt_mac_convert_mac_region(macr);

    QPoint clip_pos = wrect.topLeft();
    for(const QWidget *last_clip = 0, *clip = widget; clip; last_clip = clip, clip = clip->parentWidget()) {
        if(clip != widget) {
            GetControlRegion((HIViewRef)clip->winId(), kControlStructureMetaPart, macr);
            OffsetRgn(macr, clip_pos.x(), clip_pos.y());
            ret &= qt_mac_convert_mac_region(macr);
        }
        const QObjectList &children = clip->children();
        for(int i = children.size()-1; i >= 0; --i) {
            if(QWidget *child = qobject_cast<QWidget*>(children.at(i))) {
                if(child == last_clip)
                    break;

                // This check may seem weird, but when we are using a unified toolbar
                // The widget is actually being owned by that toolbar and not by Qt.
                // This means that the geometry it reports will be wrong
                // and will accidentally cause problems when calculating the region
                // So, it is better to skip these widgets since they aren't the hierarchy
                // anyway.
                if (HIViewGetSuperview(HIViewRef(child->winId())) != HIViewRef(clip->winId()))
                    continue;

                if(child->isVisible() && !child->isMinimized() && !child->isTopLevel()) {
                    const QRect childRect = QRect(clip_pos+child->pos(), child->size());
                    if(childRect.isValid() && wrect.intersects(childRect)) {
                        GetControlRegion((HIViewRef)child->winId(), kControlStructureMetaPart, macr);
                        OffsetRgn(macr, childRect.x(), childRect.y());
                        ret -= qt_mac_convert_mac_region(macr);
                    }
                }
            }
        }
        if(clip->isWindow())
            break;
        clip_pos -= clip->pos();
    }
    qt_mac_dispose_rgn(macr);
    return ret;
}

#endif

void QGLWidget::setMouseTracking(bool enable)
{
    QWidget::setMouseTracking(enable);
}

void QGLWidget::resizeEvent(QResizeEvent *)
{
    Q_D(QGLWidget);
    if (!isValid())
        return;
#ifndef QT_MAC_USE_COCOA
    if (!isWindow())
        d->glcx->d_func()->update = true;
#endif
    makeCurrent();
    if (!d->glcx->initialized())
        glInit();
#ifdef QT_MAC_USE_COCOA
    d->glcx->updatePaintDevice();
#endif
#ifndef QT_MAC_USE_COCOA
    float scale  = qt_mac_get_scale_factor(this);
    resizeGL(width() * scale, height() * scale);
#else
    resizeGL(width(), height());
#endif
}

const QGLContext* QGLWidget::overlayContext() const
{
    return 0;
}

void QGLWidget::makeOverlayCurrent()
{
}

void QGLWidget::updateOverlayGL()
{
}

void QGLWidget::setContext(QGLContext *context, const QGLContext* shareContext, bool deleteOldContext)
{
    Q_D(QGLWidget);
    if (context == 0) {
        qWarning("QGLWidget::setContext: Cannot set null context");
        return;
    }

    if (d->glcx)
        d->glcx->doneCurrent();
    QGLContext* oldcx = d->glcx;
    d->glcx = context;
    if (!d->glcx->isValid())
        d->glcx->create(shareContext ? shareContext : oldcx);
    if (deleteOldContext && oldcx)
        delete oldcx;
}

void QGLWidgetPrivate::init(QGLContext *context, const QGLWidget *shareWidget)
{
    Q_Q(QGLWidget);

    initContext(context, shareWidget);

    QWidget *current = q;
    while (current) {
        qt_widget_private(current)->glWidgets.append(QWidgetPrivate::GlWidgetInfo(q));
        if (current->isWindow())
            break;
        current = current->parentWidget();
    }
}

bool QGLWidgetPrivate::renderCxPm(QPixmap*)
{
    return false;
}

void QGLWidgetPrivate::cleanupColormaps()
{
}

const QGLColormap & QGLWidget::colormap() const
{
    return d_func()->cmap;
}

void QGLWidget::setColormap(const QGLColormap &)
{
}

void QGLWidgetPrivate::updatePaintDevice()
{
    Q_Q(QGLWidget);
    glcx->updatePaintDevice();
    q->update();
}

#endif

QT_END_NAMESPACE
