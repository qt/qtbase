/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qiosurfacegraphicsbuffer.h"

#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>

#include <CoreGraphics/CoreGraphics.h>
#include <IOSurface/IOSurface.h>

// CGColorSpaceCopyPropertyList is available on 10.12 and above,
// but was only added in the 10.14 SDK, so declare it just in case.
extern "C" CFPropertyListRef CGColorSpaceCopyPropertyList(CGColorSpaceRef space);

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaIOSurface, "qt.qpa.backingstore.iosurface");

QIOSurfaceGraphicsBuffer::QIOSurfaceGraphicsBuffer(const QSize &size, const QPixelFormat &format)
    : QPlatformGraphicsBuffer(size, format)
{
    const size_t width = size.width();
    const size_t height = size.height();

    Q_ASSERT(width <= IOSurfaceGetPropertyMaximum(kIOSurfaceWidth));
    Q_ASSERT(height <= IOSurfaceGetPropertyMaximum(kIOSurfaceHeight));

    static const char bytesPerElement = 4;

    const size_t bytesPerRow = IOSurfaceAlignProperty(kIOSurfaceBytesPerRow, width * bytesPerElement);
    const size_t totalBytes = IOSurfaceAlignProperty(kIOSurfaceAllocSize, height * bytesPerRow);

    NSDictionary *options = @{
        (id)kIOSurfaceWidth: @(width),
        (id)kIOSurfaceHeight: @(height),
        (id)kIOSurfacePixelFormat: @(unsigned('BGRA')),
        (id)kIOSurfaceBytesPerElement: @(bytesPerElement),
        (id)kIOSurfaceBytesPerRow: @(bytesPerRow),
        (id)kIOSurfaceAllocSize: @(totalBytes),
    };

    m_surface = IOSurfaceCreate((CFDictionaryRef)options);
    Q_ASSERT(m_surface);

    Q_ASSERT(size_t(bytesPerLine()) == bytesPerRow);
    Q_ASSERT(size_t(byteCount()) == totalBytes);
}

QIOSurfaceGraphicsBuffer::~QIOSurfaceGraphicsBuffer()
{
}

void QIOSurfaceGraphicsBuffer::setColorSpace(QCFType<CGColorSpaceRef> colorSpace)
{
    static const auto kIOSurfaceColorSpace = CFSTR("IOSurfaceColorSpace");

    qCDebug(lcQpaIOSurface) << "Tagging" << this << "with color space" << colorSpace;

    if (colorSpace) {
        IOSurfaceSetValue(m_surface, kIOSurfaceColorSpace,
            QCFType<CFPropertyListRef>(CGColorSpaceCopyPropertyList(colorSpace)));
    } else {
        IOSurfaceRemoveValue(m_surface, kIOSurfaceColorSpace);
    }
}

const uchar *QIOSurfaceGraphicsBuffer::data() const
{
    return (const uchar *)IOSurfaceGetBaseAddress(m_surface);
}

uchar *QIOSurfaceGraphicsBuffer::data()
{
    return (uchar *)IOSurfaceGetBaseAddress(m_surface);
}

int QIOSurfaceGraphicsBuffer::bytesPerLine() const
{
    return IOSurfaceGetBytesPerRow(m_surface);
}

IOSurfaceRef QIOSurfaceGraphicsBuffer::surface()
{
    return m_surface;
}

bool QIOSurfaceGraphicsBuffer::isInUse() const
{
    return IOSurfaceIsInUse(m_surface);
}

IOSurfaceLockOptions lockOptionsForAccess(QPlatformGraphicsBuffer::AccessTypes access)
{
    IOSurfaceLockOptions lockOptions = 0;
    if (!(access & QPlatformGraphicsBuffer::SWWriteAccess))
        lockOptions |= kIOSurfaceLockReadOnly;
    return lockOptions;
}

bool QIOSurfaceGraphicsBuffer::doLock(AccessTypes access, const QRect &rect)
{
    Q_UNUSED(rect);
    Q_ASSERT(!isLocked());

    qCDebug(lcQpaIOSurface) << "Locking" << this << "for" << access;

    // FIXME: Teach QPlatformBackingStore::composeAndFlush about non-2D texture
    // targets, so that we can use CGLTexImageIOSurface2D to support TextureAccess.
    if (access & (TextureAccess | HWCompositor))
        return false;

    auto lockOptions = lockOptionsForAccess(access);

    // Try without read-back first
    lockOptions |= kIOSurfaceLockAvoidSync;
    kern_return_t ret = IOSurfaceLock(m_surface, lockOptions, nullptr);
    if (ret == kIOSurfaceSuccess)
        return true;

    if (ret == kIOReturnCannotLock) {
        qCWarning(lcQpaIOSurface) << "Locking of" << this << "requires read-back";
        lockOptions ^= kIOSurfaceLockAvoidSync;
        ret = IOSurfaceLock(m_surface, lockOptions, nullptr);
    }

    if (ret != kIOSurfaceSuccess) {
        qCWarning(lcQpaIOSurface) << "Failed to lock" << this << ret;
        return false;
    }

    return true;
}

void QIOSurfaceGraphicsBuffer::doUnlock()
{
    qCDebug(lcQpaIOSurface) << "Unlocking" << this << "from" << isLocked();

    auto lockOptions = lockOptionsForAccess(isLocked());
    bool success = IOSurfaceUnlock(m_surface, lockOptions, nullptr) == kIOSurfaceSuccess;
    Q_ASSERT_X(success, "QIOSurfaceGraphicsBuffer", "Unlocking surface should succeed");
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QIOSurfaceGraphicsBuffer *graphicsBuffer)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug << "QIOSurfaceGraphicsBuffer(" << (const void *)graphicsBuffer;
    if (graphicsBuffer) {
        debug << ", surface=" << graphicsBuffer->m_surface;
        debug << ", size=" << graphicsBuffer->size();
        debug << ", isLocked=" << bool(graphicsBuffer->isLocked());
        debug << ", isInUse=" << graphicsBuffer->isInUse();
    }
    debug << ')';
    return debug;
}
#endif // !QT_NO_DEBUG_STREAM

QT_END_NAMESPACE
