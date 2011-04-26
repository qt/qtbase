/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QGL_P_H
#define QGL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QGLWidget class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "QtOpenGL/qgl.h"
#include "QtOpenGL/qglcolormap.h"
#include "QtCore/qmap.h"
#include "QtCore/qthread.h"
#include "QtCore/qthreadstorage.h"
#include "QtCore/qhash.h"
#include "QtCore/qatomic.h"
#include "private/qwidget_p.h"
#include "qcache.h"
#include "qglpaintdevice_p.h"

#ifndef QT_NO_EGL
#include <QtGui/private/qegl_p.h>
#endif

#if defined(Q_WS_QPA)
#include <QtGui/QWindowContext>
#endif

QT_BEGIN_NAMESPACE

class QGLContext;
class QGLOverlayWidget;
class QPixmap;
class QPixmapFilter;
#ifdef Q_WS_MAC
# ifdef qDebug
#   define old_qDebug qDebug
#   undef qDebug
# endif
QT_BEGIN_INCLUDE_NAMESPACE
#ifndef QT_MAC_USE_COCOA
# include <AGL/agl.h>
#endif
QT_END_INCLUDE_NAMESPACE
# ifdef old_qDebug
#   undef qDebug
#   define qDebug QT_NO_QDEBUG_MACRO
#   undef old_qDebug
# endif
class QMacWindowChangeEvent;
#endif

#ifdef Q_WS_QWS
class QWSGLWindowSurface;
#endif

#ifndef QT_NO_EGL
class QEglContext;
#endif

QT_BEGIN_INCLUDE_NAMESPACE
#include <QtOpenGL/private/qglextensions_p.h>
QT_END_INCLUDE_NAMESPACE

class QGLFormatPrivate
{
public:
    QGLFormatPrivate()
        : ref(1)
    {
        opts = QGL::DoubleBuffer | QGL::DepthBuffer | QGL::Rgba | QGL::DirectRendering
             | QGL::StencilBuffer | QGL::DeprecatedFunctions;
        pln = 0;
        depthSize = accumSize = stencilSize = redSize = greenSize = blueSize = alphaSize = -1;
        numSamples = -1;
        swapInterval = -1;
        majorVersion = 1;
        minorVersion = 0;
        profile = QGLFormat::NoProfile;
    }
    QGLFormatPrivate(const QGLFormatPrivate *other)
        : ref(1),
          opts(other->opts),
          pln(other->pln),
          depthSize(other->depthSize),
          accumSize(other->accumSize),
          stencilSize(other->stencilSize),
          redSize(other->redSize),
          greenSize(other->greenSize),
          blueSize(other->blueSize),
          alphaSize(other->alphaSize),
          numSamples(other->numSamples),
          swapInterval(other->swapInterval),
          majorVersion(other->majorVersion),
          minorVersion(other->minorVersion),
          profile(other->profile)
    {
    }
    QAtomicInt ref;
    QGL::FormatOptions opts;
    int pln;
    int depthSize;
    int accumSize;
    int stencilSize;
    int redSize;
    int greenSize;
    int blueSize;
    int alphaSize;
    int numSamples;
    int swapInterval;
    int majorVersion;
    int minorVersion;
    QGLFormat::OpenGLContextProfile profile;
};

class QGLWidgetPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QGLWidget)
public:
    QGLWidgetPrivate() : QWidgetPrivate()
                       , disable_clear_on_painter_begin(false)
#if defined(Q_WS_QWS)
                       , wsurf(0)
#endif
#if defined(Q_WS_X11) && !defined(QT_NO_EGL)
                       , eglSurfaceWindowId(0)
#endif
#if defined(Q_OS_SYMBIAN)
                       , eglSurfaceWindowId(0)
#endif
    {
        isGLWidget = 1;
    }

    ~QGLWidgetPrivate() {}

    void init(QGLContext *context, const QGLWidget* shareWidget);
    void initContext(QGLContext *context, const QGLWidget* shareWidget);
    bool renderCxPm(QPixmap *pixmap);
    void cleanupColormaps();
    void aboutToDestroy() {
        if (glcx)
            glcx->reset();
    }

    QGLContext *glcx;
    QGLWidgetGLPaintDevice glDevice;
    bool autoSwap;

    QGLColormap cmap;
#ifndef QT_OPENGL_ES
    QMap<QString, int> displayListCache;
#endif

    bool disable_clear_on_painter_begin;

#if defined(Q_WS_WIN)
    void updateColormap();
    QGLContext *olcx;
#elif defined(Q_WS_X11)
    QGLOverlayWidget *olw;
#ifndef QT_NO_EGL
    void recreateEglSurface();
    WId eglSurfaceWindowId;
#endif
#elif defined(Q_WS_MAC)
    QGLContext *olcx;
    void updatePaintDevice();
#elif defined(Q_WS_QWS)
    QWSGLWindowSurface *wsurf;
#endif
#ifdef Q_OS_SYMBIAN
    void recreateEglSurface();
    WId eglSurfaceWindowId;
#endif
};

class QGLContextGroupResourceBase;
class QGLSharedResourceGuard;

// QGLContextPrivate has the responsibility of creating context groups.
// QGLContextPrivate maintains the reference counter and destroys
// context groups when needed.
class QGLContextGroup
{
public:
    ~QGLContextGroup();

    QGLExtensionFuncs &extensionFuncs() {return m_extensionFuncs;}
    const QGLContext *context() const {return m_context;}
    bool isSharing() const { return m_shares.size() >= 2; }
    QList<const QGLContext *> shares() const { return m_shares; }

    void addGuard(QGLSharedResourceGuard *guard);
    void removeGuard(QGLSharedResourceGuard *guard);

    static void addShare(const QGLContext *context, const QGLContext *share);
    static void removeShare(const QGLContext *context);

private:
    QGLContextGroup(const QGLContext *context);

    QGLExtensionFuncs m_extensionFuncs;
    const QGLContext *m_context; // context group's representative
    QList<const QGLContext *> m_shares;
    QHash<QGLContextGroupResourceBase *, void *> m_resources;
    QGLSharedResourceGuard *m_guards; // double-linked list of active guards.
    QAtomicInt m_refs;

    void cleanupResources(const QGLContext *ctx);

    friend class QGLContext;
    friend class QGLContextPrivate;
    friend class QGLContextGroupResourceBase;
};

// Get the context that resources for "ctx" will transfer to once
// "ctx" is destroyed.  Returns null if nothing is sharing with ctx.
Q_OPENGL_EXPORT const QGLContext *qt_gl_transfer_context(const QGLContext *);

// GL extension definitions
class QGLExtensions {
public:
    enum Extension {
        TextureRectangle        = 0x00000001,
        SampleBuffers           = 0x00000002,
        GenerateMipmap          = 0x00000004,
        TextureCompression      = 0x00000008,
        FragmentProgram         = 0x00000010,
        MirroredRepeat          = 0x00000020,
        FramebufferObject       = 0x00000040,
        StencilTwoSide          = 0x00000080,
        StencilWrap             = 0x00000100,
        PackedDepthStencil      = 0x00000200,
        NVFloatBuffer           = 0x00000400,
        PixelBufferObject       = 0x00000800,
        FramebufferBlit         = 0x00001000,
        NPOTTextures            = 0x00002000,
        BGRATextureFormat       = 0x00004000,
        DDSTextureCompression   = 0x00008000,
        ETC1TextureCompression  = 0x00010000,
        PVRTCTextureCompression = 0x00020000,
        FragmentShader          = 0x00040000,
        ElementIndexUint        = 0x00080000,
        Depth24                 = 0x00100000
    };
    Q_DECLARE_FLAGS(Extensions, Extension)

    static Extensions glExtensions();
    static Extensions currentContextExtensions();
};

/*
    QGLTemporaryContext - the main objective of this class is to have a way of
    creating a GL context and making it current, without going via QGLWidget
    and friends. At certain points during GL initialization we need a current
    context in order decide what GL features are available, and to resolve GL
    extensions. Having a light-weight way of creating such a context saves
    initial application startup time, and it doesn't wind up creating recursive
    conflicts.
    The class currently uses a private d pointer to hide the platform specific
    types. This could possibly been done inline with #ifdef'ery, but it causes
    major headaches on e.g. X11 due to namespace pollution.
*/
class QGLTemporaryContextPrivate;
class QGLTemporaryContext {
public:
    QGLTemporaryContext(bool directRendering = true, QWidget *parent = 0);
    ~QGLTemporaryContext();

private:
    QScopedPointer<QGLTemporaryContextPrivate> d;
};

class QGLTexture;
class QGLTextureDestroyer;

// This probably needs to grow to GL_MAX_VERTEX_ATTRIBS, but 3 is ok for now as that's
// all the GL2 engine uses:
#define QT_GL_VERTEX_ARRAY_TRACKED_COUNT 3

class QGLContextResourceBase;

class QGLContextPrivate
{
    Q_DECLARE_PUBLIC(QGLContext)
public:
    explicit QGLContextPrivate(QGLContext *context);
    ~QGLContextPrivate();
    QGLTexture *bindTexture(const QImage &image, GLenum target, GLint format,
                            QGLContext::BindOptions options);
    QGLTexture *bindTexture(const QImage &image, GLenum target, GLint format, const qint64 key,
                            QGLContext::BindOptions options);
    QGLTexture *bindTexture(const QPixmap &pixmap, GLenum target, GLint format,
                            QGLContext::BindOptions options);
    QGLTexture *textureCacheLookup(const qint64 key, GLenum target);
    void init(QPaintDevice *dev, const QGLFormat &format);
    QImage convertToGLFormat(const QImage &image, bool force_premul, GLenum texture_format);
    int maxTextureSize();

    void cleanup();

    void setVertexAttribArrayEnabled(int arrayIndex, bool enabled = true);
    void syncGlState(); // Makes sure the GL context's state is what we think it is
    void swapRegion(const QRegion &region);

#if defined(Q_WS_WIN)
    void updateFormatVersion();
#endif

#if defined(Q_WS_WIN)
    HGLRC rc;
    HDC dc;
    WId        win;
    int pixelFormatId;
    QGLCmap* cmap;
    HBITMAP hbitmap;
    HDC hbitmap_hdc;
    Qt::HANDLE threadId;
#endif
#ifndef QT_NO_EGL
    QEglContext *eglContext;
    EGLSurface eglSurface;
    void destroyEglSurfaceForDevice();
    EGLSurface eglSurfaceForDevice() const;
    static QEglProperties *extraWindowSurfaceCreationProps;
    static void setExtraWindowSurfaceCreationProps(QEglProperties *props);
#endif

#if defined(Q_WS_QPA)
    QWindowContext *windowContext;
    void setupSharing();

#elif defined(Q_WS_X11) || defined(Q_WS_MAC)
    void* cx;
#endif
#if defined(Q_WS_X11) || defined(Q_WS_MAC)
    void* vi;
#endif
#if defined(Q_WS_X11)
    void* pbuf;
    quint32 gpm;
    int screen;
    QHash<QPixmapData*, QPixmap> boundPixmaps;
    QGLTexture *bindTextureFromNativePixmap(QPixmap*, const qint64 key,
                                            QGLContext::BindOptions options);
    static void destroyGlSurfaceForPixmap(QPixmapData*);
    static void unbindPixmapFromTexture(QPixmapData*);
#endif
#if defined(Q_WS_MAC)
    bool update;
    void *tryFormat(const QGLFormat &format);
    void clearDrawable();
#endif
    QGLFormat glFormat;
    QGLFormat reqFormat;
    GLuint fbo;

    uint valid : 1;
    uint sharing : 1;
    uint initDone : 1;
    uint crWin : 1;
    uint internal_context : 1;
    uint version_flags_cached : 1;
    uint extension_flags_cached : 1;

    // workarounds for driver/hw bugs on different platforms
    uint workaround_needsFullClearOnEveryFrame : 1;
    uint workaround_brokenFBOReadBack : 1;
    uint workaround_brokenTexSubImage : 1;
    uint workaroundsCached : 1;

    uint workaround_brokenTextureFromPixmap : 1;
    uint workaround_brokenTextureFromPixmap_init : 1;

    uint workaround_brokenAlphaTexSubImage : 1;
    uint workaround_brokenAlphaTexSubImage_init : 1;

#ifndef QT_NO_EGL
    uint ownsEglContext : 1;
#endif

    QPaintDevice *paintDevice;
    QColor transpColor;
    QGLContext *q_ptr;
    QGLFormat::OpenGLVersionFlags version_flags;
    QGLExtensions::Extensions extension_flags;

    QGLContextGroup *group;
    GLint max_texture_size;

    GLuint current_fbo;
    GLuint default_fbo;
    QPaintEngine *active_engine;
    QHash<QGLContextResourceBase *, void *> m_resources;
    QGLTextureDestroyer *texture_destroyer;

    bool vertexAttributeArraysEnabledState[QT_GL_VERTEX_ARRAY_TRACKED_COUNT];

    static inline QGLContextGroup *contextGroup(const QGLContext *ctx) { return ctx->d_ptr->group; }

#ifdef Q_WS_WIN
    static inline QGLExtensionFuncs& extensionFuncs(const QGLContext *ctx) { return ctx->d_ptr->group->extensionFuncs(); }
#endif

#if defined(Q_WS_X11) || defined(Q_WS_MAC) || defined(Q_WS_QWS) || defined(Q_WS_QPA) || defined(Q_OS_SYMBIAN) 
    static Q_OPENGL_EXPORT QGLExtensionFuncs qt_extensionFuncs;
    static Q_OPENGL_EXPORT QGLExtensionFuncs& extensionFuncs(const QGLContext *);
#endif

    static void setCurrentContext(QGLContext *context);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QGLExtensions::Extensions)

// Temporarily make a context current if not already current or
// shared with the current contex.  The previous context is made
// current when the object goes out of scope.
class Q_OPENGL_EXPORT QGLShareContextScope
{
public:
    QGLShareContextScope(const QGLContext *ctx)
        : m_oldContext(0)
    {
        QGLContext *currentContext = const_cast<QGLContext *>(QGLContext::currentContext());
        if (currentContext != ctx && !QGLContext::areSharing(ctx, currentContext)) {
            m_oldContext = currentContext;
            m_ctx = const_cast<QGLContext *>(ctx);
            m_ctx->makeCurrent();
        } else {
            m_ctx = currentContext;
        }
    }

    operator QGLContext *()
    {
        return m_ctx;
    }

    QGLContext *operator->()
    {
        return m_ctx;
    }

    ~QGLShareContextScope()
    {
        if (m_oldContext)
            m_oldContext->makeCurrent();
    }

private:
    QGLContext *m_oldContext;
    QGLContext *m_ctx;
};

class QGLTextureDestroyer : public QObject
{
    Q_OBJECT
public:
    QGLTextureDestroyer() : QObject() {
        qRegisterMetaType<GLuint>("GLuint");
        connect(this, SIGNAL(freeTexture(QGLContext *, QPixmapData *, GLuint)),
                this, SLOT(freeTexture_slot(QGLContext *, QPixmapData *, GLuint)));
    }
    void emitFreeTexture(QGLContext *context, QPixmapData *boundPixmap, GLuint id) {
        emit freeTexture(context, boundPixmap, id);
    }

Q_SIGNALS:
    void freeTexture(QGLContext *context, QPixmapData *boundPixmap, GLuint id);

private slots:
    void freeTexture_slot(QGLContext *context, QPixmapData *boundPixmap, GLuint id) {
        Q_UNUSED(boundPixmap);
#if defined(Q_WS_X11)
        if (boundPixmap) {
            QGLContext *oldContext = const_cast<QGLContext *>(QGLContext::currentContext());
            context->makeCurrent();
            // Although glXReleaseTexImage is a glX call, it must be called while there
            // is a current context - the context the pixmap was bound to a texture in.
            // Otherwise the release doesn't do anything and you get BadDrawable errors
            // when you come to delete the context.
            QGLContextPrivate::unbindPixmapFromTexture(boundPixmap);
            glDeleteTextures(1, &id);
            if (oldContext)
                oldContext->makeCurrent();
            return;
        }
#endif
        QGLShareContextScope scope(context);
        glDeleteTextures(1, &id);
    }
};

// ### make QGLContext a QObject in 5.0 and remove the proxy stuff
class Q_OPENGL_EXPORT QGLSignalProxy : public QObject
{
    Q_OBJECT
public:
    void emitAboutToDestroyContext(const QGLContext *context) {
        emit aboutToDestroyContext(context);
    }
    static QGLSignalProxy *instance();
Q_SIGNALS:
    void aboutToDestroyContext(const QGLContext *context);
};

class QGLTexture {
public:
    QGLTexture(QGLContext *ctx = 0, GLuint tx_id = 0, GLenum tx_target = GL_TEXTURE_2D,
               QGLContext::BindOptions opt = QGLContext::DefaultBindOption)
        : context(ctx),
          id(tx_id),
          target(tx_target),
          options(opt)
#if defined(Q_WS_X11)
        , boundPixmap(0)
#endif
    {}

    ~QGLTexture() {
        if (options & QGLContext::MemoryManagedBindOption) {
            Q_ASSERT(context);
#if !defined(Q_WS_X11)
            QPixmapData *boundPixmap = 0;
#endif
            context->d_ptr->texture_destroyer->emitFreeTexture(context, boundPixmap, id);
        }
     }

    QGLContext *context;
    GLuint id;
    GLenum target;

    QGLContext::BindOptions options;

#if defined(Q_WS_X11)
    QPixmapData* boundPixmap;
#endif

    bool canBindCompressedTexture
        (const char *buf, int len, const char *format, bool *hasAlpha);
    QSize bindCompressedTexture
        (const QString& fileName, const char *format = 0);
    QSize bindCompressedTexture
        (const char *buf, int len, const char *format = 0);
    QSize bindCompressedTextureDDS(const char *buf, int len);
    QSize bindCompressedTexturePVR(const char *buf, int len);
};

struct QGLTextureCacheKey {
    qint64 key;
    QGLContextGroup *group;
};

inline bool operator==(const QGLTextureCacheKey &a, const QGLTextureCacheKey &b)
{
    return a.key == b.key && a.group == b.group;
}

inline uint qHash(const QGLTextureCacheKey &key)
{
    return qHash(key.key) ^ qHash(key.group);
}


class Q_AUTOTEST_EXPORT QGLTextureCache {
public:
    QGLTextureCache();
    ~QGLTextureCache();

    void insert(QGLContext *ctx, qint64 key, QGLTexture *texture, int cost);
    void remove(qint64 key);
    inline int size();
    inline void setMaxCost(int newMax);
    inline int maxCost();
    inline QGLTexture* getTexture(QGLContext *ctx, qint64 key);

    bool remove(QGLContext *ctx, GLuint textureId);
    void removeContextTextures(QGLContext *ctx);
    static QGLTextureCache *instance();
    static void cleanupTexturesForCacheKey(qint64 cacheKey);
    static void cleanupTexturesForPixampData(QPixmapData* pixmap);
    static void cleanupBeforePixmapDestruction(QPixmapData* pixmap);

private:
    QCache<QGLTextureCacheKey, QGLTexture> m_cache;
    QReadWriteLock m_lock;
};

int QGLTextureCache::size() {
    QReadLocker locker(&m_lock);
    return m_cache.size();
}

void QGLTextureCache::setMaxCost(int newMax)
{
    QWriteLocker locker(&m_lock);
    m_cache.setMaxCost(newMax);
}

int QGLTextureCache::maxCost()
{
    QReadLocker locker(&m_lock);
    return m_cache.maxCost();
}

QGLTexture* QGLTextureCache::getTexture(QGLContext *ctx, qint64 key)
{
    QReadLocker locker(&m_lock);
    const QGLTextureCacheKey cacheKey = {key, QGLContextPrivate::contextGroup(ctx)};
    return m_cache.object(cacheKey);
}

extern Q_OPENGL_EXPORT QPaintEngine* qt_qgl_paint_engine();

bool qt_gl_preferGL2Engine();

inline GLenum qt_gl_preferredTextureFormat()
{
    return (QGLExtensions::glExtensions() & QGLExtensions::BGRATextureFormat) && QSysInfo::ByteOrder == QSysInfo::LittleEndian
        ? GL_BGRA : GL_RGBA;
}

inline GLenum qt_gl_preferredTextureTarget()
{
#if defined(QT_OPENGL_ES_2)
    return GL_TEXTURE_2D;
#else
    return (QGLExtensions::glExtensions() & QGLExtensions::TextureRectangle)
           && !qt_gl_preferGL2Engine()
           ? GL_TEXTURE_RECTANGLE_NV
           : GL_TEXTURE_2D;
#endif
}

/*
   Base for resources that are shared in a context group.
*/
class Q_OPENGL_EXPORT QGLContextGroupResourceBase
{
public:
    QGLContextGroupResourceBase();
    virtual ~QGLContextGroupResourceBase();
    void insert(const QGLContext *context, void *value);
    void *value(const QGLContext *context);
    void cleanup(const QGLContext *context);
    void cleanup(const QGLContext *context, void *value);
    virtual void freeResource(void *value) = 0;

protected:
    QList<QGLContextGroup *> m_groups;

private:
    QAtomicInt active;
};

/*
   The QGLContextGroupResource template is used to manage a resource
   for a group of sharing GL contexts. When the last context in the
   group is destroyed, or when the QGLContextGroupResource object
   itself is destroyed (implies potential context switches), the
   resource will be freed.

   The class used as the template class type needs to have a
   constructor with the following signature:
     T(const QGLContext *);
*/
template <class T>
class QGLContextGroupResource : public QGLContextGroupResourceBase
{
public:
    ~QGLContextGroupResource() {
        for (int i = 0; i < m_groups.size(); ++i) {
            const QGLContext *context = m_groups.at(i)->context();
            T *resource = reinterpret_cast<T *>(QGLContextGroupResourceBase::value(context));
            if (resource) {
                QGLShareContextScope scope(context);
                delete resource;
            }
        }
    }

    T *value(const QGLContext *context) {
        T *resource = reinterpret_cast<T *>(QGLContextGroupResourceBase::value(context));
        if (!resource) {
            resource = new T(context);
            insert(context, resource);
        }
        return resource;
    }

protected:
    void freeResource(void *resource) {
        delete reinterpret_cast<T *>(resource);
    }
};

/*
   Base for resources that are context specific.
*/
class Q_OPENGL_EXPORT QGLContextResourceBase
{
public:
    virtual ~QGLContextResourceBase() {
        for (int i = 0; i < m_contexts.size(); ++i)
            m_contexts.at(i)->d_ptr->m_resources.remove(this);
    }

    void insert(const QGLContext *context, void *value) {
        context->d_ptr->m_resources.insert(this, value);
    }

    void *value(const QGLContext *context) {
        return context->d_ptr->m_resources.value(this, 0);
    }
    virtual void freeResource(void *value) = 0;

protected:
    QList<const QGLContext *> m_contexts;
};

/*
   The QGLContextResource template is used to manage a resource for a
   single GL context. Just before the context is destroyed (while it's
   still the current context), or when the QGLContextResource object
   itself is destroyed (implies potential context switches), the
   resource will be freed.  The class used as the template class type
   needs to have a constructor with the following signature: T(const
   QGLContext *);
*/
template <class T>
class QGLContextResource : public QGLContextResourceBase
{
public:
    ~QGLContextResource() {
        for (int i = 0; i < m_contexts.size(); ++i) {
            const QGLContext *context = m_contexts.at(i);
            T *resource = reinterpret_cast<T *>(QGLContextResourceBase::value(context));
            if (resource) {
                QGLShareContextScope scope(context);
                delete resource;
            }
        }
    }

    T *value(const QGLContext *context) {
        T *resource = reinterpret_cast<T *>(QGLContextResourceBase::value(context));
        if (!resource) {
            resource = new T(context);
            insert(context, resource);
        }
        return resource;
    }

protected:
    void freeResource(void *resource) {
        delete reinterpret_cast<T *>(resource);
    }
};

// Put a guard around a GL object identifier and its context.
// When the context goes away, a shared context will be used
// in its place.  If there are no more shared contexts, then
// the identifier is returned as zero - it is assumed that the
// context destruction cleaned up the identifier in this case.
class Q_OPENGL_EXPORT QGLSharedResourceGuard
{
public:
    QGLSharedResourceGuard(const QGLContext *context)
        : m_group(0), m_id(0), m_next(0), m_prev(0)
    {
        setContext(context);
    }
    QGLSharedResourceGuard(const QGLContext *context, GLuint id)
        : m_group(0), m_id(id), m_next(0), m_prev(0)
    {
        setContext(context);
    }
    ~QGLSharedResourceGuard();

    const QGLContext *context() const
    {
        return m_group ? m_group->context() : 0;
    }

    void setContext(const QGLContext *context);

    GLuint id() const
    {
        return m_id;
    }

    void setId(GLuint id)
    {
        m_id = id;
    }

private:
    QGLContextGroup *m_group;
    GLuint m_id;
    QGLSharedResourceGuard *m_next;
    QGLSharedResourceGuard *m_prev;

    friend class QGLContextGroup;
};


class QGLExtensionMatcher
{
public:
    QGLExtensionMatcher(const char *str);
    QGLExtensionMatcher();

    bool match(const char *str) const {
        int str_length = qstrlen(str);

        Q_ASSERT(str);
        Q_ASSERT(str_length > 0);
        Q_ASSERT(str[str_length-1] != ' ');

        for (int i = 0; i < m_offsets.size(); ++i) {
            const char *extension = m_extensions.constData() + m_offsets.at(i);
            if (qstrncmp(extension, str, str_length) == 0 && extension[str_length] == ' ')
                return true;
        }
        return false;
    }

private:
    void init(const char *str);

    QByteArray m_extensions;
    QVector<int> m_offsets;
};


// this is a class that wraps a QThreadStorage object for storing
// thread local instances of the GL 1 and GL 2 paint engines

template <class T>
class QGLEngineThreadStorage
{
public:
    QPaintEngine *engine() {
        QPaintEngine *&localEngine = storage.localData();
        if (!localEngine)
            localEngine = new T;
        return localEngine;
    }

private:
    QThreadStorage<QPaintEngine *> storage;
};
QT_END_NAMESPACE

#endif // QGL_P_H
