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
#include <private/qbackingstorerhisupport_p.h>
#include <private/qbackingstoredefaultcompositor_p.h>
#include <private/qwindow_p.h>

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

    QWindow *window;
    QBackingStore *backingStore;

    // The order matters. if it needs to be rearranged in the future, call
    // reset() explicitly from the dtor in the correct order.
    // (first the compositor, then the rhiSupport)
    QBackingStoreRhiSupport rhiSupport;
    QBackingStoreDefaultCompositor compositor;
};

struct QBackingstoreTextureInfo
{
    void *source; // may be null
    QRhiTexture *texture;
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

QRhiTexture *QPlatformTextureList::texture(int index) const
{
    Q_D(const QPlatformTextureList);
    return d->textures.at(index).texture;
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

void QPlatformTextureList::appendTexture(void *source, QRhiTexture *texture, const QRect &geometry,
                                         const QRect &clipRect, Flags flags)
{
    Q_D(QPlatformTextureList);
    QBackingstoreTextureInfo bi;
    bi.source = source;
    bi.texture = texture;
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

/*!
    \class QPlatformBackingStore
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformBackingStore class provides the drawing area for top-level
    windows.
*/

/*!
    Flushes the given \a region from the specified \a window.

    \note \a region is relative to the window which may not be top-level in case
    \a window corresponds to a native child widget. \a offset is the position of
    the native child relative to the top-level window.

    Unlike rhiFlush(), this function's default implementation does nothing. It
    is expected that subclasses provide a platform-specific (non-QRhi-based)
    implementation, if applicable on the given platform.

    \sa rhiFlush()
 */
void QPlatformBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(window);
    Q_UNUSED(region);
    Q_UNUSED(offset);
}

/*!
    Flushes the given \a region from the specified \a window, and compositing
    it with the specified \a textures list.

    The default implementation retrieves the contents using toTexture() and
    composes using QRhi with OpenGL, Metal, Vulkan, or Direct 3D underneath.
    May be reimplemented in subclasses if customization is desired.

    \note \a region is relative to the window which may not be top-level in case
    \a window corresponds to a native child widget. \a offset is the position of
    the native child relative to the top-level window.

    \sa flush()
 */
QPlatformBackingStore::FlushResult QPlatformBackingStore::rhiFlush(QWindow *window,
                                                                   const QRegion &region,
                                                                   const QPoint &offset,
                                                                   QPlatformTextureList *textures,
                                                                   bool translucentBackground)
{
    return d_ptr->compositor.flush(this, d_ptr->rhiSupport.rhi(), d_ptr->rhiSupport.swapChainForWindow(window),
                                   window, region, offset, textures, translucentBackground);
}

/*!
  Implemented in subclasses to return the content of the backingstore as a QImage.

  If composition via a 3D graphics API is supported, either this function or
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

/*!
  May be reimplemented in subclasses to return the content of the
  backingstore as an QRhiTexture. \a dirtyRegion is the part of the
  backingstore which may have changed since the last call to this function. The
  caller of this function must ensure that there is a current context.

  The ownership of the texture is not transferred. The caller must not store
  the return value between calls, but instead call this function before each use.

  The default implementation returns a cached texture if \a dirtyRegion is
  empty and the existing texture's size matches the backingstore size,
  otherwise it retrieves the content using toImage() and performs a texture
  upload.

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
QRhiTexture *QPlatformBackingStore::toTexture(QRhiResourceUpdateBatch *resourceUpdates,
                                              const QRegion &dirtyRegion,
                                              TextureFlags *flags) const
{
    return d_ptr->compositor.toTexture(this, d_ptr->rhiSupport.rhi(), resourceUpdates, dirtyRegion, flags);
}

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

void QPlatformBackingStore::setRhiConfig(const QPlatformBackingStoreRhiConfig &config)
{
    if (!config.isEnabled())
        return;

    d_ptr->rhiSupport.setConfig(config);
    d_ptr->rhiSupport.setWindow(d_ptr->window);
    d_ptr->rhiSupport.setFormat(d_ptr->window->format());
    d_ptr->rhiSupport.create();
}

QRhi *QPlatformBackingStore::rhi() const
{
    // Returning null is valid, and means this is not a QRhi-capable backingstore.
    return d_ptr->rhiSupport.rhi();
}

void QPlatformBackingStore::graphicsDeviceReportedLost()
{
    if (!d_ptr->rhiSupport.rhi())
        return;

    qWarning("Rhi backingstore: graphics device lost, attempting to reinitialize");
    d_ptr->rhiSupport.reset();
    d_ptr->rhiSupport.create();
    if (!d_ptr->rhiSupport.rhi())
        qWarning("Rhi backingstore: failed to reinitialize after losing the device");
}

QT_END_NAMESPACE
