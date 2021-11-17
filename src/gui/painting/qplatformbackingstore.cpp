/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qplatformbackingstore.h"
#include <qwindow.h>
#include <qpixmap.h>

#include <QtCore/private/qobject_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaBackingStore, "qt.qpa.backingstore", QtWarningMsg);

class QPlatformBackingStorePrivate
{
public:
    QPlatformBackingStorePrivate(QWindow *w)
        : window(w)
        , backingStore(nullptr)
    {
    }

    ~QPlatformBackingStorePrivate()
    {
#ifndef QT_NO_OPENGL
        delete openGLSupport;
#endif
    }
    QWindow *window;
    QBackingStore *backingStore;
#ifndef QT_NO_OPENGL
    QPlatformBackingStoreOpenGLSupportBase *openGLSupport = nullptr;
#endif
};

#ifndef QT_NO_OPENGL

struct QBackingstoreTextureInfo
{
    void *source; // may be null
    GLuint textureId;
    QRect rect;
    QRect clipRect;
    QPlatformTextureList::Flags flags;
};

Q_DECLARE_TYPEINFO(QBackingstoreTextureInfo, Q_RELOCATABLE_TYPE);

class QPlatformTextureListPrivate : public QObjectPrivate
{
public:
    QPlatformTextureListPrivate()
        : locked(false)
    {
    }

    QList<QBackingstoreTextureInfo> textures;
    bool locked;
};

QPlatformTextureList::QPlatformTextureList(QObject *parent)
: QObject(*new QPlatformTextureListPrivate, parent)
{
}

QPlatformTextureList::~QPlatformTextureList()
{
}

int QPlatformTextureList::count() const
{
    Q_D(const QPlatformTextureList);
    return d->textures.count();
}

GLuint QPlatformTextureList::textureId(int index) const
{
    Q_D(const QPlatformTextureList);
    return d->textures.at(index).textureId;
}

void *QPlatformTextureList::source(int index)
{
    Q_D(const QPlatformTextureList);
    return d->textures.at(index).source;
}

QPlatformTextureList::Flags QPlatformTextureList::flags(int index) const
{
    Q_D(const QPlatformTextureList);
    return d->textures.at(index).flags;
}

QRect QPlatformTextureList::geometry(int index) const
{
    Q_D(const QPlatformTextureList);
    return d->textures.at(index).rect;
}

QRect QPlatformTextureList::clipRect(int index) const
{
    Q_D(const QPlatformTextureList);
    return d->textures.at(index).clipRect;
}

void QPlatformTextureList::lock(bool on)
{
    Q_D(QPlatformTextureList);
    if (on != d->locked) {
        d->locked = on;
        emit locked(on);
    }
}

bool QPlatformTextureList::isLocked() const
{
    Q_D(const QPlatformTextureList);
    return d->locked;
}

void QPlatformTextureList::appendTexture(void *source, GLuint textureId, const QRect &geometry,
                                         const QRect &clipRect, Flags flags)
{
    Q_D(QPlatformTextureList);
    QBackingstoreTextureInfo bi;
    bi.source = source;
    bi.textureId = textureId;
    bi.rect = geometry;
    bi.clipRect = clipRect;
    bi.flags = flags;
    d->textures.append(bi);
}

void QPlatformTextureList::clear()
{
    Q_D(QPlatformTextureList);
    d->textures.clear();
}
#endif // QT_NO_OPENGL

/*!
    \class QPlatformBackingStore
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformBackingStore class provides the drawing area for top-level
    windows.
*/

#ifndef QT_NO_OPENGL
/*!
    Flushes the given \a region from the specified \a window onto the
    screen, and composes it with the specified \a textures.

    The default implementation retrieves the contents using toTexture()
    and composes using OpenGL. May be reimplemented in subclasses if there
    is a more efficient native way to do it.

    \note \a region is relative to the window which may not be top-level in case
    \a window corresponds to a native child widget. \a offset is the position of
    the native child relative to the top-level window.
 */

void QPlatformBackingStore::composeAndFlush(QWindow *window, const QRegion &region,
                                            const QPoint &offset,
                                            QPlatformTextureList *textures,
                                            bool translucentBackground)
{
    if (auto *c = d_ptr->openGLSupport)
        c->composeAndFlush(window, region, offset, textures, translucentBackground);
    else
        qWarning() << Q_FUNC_INFO << "no opengl support set";
}
#endif

/*!
  Implemented in subclasses to return the content of the backingstore as a QImage.

  If QPlatformIntegration::RasterGLSurface is supported, either this function or
  toTexture() must be implemented.

  The returned image is only valid until the next operation (resize, paint, scroll,
  or flush) on the backingstore. The caller must not store the return value between
  calls, but instead call this function before each use, or make an explicit copy.

  \sa toTexture()
 */
QImage QPlatformBackingStore::toImage() const
{
    return QImage();
}

#ifndef QT_NO_OPENGL
/*!
  May be reimplemented in subclasses to return the content of the
  backingstore as an OpenGL texture. \a dirtyRegion is the part of the
  backingstore which may have changed since the last call to this function. The
  caller of this function must ensure that there is a current context.

  The size of the texture is returned in \a textureSize.

  The ownership of the texture is not transferred. The caller must not store
  the return value between calls, but instead call this function before each use.

  The default implementation returns a cached texture if \a dirtyRegion is empty and
  \a textureSize matches the backingstore size, otherwise it retrieves the content using
  toImage() and performs a texture upload. This works only if the value of \a textureSize
  is preserved between the calls to this function.

  If the red and blue components have to swapped, \a flags will be set to include \c
  TextureSwizzle. This allows creating textures from images in formats like
  QImage::Format_RGB32 without any further image conversion. Instead, the swizzling will
  be done in the shaders when performing composition. Other formats, that do not need
  such swizzling due to being already byte ordered RGBA, for example
  QImage::Format_RGBA8888, must result in having \a needsSwizzle set to false.

  If the image has to be flipped (e.g. because the texture is attached to an FBO), \a
  flags will be set to include \c TextureFlip.

  \note \a dirtyRegion is relative to the backingstore so no adjustment is needed.
 */
GLuint QPlatformBackingStore::toTexture(const QRegion &dirtyRegion, QSize *textureSize, TextureFlags *flags) const
{
    if (auto *c = d_ptr->openGLSupport)
        return c->toTexture(dirtyRegion, textureSize, flags);
    else {
        qWarning() << Q_FUNC_INFO << "no opengl support set";
        return 0;
    }
}
#endif // QT_NO_OPENGL

/*!
    \fn QPaintDevice* QPlatformBackingStore::paintDevice()

    Implement this function to return the appropriate paint device.
*/

/*!
    Constructs an empty surface for the given top-level \a window.
*/
QPlatformBackingStore::QPlatformBackingStore(QWindow *window)
    : d_ptr(new QPlatformBackingStorePrivate(window))
{
#ifndef QT_NO_OPENGL
    if (auto createOpenGLSupport = QPlatformBackingStoreOpenGLSupportBase::factoryFunction()) {
        d_ptr->openGLSupport = createOpenGLSupport();
        d_ptr->openGLSupport->backingStore = this;
    }
#endif
}

/*!
    Destroys this surface.
*/
QPlatformBackingStore::~QPlatformBackingStore()
{
    delete d_ptr;
}

/*!
    Returns a pointer to the top-level window associated with this
    surface.
*/
QWindow* QPlatformBackingStore::window() const
{
    return d_ptr->window;
}

/*!
    Sets the backing store associated with this surface.
*/
void QPlatformBackingStore::setBackingStore(QBackingStore *backingStore)
{
    d_ptr->backingStore = backingStore;
}

/*!
    Returns a pointer to the backing store associated with this
    surface.
*/
QBackingStore *QPlatformBackingStore::backingStore() const
{
    return d_ptr->backingStore;
}

#ifndef QT_NO_OPENGL

using FactoryFunction = QPlatformBackingStoreOpenGLSupportBase::FactoryFunction;

/*!
    Registers a factory function for OpenGL implementation helper.

    The QtOpenGL library automatically registers a default function,
    unless already set by the platform plugin in other ways.
*/
void QPlatformBackingStoreOpenGLSupportBase::setFactoryFunction(FactoryFunction function)
{
    s_factoryFunction = function;
}

FactoryFunction QPlatformBackingStoreOpenGLSupportBase::factoryFunction()
{
    return s_factoryFunction;
}

FactoryFunction QPlatformBackingStoreOpenGLSupportBase::s_factoryFunction = nullptr;

#endif // QT_NO_OPENGL

/*!
    This function is called before painting onto the surface begins,
    with the \a region in which the painting will occur.

    \sa endPaint(), paintDevice()
*/

void QPlatformBackingStore::beginPaint(const QRegion &)
{
}

/*!
    This function is called after painting onto the surface has ended.

    \sa beginPaint(), paintDevice()
*/

void QPlatformBackingStore::endPaint()
{
}

/*!
    Accessor for a backingstores graphics buffer abstraction
*/
QPlatformGraphicsBuffer *QPlatformBackingStore::graphicsBuffer() const
{
    return nullptr;
}

/*!
    Scrolls the given \a area \a dx pixels to the right and \a dy
    downward; both \a dx and \a dy may be negative.

    Returns \c true if the area was scrolled successfully; false otherwise.
*/
bool QPlatformBackingStore::scroll(const QRegion &area, int dx, int dy)
{
    Q_UNUSED(area);
    Q_UNUSED(dx);
    Q_UNUSED(dy);

    return false;
}

QT_END_NAMESPACE
