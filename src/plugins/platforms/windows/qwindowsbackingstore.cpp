// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsbackingstore.h"
#include "qwindowswindow.h"
#include "qwindowscontext.h"

#include <QtGui/qwindow.h>
#include <QtGui/qpainter.h>
#include <QtGui/private/qwindowsnativeimage_p.h>
#include <private/qhighdpiscaling_p.h>
#include <private/qimage_p.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

/*!
    \class QWindowsBackingStore
    \brief Backing store for windows.
    \internal
*/

QWindowsBackingStore::QWindowsBackingStore(QWindow *window) :
    QPlatformBackingStore(window),
    m_alphaNeedsFill(false)
{
    qCDebug(lcQpaBackingStore) << __FUNCTION__ << this << window;
}

QWindowsBackingStore::~QWindowsBackingStore()
{
    qCDebug(lcQpaBackingStore) << __FUNCTION__ << this;
}

QPaintDevice *QWindowsBackingStore::paintDevice()
{
    Q_ASSERT(!m_image.isNull());
    return &m_image->image();
}

void QWindowsBackingStore::flush(QWindow *window, const QRegion &region,
                                 const QPoint &offset)
{
    Q_ASSERT(window);

    const QRect br = region.boundingRect();
    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaBackingStore) << __FUNCTION__ << this << window << offset << br;
    QWindowsWindow *rw = QWindowsWindow::windowsWindowOf(window);
    Q_ASSERT(rw);

    const bool hasAlpha = rw->format().hasAlpha();
    const Qt::WindowFlags flags = window->flags();
    // Note: This condition must be in sync with setWindowOpacity. FIXME: Improve this :)
    if (rw->isLayered() && hasAlpha && QWindowsWindow::hasNoNativeFrame(rw->handle(), flags)) {
        // Windows with alpha: Use blend function to update.
        QRect r = QHighDpi::toNativePixels(window->frameGeometry(), window);
        QMargins frameMargins = rw->frameMargins();
        QRect dirtyRect = br.translated(offset + QPoint(frameMargins.left(), frameMargins.top()));

        SIZE size = {r.width(), r.height()};
        POINT ptDst = {r.x(), r.y()};
        POINT ptSrc = {0, 0};
        BLENDFUNCTION blend = {AC_SRC_OVER, 0, BYTE(qRound(255.0 * rw->opacity())), AC_SRC_ALPHA};
        RECT dirty = {dirtyRect.x(), dirtyRect.y(),
                      dirtyRect.x() + dirtyRect.width(), dirtyRect.y() + dirtyRect.height()};
        UPDATELAYEREDWINDOWINFO info = {sizeof(info), nullptr, &ptDst, &size,
                                        m_image->hdc(), &ptSrc, 0, &blend, ULW_ALPHA, &dirty};
        const BOOL result = UpdateLayeredWindowIndirect(rw->handle(), &info);
        if (!result)
            qErrnoWarning("UpdateLayeredWindowIndirect failed for ptDst=(%d, %d),"
                          " size=(%dx%d), dirty=(%dx%d %d, %d)", r.x(), r.y(),
                          r.width(), r.height(), dirtyRect.width(), dirtyRect.height(),
                          dirtyRect.x(), dirtyRect.y());
    } else {
        const HDC dc = rw->getDC();
        if (!dc) {
            qErrnoWarning("%s: GetDC failed", __FUNCTION__);
            return;
        }

        if (!BitBlt(dc, br.x(), br.y(), br.width(), br.height(),
                    m_image->hdc(), br.x() + offset.x(), br.y() + offset.y(), SRCCOPY)) {
            const DWORD lastError = GetLastError(); // QTBUG-35926, QTBUG-29716: may fail after lock screen.
            if (lastError != ERROR_SUCCESS && lastError != ERROR_INVALID_HANDLE)
                qErrnoWarning(int(lastError), "%s: BitBlt failed", __FUNCTION__);
        }
        rw->releaseDC();
    }

    // Write image for debug purposes.
    if (QWindowsContext::verbose > 2 && lcQpaBackingStore().isDebugEnabled()) {
        static int n = 0;
        const QString fileName = QString::fromLatin1("win%1_%2.png").
                arg(rw->winId()).arg(n++);
        m_image->image().save(fileName);
        qCDebug(lcQpaBackingStore) << "Wrote " << m_image->image().size() << fileName;
    }
}

void QWindowsBackingStore::resize(const QSize &size, const QRegion &region)
{
    if (m_image.isNull() || m_image->image().size() != size) {
#ifndef QT_NO_DEBUG_OUTPUT
        if (QWindowsContext::verbose && lcQpaBackingStore().isDebugEnabled()) {
            qCDebug(lcQpaBackingStore)
                << __FUNCTION__ << ' ' << window() << ' ' << size << ' ' << region
                << " from: " << (m_image.isNull() ? QSize() : m_image->image().size());
        }
#endif
        QImage::Format format = window()->format().hasAlpha() ?
                    QImage::Format_ARGB32_Premultiplied : QWindowsNativeImage::systemFormat();

        // The backingstore composition (enabling render-to-texture widgets)
        // punches holes in the backingstores using the alpha channel. Hence
        // the need for a true alpha format.
        if (QImage::toPixelFormat(format).alphaUsage() == QPixelFormat::UsesAlpha)
            m_alphaNeedsFill = true;
        else // upgrade but here we know app painting does not rely on alpha hence no need to fill
            format = qt_maybeDataCompatibleAlphaVersion(format);

        QWindowsNativeImage *oldwni = m_image.data();
        auto *newwni = new QWindowsNativeImage(size.width(), size.height(), format);

        if (oldwni && !region.isEmpty()) {
            const QImage &oldimg(oldwni->image());
            QImage &newimg(newwni->image());
            QRegion staticRegion(region);
            staticRegion &= QRect(0, 0, oldimg.width(), oldimg.height());
            staticRegion &= QRect(0, 0, newimg.width(), newimg.height());
            QPainter painter(&newimg);
            painter.setCompositionMode(QPainter::CompositionMode_Source);
            for (const QRect &rect : staticRegion)
                painter.drawImage(rect, oldimg, rect);
        }

        m_image.reset(newwni);
    }
}

Q_GUI_EXPORT void qt_scrollRectInImage(QImage &img, const QRect &rect, const QPoint &offset);

bool QWindowsBackingStore::scroll(const QRegion &area, int dx, int dy)
{
    if (m_image.isNull() || m_image->image().isNull())
        return false;

    const QPoint offset(dx, dy);
    const QRect rect = area.boundingRect();
    qt_scrollRectInImage(m_image->image(), rect, offset);

    return true;
}

void QWindowsBackingStore::beginPaint(const QRegion &region)
{
    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaBackingStore) <<__FUNCTION__ << region;

    if (m_alphaNeedsFill) {
        QPainter p(&m_image->image());
        p.setCompositionMode(QPainter::CompositionMode_Source);
        const QColor blank = Qt::transparent;
        for (const QRect &r : region)
            p.fillRect(r, blank);
    }
}

HDC QWindowsBackingStore::getDC() const
{
    if (!m_image.isNull())
        return m_image->hdc();
    return nullptr;
}

QImage QWindowsBackingStore::toImage() const
{
    if (m_image.isNull()) {
        qCWarning(lcQpaBackingStore) <<__FUNCTION__ << "Image is null.";
        return QImage();
    }
    return m_image.data()->image();
}

QT_END_NAMESPACE
