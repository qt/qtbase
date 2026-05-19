// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default
#include "qwaylandshmbackingstore_p.h"
#include "qwaylandwindow_p.h"
#include "qwaylandsubsurface_p.h"
#include "qwaylanddisplay_p.h"
#include "qwaylandscreen_p.h"
#include "qwaylandabstractdecoration_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qtemporaryfile.h>
#include <QtGui/QPainter>
#include <QtGui/QTransform>
#include <QMutexLocker>

#include <QtWaylandClient/private/wayland-wayland-client-protocol.h>

#include <memory>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#ifdef Q_OS_LINUX
#  include <sys/syscall.h>
// from linux/memfd.h:
#  ifndef MFD_CLOEXEC
#    define MFD_CLOEXEC     0x0001U
#  endif
#  ifndef MFD_ALLOW_SEALING
#    define MFD_ALLOW_SEALING 0x0002U
#  endif
// from bits/fcntl-linux.h
#  ifndef F_ADD_SEALS
#    define F_ADD_SEALS 1033
#  endif
#  ifndef F_SEAL_SEAL
#    define F_SEAL_SEAL 0x0001
#  endif
#  ifndef F_SEAL_SHRINK
#    define F_SEAL_SHRINK 0x0002
#  endif
#endif

QT_BEGIN_NAMESPACE

extern void qt_scrollRectInImage(QImage &, const QRect &, const QPoint &);

namespace QtWaylandClient {

static int alignTo(int input, int alignment)
{
    Q_ASSERT(alignment > 0);
    if (int remainder = input % alignment)
        return input + (alignment - remainder);
    else
        return input;
}

QWaylandShmBuffer::QWaylandShmBuffer(QWaylandDisplay *display,
                                     const QSize &size, QImage::Format format, qreal scale, wl_event_queue *customEventQueue)
    : mDirtyRegion(QRect(QPoint(0, 0), size / scale))
{
    // This alignment of stride and size is done to improve performance of
    // buffer accesses on the compositor side.
    // Aligning the size of the shm pool to pages means the buffer can be
    // imported as a udmabuf, and if the stride is additionally compatible with
    // the GPU, that udmabuf can be used directly for rendering instead of needing
    // to first copy to a GPU-accessible buffer.
    // The 256 bytes stride alignment used here is what all common GPUs can read from.
    const int stride = alignTo(size.width() * 4, 256);
    const int alloc = alignTo(stride * size.height(), getpagesize());
    int fd = -1;

#ifdef SYS_memfd_create
    fd = syscall(SYS_memfd_create, "wayland-shm", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (fd >= 0)
        fcntl(fd, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_SEAL);
#endif

    std::unique_ptr<QFile> filePointer;
    bool opened;

    if (fd == -1) {
        auto tmpFile =
            std::make_unique<QTemporaryFile>(QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation) +
                                       QLatin1String("/wayland-shm-XXXXXX"));
        opened = tmpFile->open();
        filePointer = std::move(tmpFile);
    } else {
        auto file = std::make_unique<QFile>();
        opened = file->open(fd, QIODevice::ReadWrite | QIODevice::Unbuffered, QFile::AutoCloseHandle);
        filePointer = std::move(file);
    }
    // NOTE beginPaint assumes a new buffer be all zeroes, which QFile::resize does.
    if (!opened || !filePointer->resize(alloc)) {
        qWarning("QWaylandShmBuffer: failed: %s", qUtf8Printable(filePointer->errorString()));
        return;
    }
    fd = filePointer->handle();

    // map ourselves: QFile::map() will unmap when the object is destroyed,
    // but we want this mapping to persist (unmapping in destructor)
    uchar *data = (uchar *)
            mmap(nullptr, alloc, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == (uchar *) MAP_FAILED) {
        qErrnoWarning("QWaylandShmBuffer: mmap failed");
        return;
    }

    QWaylandShm* shm = display->shm();
    wl_shm_format wl_format = shm->formatFrom(format);
    mImage = QImage(data, size.width(), size.height(), stride, format);
    mImage.setDevicePixelRatio(scale);

    mShmPool = wl_shm_create_pool(shm->object(), fd, alloc);
    init(wl_shm_pool_create_buffer(mShmPool,0, size.width(), size.height(),
                                       stride, wl_format));
    if (customEventQueue)
        wl_proxy_set_queue(reinterpret_cast<struct wl_proxy *>(buffer()), customEventQueue);
}

QWaylandShmBuffer::~QWaylandShmBuffer(void)
{
    delete mMarginsImage;
    if (mImage.constBits())
        munmap((void *) mImage.constBits(), mImage.sizeInBytes());
    if (mShmPool)
        wl_shm_pool_destroy(mShmPool);
}

QImage *QWaylandShmBuffer::imageInsideMargins(const QMargins &marginsIn)
{
    QMargins margins = marginsIn * mImage.devicePixelRatio();

    if (!margins.isNull() && margins != mMargins) {
        if (mMarginsImage) {
            delete mMarginsImage;
        }
        uchar *bits = const_cast<uchar *>(mImage.constBits());
        uchar *b_s_data = bits + margins.top() * mImage.bytesPerLine() + margins.left() * 4;
        int b_s_width = mImage.size().width() - margins.left() - margins.right();
        int b_s_height = mImage.size().height() - margins.top() - margins.bottom();
        mMarginsImage = new QImage(b_s_data, b_s_width,b_s_height,mImage.bytesPerLine(),mImage.format());
        mMarginsImage->setDevicePixelRatio(mImage.devicePixelRatio());
    }
    if (margins.isNull()) {
        delete mMarginsImage;
        mMarginsImage = nullptr;
    }

    mMargins = margins;
    if (!mMarginsImage)
        return &mImage;

    return mMarginsImage;

}

QWaylandShmBackingStore::QWaylandShmBackingStore(QWindow *window, QWaylandDisplay *display)
    : QPlatformBackingStore(window)
    , mDisplay(display)
{
    mEventQueue = wl_display_create_queue(mDisplay->wl_display());
    QObject::connect(mDisplay, &QWaylandDisplay::connected, window, [this]() {
        auto oldEventQueue = mEventQueue;
        mEventQueue = wl_display_create_queue(mDisplay->wl_display());
        auto copy = mBuffers;
        // clear available buffers so we create new ones
        // actual deletion is deferred till after resize call so we can copy
        // contents from the back buffer
        mBuffers.clear();
        mFrontBuffer = nullptr;
        // recreateBackBufferIfNeeded always resets mBackBuffer
        if (mRequestedSize.isValid() && waylandWindow())
            recreateBackBufferIfNeeded();
        else
            mBackBuffer = nullptr;
        qDeleteAll(copy);
        wl_event_queue_destroy(oldEventQueue);
    });
}

QWaylandShmBackingStore::~QWaylandShmBackingStore()
{
    if (QWaylandWindow *w = waylandWindow())
        w->setBackingStore(nullptr);

//    if (mFrontBuffer == waylandWindow()->attached())
//        waylandWindow()->attach(0);

    qDeleteAll(mBuffers);
    wl_event_queue_destroy(mEventQueue);
}

QPaintDevice *QWaylandShmBackingStore::paintDevice()
{
    return contentSurface();
}

void QWaylandShmBackingStore::updateDirtyStates(const QRegion &region)
{
    // Update dirty state of buffers based on what was painted. The back buffer will be
    // less dirty, since we painted to it, while other buffers will become more dirty.
    // This allows us to minimize copies between front and back buffers on swap in the
    // cases where the painted region overlaps with the previous frame (front buffer).
    for (QWaylandShmBuffer *b : std::as_const(mBuffers)) {
        if (b != mBackBuffer)
            b->dirtyRegion() += region;
        else
            b->dirtyRegion() -= region;
    }
}

void QWaylandShmBackingStore::beginPaint(const QRegion &region)
{
    mPainting = true;
    waylandWindow()->setBackingStore(this);

    const QMargins margins = windowDecorationMargins();
    const QRegion regionTranslated = region.translated(margins.left(), margins.top());
    const bool bufferWasRecreated = recreateBackBufferIfNeeded();
    updateDirtyStates(regionTranslated);

    // Although undocumented, QBackingStore::beginPaint expects the painted region
    // to be cleared before use if the window has a surface format with an alpha.
    // Fresh QWaylandShmBuffer are already cleared, so we don't need to clear those.
    if (!bufferWasRecreated && window()->format().hasAlpha()) {
        QPainter p(paintDevice());
        p.setCompositionMode(QPainter::CompositionMode_Source);
        const QColor blank = Qt::transparent;
        for (const QRect &rect : region)
            p.fillRect(rect, blank);
    }
}

void QWaylandShmBackingStore::endPaint()
{
    mPainting = false;
    if (mPendingFlush)
        flush(window(), mPendingRegion, QPoint());
}

// Inspired by QCALayerBackingStore.
bool QWaylandShmBackingStore::scroll(const QRegion &region, int dx, int dy)
{
    if (Q_UNLIKELY(!mBackBuffer))
        return false;

    const qreal devicePixelRatio = waylandWindow()->scale();

    // On Wayland, the window can have a device pixel ratio different from
    // the window/screen, therefore we cannot rely on QHighDpi here, cf. QBackingStore::scroll.
    // With fractional scaling we cannot easily scroll the existing pixels.
    if (!qFuzzyIsNull(devicePixelRatio - static_cast<int>(devicePixelRatio)))
        return false;

    recreateBackBufferIfNeeded();
    if (!mFrontBuffer)
        return false;

    const QPoint scrollDelta(dx, dy);
    const QMargins margins = windowDecorationMargins();
    const QRegion adjustedRegion = region.translated(margins.left(), margins.top());

    const QRegion inPlaceRegion = adjustedRegion - mBackBuffer->dirtyRegion();
    const QRegion frontBufferRegion = adjustedRegion - inPlaceRegion;

    if (!inPlaceRegion.isEmpty()) {
        const QRect inPlaceBoundingRect = inPlaceRegion.boundingRect();
        const QPoint devicePixelDelta = scrollDelta * devicePixelRatio;

        qt_scrollRectInImage(*mBackBuffer->image(),
                             QRect(inPlaceBoundingRect.topLeft() * devicePixelRatio,
                                   inPlaceBoundingRect.size() * devicePixelRatio),
                             devicePixelDelta);
    }

    if (!frontBufferRegion.isEmpty()) {
        QPainter painter(mBackBuffer->image());
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.scale(qreal(1) / devicePixelRatio, qreal(1) / devicePixelRatio);
        for (const QRect &rect : frontBufferRegion) {
            QRect sourceRect(rect.topLeft() * devicePixelRatio,
                             rect.size() * devicePixelRatio);
            QRect destinationRect((rect.topLeft() + scrollDelta) * devicePixelRatio,
                                   rect.size() * devicePixelRatio);
            painter.drawImage(destinationRect, *mFrontBuffer->image(), sourceRect);
        }
    }

    // We do not mark the source region as dirty, even though it technically has "moved".
    // This matches the behavior of other backingstore implementations using qt_scrollRectInImage.
    updateDirtyStates(adjustedRegion.translated(scrollDelta));

    return true;
}

void QWaylandShmBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    // Invoked when the window is of type RasterSurface.

    if (window != this->window()) {
        auto waylandWindow = static_cast<QWaylandWindow *>(window->handle());
        const auto scale = waylandWindow->scale();
        auto newBuffer = new QWaylandShmBuffer(
                mDisplay, window->size() * scale, mBackBuffer->image()->format(),
                mBackBuffer->image()->devicePixelRatio());
        newBuffer->setDeleteOnRelease(true);
        QRect sourceRect(offset * scale, window->size() * scale);
        QPainter painter(newBuffer->image());
        painter.drawImage(QPoint(0, 0), *mBackBuffer->image(), sourceRect);
        waylandWindow->safeCommit(newBuffer, region);
        return;
    }

    if (mPainting) {
        mPendingRegion |= region;
        mPendingFlush = true;
        return;
    }

    mPendingFlush = false;
    mPendingRegion = QRegion();

    if (windowDecoration() && windowDecoration()->isDirty())
        updateDecorations();

    finalizeBackBuffer();

    mFrontBuffer = mBackBuffer;

    QMargins margins = windowDecorationMargins();
    waylandWindow()->safeCommit(mFrontBuffer, region.translated(margins.left(), margins.top()));
}

void QWaylandShmBackingStore::resize(const QSize &size, const QRegion &)
{
    mRequestedSize = size;
}

QWaylandShmBuffer *QWaylandShmBackingStore::getBuffer(const QSize &size, bool &bufferWasRecreated)
{
    static const int MAX_BUFFERS = 5;
    static const int MAX_AGE = 10 * MAX_BUFFERS;
    bufferWasRecreated = false;

    // Prune buffers that have not been used in a while. The front buffer must not be touched
    // so its data can be copied to new buffers. The back buffer can be pruned but we don't
    // do it because we will probably need it anyway if the front buffer is used by the compositor.
    for (auto i = mBuffers.size() - 1; i >= 0; --i) {
        QWaylandShmBuffer *buffer = mBuffers[i];
        if (mBackBuffer == buffer)
            continue;
        if (mFrontBuffer == buffer)
            continue;

        if (buffer->age() > MAX_AGE) {
            mBuffers.removeAt(i);
            delete buffer;
        }
    }

    // Prune buffers that have mismatching size.
    for (auto i = mBuffers.size() - 1; i >= 0; --i) {
        QWaylandShmBuffer *buffer = mBuffers[i];
        if (buffer->size() != size) {
            mBuffers.removeAt(i);
            if (mBackBuffer == buffer)
                mBackBuffer = nullptr;
            if (mFrontBuffer == buffer)
                mFrontBuffer = nullptr;
            delete buffer;
        }
    }

    QWaylandShmBuffer *buffer = nullptr;
    for (QWaylandShmBuffer *candidate : std::as_const(mBuffers)) {
        if (candidate->busy())
            continue;

        if (!buffer || candidate->age() < buffer->age())
            buffer = candidate;
    }

    if (buffer)
        return buffer;

    if (mBuffers.size() < MAX_BUFFERS) {
        QImage::Format format = QImage::Format_ARGB32_Premultiplied;
        if (!waylandWindow()->format().hasAlpha())
            format = QImage::Format_RGB32;
        QWaylandShmBuffer *b = new QWaylandShmBuffer(mDisplay, size, format, waylandWindow()->scale(), mEventQueue);
        bufferWasRecreated = true;
        mBuffers.push_front(b);
        return b;
    }
    return nullptr;
}

bool QWaylandShmBackingStore::recreateBackBufferIfNeeded()
{
    wl_display_dispatch_queue_pending(mDisplay->wl_display(), mEventQueue);

    bool bufferWasRecreated = false;
    QMargins margins = windowDecorationMargins();
    qreal scale = waylandWindow()->scale();
    const QSize sizeWithMargins = (mRequestedSize + QSize(margins.left() + margins.right(), margins.top() + margins.bottom())) * scale;

    // We look for a free buffer to draw into. If the buffer is not the last buffer we used,
    // that is mBackBuffer, and the size is the same we copy the damaged content into the new
    // buffer so that QPainter is happy to find the stuff it had drawn before. If the new
    // buffer has a different size it needs to be redrawn completely anyway, and if the buffer
    // is the same the stuff is there already.
    // You can exercise the different codepaths with weston, switching between the gl and the
    // pixman renderer. With the gl renderer release events are sent early so we can effectively
    // run single buffered, while with the pixman renderer we have to use two.
    QWaylandShmBuffer *buffer = getBuffer(sizeWithMargins, bufferWasRecreated);
    while (!buffer) {
        struct ::wl_display *display = mDisplay->wl_display();
        if (wl_display_dispatch_queue(display, mEventQueue) < 0) {
            int ecode = wl_display_get_error(display);
            if ((ecode == EPIPE || ecode == ECONNRESET))
                qWarning("The Wayland connection broke during blocking read event. Did the Wayland compositor die?");
            else
                qWarning("The Wayland connection experienced a fatal error during blocking read event: %s", strerror(ecode));
            _exit(-1);
        }
        buffer = getBuffer(sizeWithMargins, bufferWasRecreated);
    }

    qsizetype oldSizeInBytes = mBackBuffer ? mBackBuffer->image()->sizeInBytes() : 0;
    qsizetype newSizeInBytes = buffer->image()->sizeInBytes();

    mBackBuffer = buffer;

    for (QWaylandShmBuffer *buffer : std::as_const(mBuffers)) {
        if (mBackBuffer == buffer) {
            buffer->setAge(0);
        } else {
            buffer->setAge(buffer->age() + 1);
        }
    }

    if (windowDecoration() && window()->isVisible() && oldSizeInBytes != newSizeInBytes)
        windowDecoration()->update();

    return bufferWasRecreated;
}

void QWaylandShmBackingStore::finalizeBackBuffer()
{
    Q_ASSERT(mBackBuffer);

    const QRegion clipRegion = mBackBuffer->dirtyRegion();
    if (clipRegion.isEmpty())
        return;

    if (Q_UNLIKELY(!mFrontBuffer || mFrontBuffer == mBackBuffer))
        return;

    const QImage *sourceImage = mFrontBuffer->image();
    QImage *targetImage = mBackBuffer->image();

    QPainter painter(targetImage);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    const qreal targetDevicePixelRatio = painter.device()->devicePixelRatio();
    const auto clipRects = clipRegion.rects();
    for (const QRect &clipRect : clipRects) { // Iterate clip rects, because complicated clip region causes higher CPU usage
        if (clipRects.size() > 1)
            painter.save();
        painter.setClipRect(clipRect);
        painter.scale(qreal(1) / targetDevicePixelRatio, qreal(1) / targetDevicePixelRatio);
        painter.drawImage(QRectF(QPointF(), targetImage->size()), *sourceImage, sourceImage->rect());
        if (clipRects.size() > 1)
            painter.restore();
    }

    mBackBuffer->dirtyRegion() = QRegion();
}

QImage *QWaylandShmBackingStore::entireSurface() const
{
    return mBackBuffer->image();
}

QImage *QWaylandShmBackingStore::contentSurface() const
{
    return windowDecoration() ? mBackBuffer->imageInsideMargins(windowDecorationMargins()) : mBackBuffer->image();
}

void QWaylandShmBackingStore::updateDecorations()
{
    QPainter decorationPainter(entireSurface());
    decorationPainter.setCompositionMode(QPainter::CompositionMode_Source);
    QImage sourceImage = windowDecoration()->contentImage();

    qreal dp = sourceImage.devicePixelRatio();
    int dpWidth = int(sourceImage.width() / dp);
    int dpHeight = int(sourceImage.height() / dp);
    QTransform sourceMatrix;
    sourceMatrix.scale(dp, dp);
    QRect target; // needs to be in device independent pixels
    QRegion dirtyRegion;

    //Top
    target.setX(0);
    target.setY(0);
    target.setWidth(dpWidth);
    target.setHeight(windowDecorationMargins().top());
    decorationPainter.drawImage(target, sourceImage, sourceMatrix.mapRect(target));
    dirtyRegion += target;

    //Left
    target.setWidth(windowDecorationMargins().left());
    target.setHeight(dpHeight);
    decorationPainter.drawImage(target, sourceImage, sourceMatrix.mapRect(target));
    dirtyRegion += target;

    //Right
    target.setX(dpWidth - windowDecorationMargins().right());
    target.setWidth(windowDecorationMargins().right());
    decorationPainter.drawImage(target, sourceImage, sourceMatrix.mapRect(target));
    dirtyRegion += target;

    //Bottom
    target.setX(0);
    target.setY(dpHeight - windowDecorationMargins().bottom());
    target.setWidth(dpWidth);
    target.setHeight(windowDecorationMargins().bottom());
    decorationPainter.drawImage(target, sourceImage, sourceMatrix.mapRect(target));
    dirtyRegion += target;

    updateDirtyStates(dirtyRegion);
}

QWaylandAbstractDecoration *QWaylandShmBackingStore::windowDecoration() const
{
    return waylandWindow()->decoration();
}

QMargins QWaylandShmBackingStore::windowDecorationMargins() const
{
    if (windowDecoration())
        return windowDecoration()->margins();
    return QMargins();
}

QWaylandWindow *QWaylandShmBackingStore::waylandWindow() const
{
    return static_cast<QWaylandWindow *>(window()->handle());
}

#if QT_CONFIG(opengl)
QImage QWaylandShmBackingStore::toImage() const
{
    // Invoked from QPlatformBackingStore::composeAndFlush() that is called
    // instead of flush() for widgets that have renderToTexture children
    // (QOpenGLWidget, QQuickWidget).

    const_cast<QWaylandShmBackingStore *>(this)->finalizeBackBuffer();

    return *contentSurface();
}
#endif  // opengl

}

QT_END_NAMESPACE
