/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qxcbbackingstore.h"

#include "qxcbconnection.h"
#include "qxcbscreen.h"
#include "qxcbwindow.h"

#include <xcb/shm.h>
#include <xcb/xcb_image.h>
#include <xcb/render.h>
#include <xcb/xcb_renderutil.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>

#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <qdebug.h>
#include <qpainter.h>
#include <qscreen.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <qpa/qplatformgraphicsbuffer.h>
#include <private/qimage_p.h>
#include <qendian.h>

#include <algorithm>

#if (XCB_SHM_MAJOR_VERSION == 1 && XCB_SHM_MINOR_VERSION >= 2) || XCB_SHM_MAJOR_VERSION > 1
#define XCB_USE_SHM_FD
#endif

QT_BEGIN_NAMESPACE

class QXcbBackingStore;

class QXcbBackingStoreImage : public QXcbObject
{
public:
    QXcbBackingStoreImage(QXcbBackingStore *backingStore, const QSize &size);
    QXcbBackingStoreImage(QXcbBackingStore *backingStore, const QSize &size, uint depth, QImage::Format format);
    ~QXcbBackingStoreImage() { destroy(true); }

    void resize(const QSize &size);

    void flushScrolledRegion(bool clientSideScroll);

    bool scroll(const QRegion &area, int dx, int dy);

    QImage *image() { return &m_qimage; }
    QPlatformGraphicsBuffer *graphicsBuffer() { return m_graphics_buffer; }

    QSize size() const { return m_qimage.size(); }

    bool hasAlpha() const { return m_hasAlpha; }
    bool hasShm() const { return m_shm_info.shmaddr != nullptr; }

    void put(xcb_drawable_t dst, const QRegion &region, const QPoint &offset);
    void preparePaint(const QRegion &region);

    static bool createSystemVShmSegment(xcb_connection_t *c, size_t segmentSize = 1,
                                        xcb_shm_segment_info_t *shm_info = nullptr);

private:
    void init(const QSize &size, uint depth, QImage::Format format);

    void createShmSegment(size_t segmentSize);
    void destroyShmSegment();
    void destroy(bool destroyShm);

    void ensureGC(xcb_drawable_t dst);
    void shmPutImage(xcb_drawable_t drawable, const QRegion &region, const QPoint &offset = QPoint());
    void flushPixmap(const QRegion &region, bool fullRegion = false);
    void setClip(const QRegion &region);

    xcb_shm_segment_info_t m_shm_info;
    size_t m_segmentSize = 0;
    QXcbBackingStore *m_backingStore = nullptr;

    xcb_image_t *m_xcb_image = nullptr;

    QImage m_qimage;
    QPlatformGraphicsBuffer *m_graphics_buffer = nullptr;

    xcb_gcontext_t m_gc = 0;
    xcb_drawable_t m_gc_drawable = 0;

    // When using shared memory these variables are used only for server-side scrolling.
    // When not using shared memory, we maintain a server-side pixmap with the backing
    // store as well as repainted content not yet flushed to the pixmap. We only flush
    // the regions we need and only when these are marked dirty. This way we can just
    // do a server-side copy on expose instead of sending the pixels every time
    xcb_pixmap_t m_xcb_pixmap = 0;
    QRegion m_pendingFlush;

    // This is the scrolled region which is stored in server-side pixmap
    QRegion m_scrolledRegion;

    // When using shared memory this is the region currently shared with the server
    QRegion m_dirtyShm;

    // When not using shared memory this is a temporary buffer which is uploaded
    // as a pixmap region to server
    QByteArray m_flushBuffer;

    bool m_hasAlpha = false;
    bool m_clientSideScroll = false;

    const xcb_format_t *m_xcb_format = nullptr;
    QImage::Format m_qimage_format = QImage::Format_Invalid;
};

class QXcbGraphicsBuffer : public QPlatformGraphicsBuffer
{
public:
    QXcbGraphicsBuffer(QImage *image)
        : QPlatformGraphicsBuffer(image->size(), QImage::toPixelFormat(image->format()))
        , m_image(image)
    { }

    bool doLock(AccessTypes access, const QRect &rect) override
    {
        Q_UNUSED(rect);
        if (access & ~(QPlatformGraphicsBuffer::SWReadAccess | QPlatformGraphicsBuffer::SWWriteAccess))
            return false;

        m_access_lock |= access;
        return true;
    }
    void doUnlock() override { m_access_lock = None; }

    const uchar *data() const override { return m_image->bits(); }
    uchar *data() override { return m_image->bits(); }
    int bytesPerLine() const override { return m_image->bytesPerLine(); }

    Origin origin() const override { return QPlatformGraphicsBuffer::OriginTopLeft; }

private:
    AccessTypes m_access_lock = QPlatformGraphicsBuffer::None;
    QImage *m_image = nullptr;
};

static inline size_t imageDataSize(const xcb_image_t *image)
{
    return static_cast<size_t>(image->stride) * image->height;
}

QXcbBackingStoreImage::QXcbBackingStoreImage(QXcbBackingStore *backingStore, const QSize &size)
    : QXcbObject(backingStore->connection())
    , m_backingStore(backingStore)
{
    auto window = static_cast<QXcbWindow *>(m_backingStore->window()->handle());
    init(size, window->depth(), window->imageFormat());
}

QXcbBackingStoreImage::QXcbBackingStoreImage(QXcbBackingStore *backingStore, const QSize &size,
                                             uint depth, QImage::Format format)
    : QXcbObject(backingStore->connection())
    , m_backingStore(backingStore)
{
    init(size, depth, format);
}

void QXcbBackingStoreImage::init(const QSize &size, uint depth, QImage::Format format)
{
    m_xcb_format = connection()->formatForDepth(depth);
    Q_ASSERT(m_xcb_format);

    m_qimage_format = format;
    m_hasAlpha = QImage::toPixelFormat(m_qimage_format).alphaUsage() == QPixelFormat::UsesAlpha;
    if (!m_hasAlpha)
        m_qimage_format = qt_maybeAlphaVersionWithSameDepth(m_qimage_format);

    memset(&m_shm_info, 0, sizeof m_shm_info);

    resize(size);
}

void QXcbBackingStoreImage::resize(const QSize &size)
{
    destroy(false);

    auto byteOrder = QSysInfo::ByteOrder == QSysInfo::BigEndian ? XCB_IMAGE_ORDER_MSB_FIRST
                                                                : XCB_IMAGE_ORDER_LSB_FIRST;
    m_xcb_image = xcb_image_create(size.width(), size.height(),
                                   XCB_IMAGE_FORMAT_Z_PIXMAP,
                                   m_xcb_format->scanline_pad,
                                   m_xcb_format->depth,
                                   m_xcb_format->bits_per_pixel,
                                   0, byteOrder,
                                   XCB_IMAGE_ORDER_MSB_FIRST,
                                   nullptr, ~0, nullptr);

    const size_t segmentSize = imageDataSize(m_xcb_image);

    if (connection()->hasShm()) {
        if (segmentSize == 0) {
            if (m_segmentSize > 0) {
                destroyShmSegment();
                qCDebug(lcQpaXcb) << "[" << m_backingStore->window()
                                  << "] destroyed SHM segment due to resize to" << size;
            }
        } else {
            // Destroy shared memory segment if it is double (or more) of what we actually
            // need with new window size. Or if the new size is bigger than what we currently
            // have allocated.
            if (m_shm_info.shmaddr && (m_segmentSize < segmentSize || m_segmentSize / 2 >= segmentSize))
                destroyShmSegment();
            if (!m_shm_info.shmaddr) {
                qCDebug(lcQpaXcb) << "[" << m_backingStore->window()
                                  << "] creating shared memory" << segmentSize << "bytes for"
                                  << size << "depth" << m_xcb_format->depth << "bits"
                                  << m_xcb_format->bits_per_pixel;
                createShmSegment(segmentSize);
            }
        }
    }

    if (segmentSize == 0)
        return;

    m_xcb_image->data = m_shm_info.shmaddr ? m_shm_info.shmaddr : (uint8_t *)malloc(segmentSize);
    m_qimage = QImage(static_cast<uchar *>(m_xcb_image->data), m_xcb_image->width,
                      m_xcb_image->height, m_xcb_image->stride, m_qimage_format);
    m_graphics_buffer = new QXcbGraphicsBuffer(&m_qimage);

    m_xcb_pixmap = xcb_generate_id(xcb_connection());
    auto xcbScreen = static_cast<QXcbScreen *>(m_backingStore->window()->screen()->handle());
    xcb_create_pixmap(xcb_connection(),
                      m_xcb_image->depth,
                      m_xcb_pixmap,
                      xcbScreen->root(),
                      m_xcb_image->width, m_xcb_image->height);
}

void QXcbBackingStoreImage::destroy(bool destroyShm)
{
    if (m_xcb_image) {
        if (m_xcb_image->data) {
            if (m_shm_info.shmaddr) {
                if (destroyShm)
                    destroyShmSegment();
            } else {
                free(m_xcb_image->data);
            }
        }
        xcb_image_destroy(m_xcb_image);
    }

    if (m_gc) {
        xcb_free_gc(xcb_connection(), m_gc);
        m_gc = 0;
    }
    m_gc_drawable = 0;

    delete m_graphics_buffer;
    m_graphics_buffer = nullptr;

    if (m_xcb_pixmap) {
        xcb_free_pixmap(xcb_connection(), m_xcb_pixmap);
        m_xcb_pixmap = 0;
    }

    m_qimage = QImage();
}

void QXcbBackingStoreImage::flushScrolledRegion(bool clientSideScroll)
{
    if (m_clientSideScroll == clientSideScroll)
       return;

    m_clientSideScroll = clientSideScroll;

    if (m_scrolledRegion.isNull())
        return;

    if (hasShm() && m_dirtyShm.intersects(m_scrolledRegion)) {
        connection()->sync();
        m_dirtyShm = QRegion();
    }

    if (m_clientSideScroll) {
        // Copy scrolled image region from server-side pixmap to client-side memory
        for (const QRect &rect : m_scrolledRegion) {
            const int w = rect.width();
            const int h = rect.height();

            auto reply = Q_XCB_REPLY_UNCHECKED(xcb_get_image,
                                               xcb_connection(),
                                               m_xcb_image->format,
                                               m_xcb_pixmap,
                                               rect.x(), rect.y(),
                                               w, h,
                                               ~0u);

            if (reply && reply->depth == m_xcb_image->depth) {
                const QImage img(xcb_get_image_data(reply.get()), w, h, m_qimage.format());

                QPainter p(&m_qimage);
                p.setCompositionMode(QPainter::CompositionMode_Source);
                p.drawImage(rect.topLeft(), img);
            }
        }
        m_scrolledRegion = QRegion();
    } else {
        // Copy scrolled image region from client-side memory to server-side pixmap
        ensureGC(m_xcb_pixmap);
        if (hasShm())
            shmPutImage(m_xcb_pixmap, m_scrolledRegion);
        else
            flushPixmap(m_scrolledRegion, true);
    }
}

void QXcbBackingStoreImage::createShmSegment(size_t segmentSize)
{
    Q_ASSERT(connection()->hasShm());
    Q_ASSERT(m_segmentSize == 0);

#ifdef XCB_USE_SHM_FD
    if (connection()->hasShmFd()) {
        if (Q_UNLIKELY(segmentSize > std::numeric_limits<uint32_t>::max())) {
            qCWarning(lcQpaXcb, "xcb_shm_create_segment() can't be called for size %zu, maximum"
                      "allowed size is %u", segmentSize, std::numeric_limits<uint32_t>::max());
            return;
        }

        const auto seg = xcb_generate_id(xcb_connection());
        auto reply = Q_XCB_REPLY(xcb_shm_create_segment,
                                 xcb_connection(), seg, segmentSize, false);
        if (!reply) {
            qCWarning(lcQpaXcb, "xcb_shm_create_segment() failed for size %zu", segmentSize);
            return;
        }

        int *fds = xcb_shm_create_segment_reply_fds(xcb_connection(), reply.get());
        if (reply->nfd != 1) {
            for (int i = 0; i < reply->nfd; i++)
                close(fds[i]);

            qCWarning(lcQpaXcb, "failed to get file descriptor for shm segment of size %zu", segmentSize);
            return;
        }

        void *addr = mmap(nullptr, segmentSize, PROT_READ|PROT_WRITE, MAP_SHARED, fds[0], 0);
        if (addr == MAP_FAILED) {
            qCWarning(lcQpaXcb, "failed to mmap segment from X server (%d: %s) for size %zu",
                     errno, strerror(errno), segmentSize);
            close(fds[0]);
            xcb_shm_detach(xcb_connection(), seg);
            return;
        }

        close(fds[0]);
        m_shm_info.shmseg = seg;
        m_shm_info.shmaddr = static_cast<quint8 *>(addr);
        m_segmentSize = segmentSize;
    } else
#endif
    {
        if (createSystemVShmSegment(xcb_connection(), segmentSize, &m_shm_info))
            m_segmentSize = segmentSize;
    }
}

bool QXcbBackingStoreImage::createSystemVShmSegment(xcb_connection_t *c, size_t segmentSize,
                                                    xcb_shm_segment_info_t *shmInfo)
{
    const int id = shmget(IPC_PRIVATE, segmentSize, IPC_CREAT | 0600);
    if (id == -1) {
        qCWarning(lcQpaXcb, "shmget() failed (%d: %s) for size %zu", errno, strerror(errno), segmentSize);
        return false;
    }

    void *addr = shmat(id, nullptr, 0);
    if (addr == (void *)-1) {
        qCWarning(lcQpaXcb, "shmat() failed (%d: %s) for id %d", errno, strerror(errno), id);
        return false;
    }

    if (shmctl(id, IPC_RMID, nullptr) == -1)
        qCWarning(lcQpaXcb, "Error while marking the shared memory segment to be destroyed");

    const auto seg = xcb_generate_id(c);
    auto cookie = xcb_shm_attach_checked(c, seg, id, false);
    auto *error = xcb_request_check(c, cookie);
    if (error) {
        qCWarning(lcQpaXcb(), "xcb_shm_attach() failed");
        free(error);
        if (shmdt(addr) == -1)
            qCWarning(lcQpaXcb, "shmdt() failed (%d: %s) for %p", errno, strerror(errno), addr);
        return false;
    } else if (!shmInfo) { // this was a test run, free the allocated test segment
        xcb_shm_detach(c, seg);
        auto shmaddr = static_cast<quint8 *>(addr);
        if (shmdt(shmaddr) == -1)
            qCWarning(lcQpaXcb, "shmdt() failed (%d: %s) for %p", errno, strerror(errno), shmaddr);
    }
    if (shmInfo) {
        shmInfo->shmseg = seg;
        shmInfo->shmid = id; // unused
        shmInfo->shmaddr = static_cast<quint8 *>(addr);
    }
    return true;
}

void QXcbBackingStoreImage::destroyShmSegment()
{
    auto cookie = xcb_shm_detach_checked(xcb_connection(), m_shm_info.shmseg);
    xcb_generic_error_t *error = xcb_request_check(xcb_connection(), cookie);
    if (error)
        connection()->printXcbError("xcb_shm_detach() failed with error", error);
    m_shm_info.shmseg = 0;

#ifdef XCB_USE_SHM_FD
    if (connection()->hasShmFd()) {
        if (munmap(m_shm_info.shmaddr, m_segmentSize) == -1) {
            qCWarning(lcQpaXcb, "munmap() failed (%d: %s) for %p with size %zu",
                      errno, strerror(errno), m_shm_info.shmaddr, m_segmentSize);
        }
    } else
#endif
    {
        if (shmdt(m_shm_info.shmaddr) == -1) {
            qCWarning(lcQpaXcb, "shmdt() failed (%d: %s) for %p",
                      errno, strerror(errno), m_shm_info.shmaddr);
        }
        m_shm_info.shmid = 0; // unused
    }
    m_shm_info.shmaddr = nullptr;

    m_segmentSize = 0;
}

extern void qt_scrollRectInImage(QImage &img, const QRect &rect, const QPoint &offset);

bool QXcbBackingStoreImage::scroll(const QRegion &area, int dx, int dy)
{
    const QRect bounds(QPoint(), size());
    const QRegion scrollArea(area & bounds);
    const QPoint delta(dx, dy);

    if (m_clientSideScroll) {
        if (m_qimage.isNull())
            return false;

        if (hasShm())
            preparePaint(scrollArea);

        for (const QRect &rect : scrollArea)
            qt_scrollRectInImage(m_qimage, rect, delta);
    } else {
        if (hasShm())
            shmPutImage(m_xcb_pixmap, m_pendingFlush.intersected(scrollArea));
        else
            flushPixmap(scrollArea);

        ensureGC(m_xcb_pixmap);

        for (const QRect &src : scrollArea) {
            const QRect dst = src.translated(delta).intersected(bounds);
            xcb_copy_area(xcb_connection(),
                          m_xcb_pixmap,
                          m_xcb_pixmap,
                          m_gc,
                          src.x(), src.y(),
                          dst.x(), dst.y(),
                          dst.width(), dst.height());
        }
    }

    m_scrolledRegion |= scrollArea.translated(delta).intersected(bounds);
    if (hasShm()) {
        m_pendingFlush -= scrollArea;
        m_pendingFlush -= m_scrolledRegion;
    }

    return true;
}

void QXcbBackingStoreImage::ensureGC(xcb_drawable_t dst)
{
    if (m_gc_drawable != dst) {
        if (m_gc)
            xcb_free_gc(xcb_connection(), m_gc);

        static const uint32_t mask = XCB_GC_GRAPHICS_EXPOSURES;
        static const uint32_t values[] = { 0 };

        m_gc = xcb_generate_id(xcb_connection());
        xcb_create_gc(xcb_connection(), m_gc, dst, mask, values);

        m_gc_drawable = dst;
    }
}

static inline void copy_unswapped(char *dst, int dstBytesPerLine, const QImage &img, const QRect &rect)
{
    const uchar *srcData = img.constBits();
    const int srcBytesPerLine = img.bytesPerLine();

    const int leftOffset = rect.left() * img.depth() >> 3;
    const int bottom = rect.bottom() + 1;

    for (int yy = rect.top(); yy < bottom; ++yy) {
        const uchar *src = srcData + yy * srcBytesPerLine + leftOffset;
        ::memmove(dst, src, dstBytesPerLine);
        dst += dstBytesPerLine;
    }
}

template <class Pixel>
static inline void copy_swapped(char *dst, const int dstStride, const QImage &img, const QRect &rect)
{
    const uchar *srcData = img.constBits();
    const int srcBytesPerLine = img.bytesPerLine();

    const int left = rect.left();
    const int width = rect.width();
    const int bottom = rect.bottom() + 1;

    for (int yy = rect.top(); yy < bottom; ++yy) {
        Pixel *dstPixels = reinterpret_cast<Pixel *>(dst);
        const Pixel *srcPixels = reinterpret_cast<const Pixel *>(srcData + yy * srcBytesPerLine) + left;

        for (int i = 0; i < width; ++i)
            dstPixels[i] = qbswap<Pixel>(*srcPixels++);

        dst += dstStride;
    }
}

static QImage native_sub_image(QByteArray *buffer, const int dstStride, const QImage &src, const QRect &rect, bool swap)
{
    if (!swap && src.rect() == rect && src.bytesPerLine() == dstStride)
        return src;

    buffer->resize(rect.height() * dstStride);

    if (swap) {
        switch (src.depth()) {
        case 32:
            copy_swapped<quint32>(buffer->data(), dstStride, src, rect);
            break;
        case 16:
            copy_swapped<quint16>(buffer->data(), dstStride, src, rect);
            break;
        }
    } else {
        copy_unswapped(buffer->data(), dstStride, src, rect);
    }

    return QImage(reinterpret_cast<const uchar *>(buffer->constData()), rect.width(), rect.height(), dstStride, src.format());
}

static inline quint32 round_up_scanline(quint32 base, quint32 pad)
{
    return (base + pad - 1) & -pad;
}

void QXcbBackingStoreImage::shmPutImage(xcb_drawable_t drawable, const QRegion &region, const QPoint &offset)
{
    for (const QRect &rect : region) {
        const QPoint source = rect.translated(offset).topLeft();
        xcb_shm_put_image(xcb_connection(),
                          drawable,
                          m_gc,
                          m_xcb_image->width,
                          m_xcb_image->height,
                          source.x(), source.y(),
                          rect.width(), rect.height(),
                          rect.x(), rect.y(),
                          m_xcb_image->depth,
                          m_xcb_image->format,
                          0, // send event?
                          m_shm_info.shmseg,
                          m_xcb_image->data - m_shm_info.shmaddr);
    }
    m_dirtyShm |= region.translated(offset);
}

void QXcbBackingStoreImage::flushPixmap(const QRegion &region, bool fullRegion)
{
    if (!fullRegion) {
        auto actualRegion = m_pendingFlush.intersected(region);
        m_pendingFlush -= region;
        flushPixmap(actualRegion, true);
        return;
    }

    xcb_image_t xcb_subimage;
    memset(&xcb_subimage, 0, sizeof(xcb_image_t));

    xcb_subimage.format = m_xcb_image->format;
    xcb_subimage.scanline_pad = m_xcb_image->scanline_pad;
    xcb_subimage.depth = m_xcb_image->depth;
    xcb_subimage.bpp = m_xcb_image->bpp;
    xcb_subimage.unit = m_xcb_image->unit;
    xcb_subimage.plane_mask = m_xcb_image->plane_mask;
    xcb_subimage.byte_order = (xcb_image_order_t) connection()->setup()->image_byte_order;
    xcb_subimage.bit_order = m_xcb_image->bit_order;

    const bool needsByteSwap = xcb_subimage.byte_order != m_xcb_image->byte_order;
    // Ensure that we don't send more than maxPutImageRequestDataBytes per request.
    const auto maxPutImageRequestDataBytes = connection()->maxRequestDataBytes(sizeof(xcb_put_image_request_t));

    for (const QRect &rect : region) {
        const quint32 stride = round_up_scanline(rect.width() * m_qimage.depth(), xcb_subimage.scanline_pad) >> 3;
        const int rows_per_put = maxPutImageRequestDataBytes / stride;

        // This assert could trigger if a single row has more pixels than fit in
        // a single PutImage request. In the absence of the BIG-REQUESTS extension
        // the theoretical maximum lengths of maxPutImageRequestDataBytes can be
        // roughly 256kB.
        Q_ASSERT(rows_per_put > 0);

        // If we upload the whole image in a single chunk, the result might be
        // larger than the server's maximum request size and stuff breaks.
        // To work around that, we upload the image in chunks where each chunk
        // is small enough for a single request.
        const int x = rect.x();
        int y = rect.y();
        const int width = rect.width();
        int height = rect.height();

        while (height > 0) {
            const int rows = std::min(height, rows_per_put);
            const QRect subRect(x, y, width, rows);
            const QImage subImage = native_sub_image(&m_flushBuffer, stride, m_qimage, subRect, needsByteSwap);

            Q_ASSERT(static_cast<size_t>(subImage.sizeInBytes()) <= maxPutImageRequestDataBytes);

            xcb_subimage.width = width;
            xcb_subimage.height = rows;
            xcb_subimage.data = const_cast<uint8_t *>(subImage.constBits());
            xcb_image_annotate(&xcb_subimage);

            xcb_image_put(xcb_connection(),
                          m_xcb_pixmap,
                          m_gc,
                          &xcb_subimage,
                          x,
                          y,
                          0);

            y += rows;
            height -= rows;
        }
    }
}

void QXcbBackingStoreImage::setClip(const QRegion &region)
{
    if (region.isEmpty()) {
        static const uint32_t mask = XCB_GC_CLIP_MASK;
        static const uint32_t values[] = { XCB_NONE };
        xcb_change_gc(xcb_connection(), m_gc, mask, values);
    } else {
        const auto xcb_rects = qRegionToXcbRectangleList(region);
        xcb_set_clip_rectangles(xcb_connection(),
                                XCB_CLIP_ORDERING_YX_BANDED,
                                m_gc,
                                0, 0,
                                xcb_rects.size(), xcb_rects.constData());
    }
}

void QXcbBackingStoreImage::put(xcb_drawable_t dst, const QRegion &region, const QPoint &offset)
{
    Q_ASSERT(!m_clientSideScroll);

    ensureGC(dst);

    if (hasShm()) {
        setClip(region); // Clip in window local coordinates

        // Copy scrolled area on server-side from pixmap to window
        const QRegion scrolledRegion = m_scrolledRegion.translated(-offset);
        for (const QRect &rect : scrolledRegion) {
            const QPoint source = rect.translated(offset).topLeft();
            xcb_copy_area(xcb_connection(),
                          m_xcb_pixmap,
                          dst,
                          m_gc,
                          source.x(), source.y(),
                          rect.x(), rect.y(),
                          rect.width(), rect.height());
        }

        // Copy non-scrolled image from client-side memory to server-side window
        const QRegion notScrolledArea = region - scrolledRegion;
        shmPutImage(dst, notScrolledArea, offset);
    } else {
        const QRect bounds = region.boundingRect();
        const QPoint target = bounds.topLeft();
        const QRect source = bounds.translated(offset);

        // First clip in backingstore-local coordinates, and upload
        // the changed parts of the backingstore to the server.
        setClip(source);
        flushPixmap(source);

        // Then clip in window local coordinates, and copy the updated
        // parts of the backingstore image server-side to the window.
        setClip(region);
        xcb_copy_area(xcb_connection(),
                      m_xcb_pixmap,
                      dst,
                      m_gc,
                      source.x(), source.y(),
                      target.x(), target.y(),
                      source.width(), source.height());
    }

    setClip(QRegion());
}

void QXcbBackingStoreImage::preparePaint(const QRegion &region)
{
    if (hasShm()) {
        // to prevent X from reading from the image region while we're writing to it
        if (m_dirtyShm.intersects(region)) {
            connection()->sync();
            m_dirtyShm = QRegion();
        }
    }
    m_scrolledRegion -= region;
    m_pendingFlush |= region;
}

bool QXcbBackingStore::createSystemVShmSegment(xcb_connection_t *c, size_t segmentSize, void *shmInfo)
{
    auto info = reinterpret_cast<xcb_shm_segment_info_t *>(shmInfo);
    return QXcbBackingStoreImage::createSystemVShmSegment(c, segmentSize, info);
}

QXcbBackingStore::QXcbBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
{
    QXcbScreen *screen = static_cast<QXcbScreen *>(window->screen()->handle());
    setConnection(screen->connection());
}

QXcbBackingStore::~QXcbBackingStore()
{
    delete m_image;
}

QPaintDevice *QXcbBackingStore::paintDevice()
{
    if (!m_image)
        return nullptr;
    return m_rgbImage.isNull() ? m_image->image() : &m_rgbImage;
}

void QXcbBackingStore::beginPaint(const QRegion &region)
{
    if (!m_image)
        return;

    m_paintRegions.push(region);
    m_image->preparePaint(region);

    if (m_image->hasAlpha()) {
        QPainter p(paintDevice());
        p.setCompositionMode(QPainter::CompositionMode_Source);
        const QColor blank = Qt::transparent;
        for (const QRect &rect : region)
            p.fillRect(rect, blank);
    }
}

void QXcbBackingStore::endPaint()
{
    if (Q_UNLIKELY(m_paintRegions.isEmpty())) {
        qCWarning(lcQpaXcb, "%s: paint regions empty!", Q_FUNC_INFO);
        return;
    }

    const QRegion region = m_paintRegions.pop();
    m_image->preparePaint(region);

    QXcbWindow *platformWindow = static_cast<QXcbWindow *>(window()->handle());
    if (!platformWindow || !platformWindow->imageNeedsRgbSwap())
        return;

    // Slow path: the paint device was m_rgbImage. Now copy with swapping red
    // and blue into m_image.
    auto it = region.begin();
    const auto end = region.end();
    if (it == end)
        return;
    QPainter p(m_image->image());
    while (it != end) {
        const QRect rect = *(it++);
        p.drawImage(rect.topLeft(), m_rgbImage.copy(rect).rgbSwapped());
    }
}

QImage QXcbBackingStore::toImage() const
{
    // If the backingstore is rgbSwapped, return the internal image type here.
    if (!m_rgbImage.isNull())
        return m_rgbImage;
    return m_image && m_image->image() ? *m_image->image() : QImage();
}

QPlatformGraphicsBuffer *QXcbBackingStore::graphicsBuffer() const
{
    return m_image ? m_image->graphicsBuffer() : nullptr;
}

void QXcbBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    if (!m_image || m_image->size().isEmpty())
        return;

    m_image->flushScrolledRegion(false);

    QSize imageSize = m_image->size();

    QRegion clipped = region;
    clipped &= QRect(QPoint(), QHighDpi::toNativePixels(window->size(), window));
    clipped &= QRect(0, 0, imageSize.width(), imageSize.height()).translated(-offset);

    QRect bounds = clipped.boundingRect();

    if (bounds.isNull())
        return;

    QXcbWindow *platformWindow = static_cast<QXcbWindow *>(window->handle());
    if (!platformWindow) {
        qCWarning(lcQpaXcb, "%s QWindow has no platform window, see QTBUG-32681", Q_FUNC_INFO);
        return;
    }

    render(platformWindow->xcb_window(), clipped, offset);

    if (platformWindow->needsSync())
        platformWindow->updateSyncRequestCounter();
    else
        xcb_flush(xcb_connection());
}

void QXcbBackingStore::render(xcb_window_t window, const QRegion &region, const QPoint &offset)
{
    m_image->put(window, region, offset);
}

#ifndef QT_NO_OPENGL
void QXcbBackingStore::composeAndFlush(QWindow *window, const QRegion &region, const QPoint &offset,
                                       QPlatformTextureList *textures,
                                       bool translucentBackground)
{
    if (!m_image || m_image->size().isEmpty())
        return;

    m_image->flushScrolledRegion(true);

    QPlatformBackingStore::composeAndFlush(window, region, offset, textures, translucentBackground);

    QXcbWindow *platformWindow = static_cast<QXcbWindow *>(window->handle());
    if (platformWindow->needsSync()) {
        platformWindow->updateSyncRequestCounter();
    } else {
        xcb_flush(xcb_connection());
    }
}
#endif // QT_NO_OPENGL

void QXcbBackingStore::resize(const QSize &size, const QRegion &)
{
    if (m_image && size == m_image->size())
        return;

    QPlatformWindow *pw = window()->handle();
    if (!pw) {
        window()->create();
        pw = window()->handle();
    }
    QXcbWindow* win = static_cast<QXcbWindow *>(pw);

    recreateImage(win, size);
}

void QXcbBackingStore::recreateImage(QXcbWindow *win, const QSize &size)
{
    if (m_image)
        m_image->resize(size);
    else
        m_image = new QXcbBackingStoreImage(this, size);

    // Slow path for bgr888 VNC: Create an additional image, paint into that and
    // swap R and B while copying to m_image after each paint.
    if (win->imageNeedsRgbSwap()) {
        m_rgbImage = QImage(size, win->imageFormat());
    }
}

bool QXcbBackingStore::scroll(const QRegion &area, int dx, int dy)
{
    if (m_image)
        return m_image->scroll(area, dx, dy);

    return false;
}

QXcbSystemTrayBackingStore::QXcbSystemTrayBackingStore(QWindow *window)
    : QXcbBackingStore(window)
{
    // We need three different behaviors depending on whether the X11 visual
    // for the system tray supports an alpha channel, i.e. is 32 bits, and
    // whether XRender can be used:
    // 1) if the visual has an alpha channel, then render the window's buffer
    //    directly to the X11 window as usual
    // 2) else if XRender can be used, then render the window's buffer to Pixmap,
    //    then render Pixmap's contents to the cleared X11 window with
    //    xcb_render_composite()
    // 3) else grab the X11 window's content and paint it first each time as a
    //    background before rendering the window's buffer to the X11 window

    auto *platformWindow = static_cast<QXcbWindow *>(window->handle());
    quint8 depth = connection()->primaryScreen()->depthOfVisual(platformWindow->visualId());

    if (depth != 32) {
        platformWindow->setParentRelativeBackPixmap();
        initXRenderMode();
        m_useGrabbedBackgound = !m_usingXRenderMode;
    }
}

QXcbSystemTrayBackingStore::~QXcbSystemTrayBackingStore()
{
    if (m_xrenderPicture) {
        xcb_render_free_picture(xcb_connection(), m_xrenderPicture);
        m_xrenderPicture = XCB_NONE;
    }
    if (m_xrenderPixmap) {
        xcb_free_pixmap(xcb_connection(), m_xrenderPixmap);
        m_xrenderPixmap = XCB_NONE;
    }
    if (m_windowPicture) {
        xcb_render_free_picture(xcb_connection(), m_windowPicture);
        m_windowPicture = XCB_NONE;
    }
}

void QXcbSystemTrayBackingStore::beginPaint(const QRegion &region)
{
    QXcbBackingStore::beginPaint(region);

    if (m_useGrabbedBackgound) {
        QPainter p(paintDevice());
        p.setCompositionMode(QPainter::CompositionMode_Source);
        for (const QRect &rect: region)
            p.drawPixmap(rect, m_grabbedBackground, rect);
    }
}

void QXcbSystemTrayBackingStore::render(xcb_window_t window, const QRegion &region, const QPoint &offset)
{
    if (!m_usingXRenderMode) {
        QXcbBackingStore::render(window, region, offset);
        return;
    }

    m_image->put(m_xrenderPixmap, region, offset);
    const QRect bounds = region.boundingRect();
    const QPoint target = bounds.topLeft();
    const QRect source = bounds.translated(offset);
    xcb_clear_area(xcb_connection(), false, window,
                   target.x(), target.y(), source.width(), source.height());
    xcb_render_composite(xcb_connection(), XCB_RENDER_PICT_OP_OVER,
                         m_xrenderPicture, 0, m_windowPicture,
                         target.x(), target.y(), 0, 0, target.x(), target.y(),
                         source.width(), source.height());
}

void QXcbSystemTrayBackingStore::recreateImage(QXcbWindow *win, const QSize &size)
{
    if (!m_usingXRenderMode) {
        QXcbBackingStore::recreateImage(win, size);

        if (m_useGrabbedBackgound) {
            xcb_clear_area(xcb_connection(), false, win->xcb_window(),
                           0, 0, size.width(), size.height());
            m_grabbedBackground = win->xcbScreen()->grabWindow(win->winId(), 0, 0,
                                                               size.width(), size.height());
        }
        return;
    }

    if (m_xrenderPicture) {
        xcb_render_free_picture(xcb_connection(), m_xrenderPicture);
        m_xrenderPicture = XCB_NONE;
    }
    if (m_xrenderPixmap) {
        xcb_free_pixmap(xcb_connection(), m_xrenderPixmap);
        m_xrenderPixmap = XCB_NONE;
    }

    QXcbScreen *screen = win->xcbScreen();

    m_xrenderPixmap = xcb_generate_id(xcb_connection());
    xcb_create_pixmap(xcb_connection(), 32, m_xrenderPixmap, screen->root(), size.width(), size.height());

    m_xrenderPicture = xcb_generate_id(xcb_connection());
    xcb_render_create_picture(xcb_connection(), m_xrenderPicture, m_xrenderPixmap, m_xrenderPictFormat, 0, nullptr);

    // XRender expects premultiplied alpha
    if (m_image)
        m_image->resize(size);
    else
        m_image = new QXcbBackingStoreImage(this, size, 32, QImage::Format_ARGB32_Premultiplied);
}

void QXcbSystemTrayBackingStore::initXRenderMode()
{
    if (!connection()->hasXRender())
        return;

    xcb_connection_t *conn = xcb_connection();
    auto formatsReply = Q_XCB_REPLY(xcb_render_query_pict_formats, conn);

    if (!formatsReply) {
        qWarning("QXcbSystemTrayBackingStore: xcb_render_query_pict_formats() failed");
        return;
    }

    xcb_render_pictforminfo_t *fmt = xcb_render_util_find_standard_format(formatsReply.get(),
                                                                          XCB_PICT_STANDARD_ARGB_32);
    if (!fmt) {
        qWarning("QXcbSystemTrayBackingStore: Failed to find format PICT_STANDARD_ARGB_32");
        return;
    }

    m_xrenderPictFormat = fmt->id;

    auto *platformWindow = static_cast<QXcbWindow *>(window()->handle());
    xcb_render_pictvisual_t *vfmt = xcb_render_util_find_visual_format(formatsReply.get(), platformWindow->visualId());

    if (!vfmt) {
        qWarning("QXcbSystemTrayBackingStore: Failed to find format for visual %x", platformWindow->visualId());
        return;
    }

    m_windowPicture = xcb_generate_id(conn);
    xcb_void_cookie_t cookie =
            xcb_render_create_picture_checked(conn, m_windowPicture, platformWindow->xcb_window(), vfmt->format, 0, nullptr);
    xcb_generic_error_t *error = xcb_request_check(conn, cookie);
    if (error) {
        qWarning("QXcbSystemTrayBackingStore: Failed to create Picture with format %x for window %x, error code %d",
                 vfmt->format, platformWindow->xcb_window(), error->error_code);
        free(error);
        return;
    }

    m_usingXRenderMode = true;
}

QT_END_NAMESPACE
