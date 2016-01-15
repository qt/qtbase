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

#include "qplatformgraphicsbuffer.h"
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/qopengl.h>
#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE
/*!
    \class QPlatformGraphicsBuffer
    \inmodule QtGui
    \since 5.5
    \brief The QPlatformGraphicsBuffer is a windowsystem abstraction for native graphics buffers

    Different platforms have different ways of representing graphics buffers. On
    some platforms, it is possible to create one graphics buffer that you can bind
    to a texture and also get main memory access to the image bits. On the
    other hand, on some platforms all graphics buffer abstraction is completely
    hidden.

    QPlatformGraphicsBuffer is an abstraction of a single Graphics Buffer.

    There is no public constructor nor any public factory function.

    QPlatformGraphicsBuffer is intended to be created by using platform specific
    APIs available from QtPlatformHeaders, or there might be accessor functions
    similar to the accessor function that QPlatformBackingstore has.
*/

/*!
    \enum QPlatformGraphicsBuffer::AccessType

    This enum describes the access that is desired or granted for the graphics
    buffer.

    \value None
    \value SWReadAccess
    \value SWWriteAccess
    \value TextureAccess
    \value HWCompositor
*/

/*!
    \enum QPlatformGraphicsBuffer::Origin

    This enum describes the origin of the content of the buffer.

    \value OriginTopLeft
    \value OriginBottomLeft
*/

/*!
    Protected constructor to initialize the private members.

    \a size is the size of the buffer.
    \a format is the format of the buffer.

    \sa size() format()
*/
QPlatformGraphicsBuffer::QPlatformGraphicsBuffer(const QSize &size, const QPixelFormat &format)
    : m_size(size)
    , m_format(format)
{
}


/*!
    Virtual destructor.
*/
QPlatformGraphicsBuffer::~QPlatformGraphicsBuffer()
{
}

/*!
    Binds the content of this graphics buffer into the currently bound texture.

    This function should fail for buffers not capable of locking to TextureAccess.

    \a rect is the subrect which is desired to be bounded to the texture. This
    argument has a no less than semantic, meaning more (if not all) of the buffer
    can be bounded to the texture. An empty QRect is interpreted as entire buffer
    should be bound.

    This function only supports binding buffers to the GL_TEXTURE_2D texture
    target.

    Returns true on success, otherwise false.
*/
bool QPlatformGraphicsBuffer::bindToTexture(const QRect &rect) const
{
    Q_UNUSED(rect);
    return false;
}

/*!
    \fn QPlatformGraphicsBuffer::AccessTypes QPlatformGraphicsBuffer::isLocked() const
    Function to check if the buffer is locked.

    \sa lock()
*/

/*!
    Before the data can be retrieved or before a buffer can be bound to a
    texture it needs to be locked. This is a separate function call since this
    operation might be time consuming, and it would not be satisfactory to do
    it per function call.

    \a access is the access type wanted.

    \a rect is the subrect which is desired to be locked. This
    argument has a no less than semantic, meaning more (if not all) of the buffer
    can be locked. An empty QRect is interpreted as entire buffer should be locked.

    Return true on successfully locking all AccessTypes specified \a access
    otherwise returns false and no locks have been granted.
*/
bool QPlatformGraphicsBuffer::lock(AccessTypes access, const QRect &rect)
{
    bool locked = doLock(access, rect);
    if (locked)
        m_lock_access |= access;

    return locked;
}

/*!
    Unlocks the current buffer lock.

    This function calls doUnlock, and then emits the unlocked signal with the
    AccessTypes from before doUnlock was called.
*/
void QPlatformGraphicsBuffer::unlock()
{
    if (m_lock_access == None)
        return;
    AccessTypes previous = m_lock_access;
    doUnlock();
    m_lock_access = None;
    emit unlocked(previous);
}


/*!
    \fn QPlatformGraphicsBuffer::doLock(AccessTypes access, const QRect &rect = QRect())

    This function should be reimplemented by subclasses. If one of the \a
    access types specified can not be locked, then all should fail and this
    function should return false.

    \a rect is the subrect which is desired to be locked. This
    argument has a no less than semantic, meaning more (if not all) of the
    buffer can be locked. An empty QRect should be interpreted as the entire buffer
    should be locked.

    It is safe to call isLocked() to verify the current lock state.
*/

/*!
    \fn QPlatformGraphicsBuffer::doUnlock()

    This function should remove all locks set on the buffer.

    It is safe to call isLocked() to verify the current lock state.
*/

/*!
    \fn QPlatformGraphicsBuffer::unlocked(AccessTypes previousAccessTypes)

    Signal that is emitted after unlocked has been called.

    \a previousAccessTypes is the access types locked before unlock was called.
*/

/*!
    Accessor for the bytes of the buffer. This function needs to be called on a
    buffer with SWReadAccess access lock. Behavior is undefined for modifying
    the memory returned when not having a SWWriteAccess.
*/
const uchar *QPlatformGraphicsBuffer::data() const
{ return Q_NULLPTR; }

/*!
    Accessor for the bytes of the buffer. This function needs to be called on a
    buffer with SWReadAccess access lock. Behavior is undefined for modifying
    the memory returned when not having a SWWriteAccess.
*/
uchar *QPlatformGraphicsBuffer::data()
{
    return Q_NULLPTR;
}

/*!
    Accessor for the length of the data buffer. This function is a convenience
    function multiplying height of buffer with bytesPerLine().

    \sa data() bytesPerLine() size()
*/
int QPlatformGraphicsBuffer::byteCount() const
{
    Q_ASSERT(isLocked() & SWReadAccess);
    return size().height() * bytesPerLine();
}

/*!
    Accessor for bytes per line in the graphics buffer.
*/
int QPlatformGraphicsBuffer::bytesPerLine() const
{
    return 0;
}


/*!
    In origin of the content of the graphics buffer.

    Default implementation is OriginTopLeft, as this is the coordinate
    system default for Qt. However, for most regular OpenGL textures
    this will be OriginBottomLeft.
*/
QPlatformGraphicsBuffer::Origin QPlatformGraphicsBuffer::origin() const
{
    return OriginTopLeft;
}

/*!
    \fn QPlatformGraphicsBuffer::size() const

    Accessor for content size.
*/

/*!
    \fn QPlatformGraphicsBuffer::format() const

    Accessor for the pixel format of the buffer.
*/

QT_END_NAMESPACE
