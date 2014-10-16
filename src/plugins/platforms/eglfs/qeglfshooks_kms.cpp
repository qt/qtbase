/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the qmake spec of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeglfshooks.h"
#include "qeglfsintegration.h"
#include "qeglfsscreen.h"

#include <QtPlatformSupport/private/qdevicediscovery_p.h>
#include <QtCore/private/qcore_unix_p.h>
#include <QtCore/QScopedPointer>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtGui/qpa/qplatformwindow.h>
#include <QtGui/qpa/qplatformcursor.h>
#include <QtGui/QPainter>
#include <QtGui/private/qguiapplication_p.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>

QT_USE_NAMESPACE

struct QEglFSKmsOutput {
    uint32_t conn_id;
    uint32_t crtc_id;
    QSizeF physical_size;
    drmModeModeInfo mode;
    bool mode_set;
    drmModeCrtc *saved_crtc;
};

class QEglFSKmsDevice
{
    Q_DISABLE_COPY(QEglFSKmsDevice)

    QString m_path;
    int m_dri_fd;
    gbm_device *m_gbm_device;

    QList<QEglFSKmsOutput> m_validOutputs;

    bool setup_kms();
    static void pageFlipHandler(int fd,
                                unsigned int sequence,
                                unsigned int tv_sec,
                                unsigned int tv_usec,
                                void *user_data);
public:
    QEglFSKmsDevice(const QString &path);

    bool open();
    void close();

    void createScreens();

    gbm_device *device() const;
    int fd() const;

    void handleDrmEvent();
};

class QEglFSKmsCursor;
class QEglFSKmsScreen : public QEglFSScreen
{
    QEglFSKmsDevice *m_device;
    gbm_surface *m_gbm_surface;

    gbm_bo *m_gbm_bo_current;
    gbm_bo *m_gbm_bo_next;

    bool m_mode_set;

    QEglFSKmsOutput m_output;
    QPoint m_pos;
    QScopedPointer<QEglFSKmsCursor> m_cursor;

    struct FrameBuffer {
        FrameBuffer() : fb(0) {}
        uint32_t fb;
    };
    static void bufferDestroyedHandler(gbm_bo *bo, void *data);
    FrameBuffer *framebufferForBufferObject(gbm_bo *bo);

    static QMutex m_waitForFlipMutex;

public:
    QEglFSKmsScreen(QEglFSKmsDevice *device, QEglFSKmsOutput output, QPoint position);
    ~QEglFSKmsScreen();

    QRect geometry() const Q_DECL_OVERRIDE;
    int depth() const Q_DECL_OVERRIDE;
    QImage::Format format() const Q_DECL_OVERRIDE;

    QSizeF physicalSize() const Q_DECL_OVERRIDE;
    QDpi logicalDpi() const Q_DECL_OVERRIDE;
    Qt::ScreenOrientation nativeOrientation() const Q_DECL_OVERRIDE;
    Qt::ScreenOrientation orientation() const Q_DECL_OVERRIDE;

    QPlatformCursor *cursor() const Q_DECL_OVERRIDE;

    QEglFSKmsDevice *device() const { return m_device; }

    gbm_surface *surface() const { return m_gbm_surface; }
    gbm_surface *createSurface();
    void destroySurface();

    void waitForFlip();
    void flip();
    void flipFinished();

    QEglFSKmsOutput &output() { return m_output; }
    void restoreMode();
};

QMutex QEglFSKmsScreen::m_waitForFlipMutex;

class QEglFSKmsCursor : public QPlatformCursor
{
    Q_OBJECT
public:
    QEglFSKmsCursor(QEglFSKmsScreen *screen);
    ~QEglFSKmsCursor();

    // input methods
    void pointerEvent(const QMouseEvent & event) Q_DECL_OVERRIDE;
#ifndef QT_NO_CURSOR
    void changeCursor(QCursor * windowCursor, QWindow * window) Q_DECL_OVERRIDE;
#endif
    QPoint pos() const Q_DECL_OVERRIDE;
    void setPos(const QPoint &pos) Q_DECL_OVERRIDE;

private:
    void initCursorAtlas();

    QEglFSKmsScreen *m_screen;
    gbm_bo *m_bo;
    QPoint m_pos;
    QPlatformCursorImage m_cursorImage;
    bool m_visible;

    // cursor atlas information
    struct CursorAtlas {
        CursorAtlas() : cursorsPerRow(0), cursorWidth(0), cursorHeight(0) { }
        int cursorsPerRow;
        int width, height; // width and height of the atlas
        int cursorWidth, cursorHeight; // width and height of cursors inside the atlas
        QList<QPoint> hotSpots;
        QImage image;
    } m_cursorAtlas;
};

class QEglKmsHooks : public QEglFSHooks
{
public:
    QEglKmsHooks();

    void platformInit() Q_DECL_OVERRIDE;
    void platformDestroy() Q_DECL_OVERRIDE;
    EGLNativeDisplayType platformDisplay() const Q_DECL_OVERRIDE;
    void screenInit() Q_DECL_OVERRIDE;
    QSurfaceFormat surfaceFormatFor(const QSurfaceFormat &inputFormat) const Q_DECL_OVERRIDE;
    EGLNativeWindowType createNativeWindow(QPlatformWindow *platformWindow,
                                           const QSize &size,
                                           const QSurfaceFormat &format) Q_DECL_OVERRIDE;
    EGLNativeWindowType createNativeOffscreenWindow(const QSurfaceFormat &format) Q_DECL_OVERRIDE;
    void destroyNativeWindow(EGLNativeWindowType window) Q_DECL_OVERRIDE;
    bool hasCapability(QPlatformIntegration::Capability cap) const Q_DECL_OVERRIDE;
    QPlatformCursor *createCursor(QPlatformScreen *screen) const Q_DECL_OVERRIDE;
    void waitForVSync(QPlatformSurface *surface) const Q_DECL_OVERRIDE;
    void presentBuffer(QPlatformSurface *surface) Q_DECL_OVERRIDE;
    bool supportsPBuffers() const Q_DECL_OVERRIDE;

private:
    QEglFSKmsDevice *m_device;
};

static QEglKmsHooks kms_hooks;
QEglFSHooks *platformHooks = &kms_hooks;

QEglKmsHooks::QEglKmsHooks()
    : m_device(Q_NULLPTR)
{}

void QEglKmsHooks::platformInit()
{
    QDeviceDiscovery *d = QDeviceDiscovery::create(QDeviceDiscovery::Device_VideoMask);
    QStringList devices = d->scanConnectedDevices();
    d->deleteLater();

    if (devices.isEmpty())
        qFatal("Could not find DRM device!");

    m_device = new QEglFSKmsDevice(devices.first());
    if (!m_device->open())
        qFatal("DRM device required, aborting");
}

void QEglKmsHooks::platformDestroy()
{
    m_device->close();
    delete m_device;
    m_device = Q_NULLPTR;
}

EGLNativeDisplayType QEglKmsHooks::platformDisplay() const
{
    Q_ASSERT(m_device);
    return reinterpret_cast<EGLNativeDisplayType>(m_device->device());
}

void QEglKmsHooks::screenInit()
{
    m_device->createScreens();
}

QSurfaceFormat QEglKmsHooks::surfaceFormatFor(const QSurfaceFormat &inputFormat) const
{
    QSurfaceFormat format(inputFormat);
    format.setRenderableType(QSurfaceFormat::OpenGLES);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setRedBufferSize(8);
    format.setGreenBufferSize(8);
    format.setBlueBufferSize(8);
    return format;
}

EGLNativeWindowType QEglKmsHooks::createNativeWindow(QPlatformWindow *platformWindow,
                                                     const QSize &size,
                                                     const QSurfaceFormat &format)
{
    Q_UNUSED(size);
    Q_UNUSED(format);

    QEglFSKmsScreen *screen = static_cast<QEglFSKmsScreen *>(platformWindow->screen());
    if (screen->surface()) {
        qWarning("Only single window per screen supported!");
        return 0;
    }

    return reinterpret_cast<EGLNativeWindowType>(screen->createSurface());
}

EGLNativeWindowType QEglKmsHooks::createNativeOffscreenWindow(const QSurfaceFormat &format)
{
    Q_UNUSED(format);
    Q_ASSERT(m_device);

    gbm_surface *surface = gbm_surface_create(m_device->device(),
                                              1, 1,
                                              GBM_FORMAT_XRGB8888,
                                              GBM_BO_USE_RENDERING);

    return reinterpret_cast<EGLNativeWindowType>(surface);
}

void QEglKmsHooks::destroyNativeWindow(EGLNativeWindowType window)
{
    gbm_surface *surface = reinterpret_cast<gbm_surface *>(window);
    gbm_surface_destroy(surface);
}

bool QEglKmsHooks::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case QPlatformIntegration::ThreadedPixmaps:
    case QPlatformIntegration::OpenGL:
    case QPlatformIntegration::ThreadedOpenGL:
        return true;
    default:
        return false;
    }
}

QPlatformCursor *QEglKmsHooks::createCursor(QPlatformScreen *screen) const
{
    Q_UNUSED(screen);
    return Q_NULLPTR;
}

void QEglKmsHooks::waitForVSync(QPlatformSurface *surface) const
{
    QWindow *window = static_cast<QWindow *>(surface->surface());
    QEglFSKmsScreen *screen = static_cast<QEglFSKmsScreen *>(window->screen()->handle());

    screen->waitForFlip();
}

void QEglKmsHooks::presentBuffer(QPlatformSurface *surface)
{
    QWindow *window = static_cast<QWindow *>(surface->surface());
    QEglFSKmsScreen *screen = static_cast<QEglFSKmsScreen *>(window->screen()->handle());

    screen->flip();
}

bool QEglKmsHooks::supportsPBuffers() const
{
    return false;
}

QEglFSKmsCursor::QEglFSKmsCursor(QEglFSKmsScreen *screen)
    : m_screen(screen)
    , m_bo(gbm_bo_create(m_screen->device()->device(), 64, 64, GBM_FORMAT_ARGB8888,
                         GBM_BO_USE_CURSOR_64X64 | GBM_BO_USE_WRITE))
    , m_cursorImage(0, 0, 0, 0, 0, 0)
    , m_visible(true)
{
    if (!m_bo) {
        qWarning("Could not create buffer for cursor!");
    } else {
        initCursorAtlas();
    }

    drmModeMoveCursor(m_screen->device()->fd(), m_screen->output().crtc_id, 0, 0);
}

QEglFSKmsCursor::~QEglFSKmsCursor()
{
    drmModeSetCursor(m_screen->device()->fd(), m_screen->output().crtc_id, 0, 0, 0);
    drmModeMoveCursor(m_screen->device()->fd(), m_screen->output().crtc_id, 0, 0);

    gbm_bo_destroy(m_bo);
    m_bo = Q_NULLPTR;
}

void QEglFSKmsCursor::pointerEvent(const QMouseEvent &event)
{
    setPos(event.screenPos().toPoint());
}

#ifndef QT_NO_CURSOR
void QEglFSKmsCursor::changeCursor(QCursor *windowCursor, QWindow *window)
{
    Q_UNUSED(window);

    if (!m_visible)
        return;

    const Qt::CursorShape newShape = windowCursor ? windowCursor->shape() : Qt::ArrowCursor;
    if (newShape == Qt::BitmapCursor) {
        m_cursorImage.set(windowCursor->pixmap().toImage(),
                          windowCursor->hotSpot().x(),
                          windowCursor->hotSpot().y());
    } else {
        // Standard cursor, look up in atlas
        const int width = m_cursorAtlas.cursorWidth;
        const int height = m_cursorAtlas.cursorHeight;
        const qreal ws = (qreal)m_cursorAtlas.cursorWidth / m_cursorAtlas.width;
        const qreal hs = (qreal)m_cursorAtlas.cursorHeight / m_cursorAtlas.height;

        QRect textureRect(ws * (newShape % m_cursorAtlas.cursorsPerRow) * m_cursorAtlas.width,
                          hs * (newShape / m_cursorAtlas.cursorsPerRow) * m_cursorAtlas.height,
                          width,
                          height);
        QPoint hotSpot = m_cursorAtlas.hotSpots[newShape];
        m_cursorImage.set(m_cursorAtlas.image.copy(textureRect),
                          hotSpot.x(),
                          hotSpot.y());
    }

    if (m_cursorImage.image()->width() > 64 || m_cursorImage.image()->height() > 64)
        qWarning("Cursor larger than 64x64, cursor will be clipped.");

    QImage cursorImage(64, 64, QImage::Format_ARGB32);
    cursorImage.fill(Qt::transparent);

    QPainter painter;
    painter.begin(&cursorImage);
    painter.drawImage(0, 0, *m_cursorImage.image());
    painter.end();

    gbm_bo_write(m_bo, cursorImage.constBits(), cursorImage.byteCount());

    uint32_t handle = gbm_bo_get_handle(m_bo).u32;
    QPoint hot = m_cursorImage.hotspot();
    int status = drmModeSetCursor2(m_screen->device()->fd(), m_screen->output().crtc_id, handle, 64, 64, hot.x(), hot.y());
    if (status != 0)
        qWarning("Could not set cursor: %d", status);
}
#endif // QT_NO_CURSOR

QPoint QEglFSKmsCursor::pos() const
{
    return m_pos;
}

void QEglFSKmsCursor::setPos(const QPoint &pos)
{
    QPoint adjustedPos = pos - m_cursorImage.hotspot();
    int ret = drmModeMoveCursor(m_screen->device()->fd(), m_screen->output().crtc_id, adjustedPos.x(), adjustedPos.y());
    if (ret == 0) {
        m_pos = pos;
    } else {
        qWarning("Failed to move cursor: %d", ret);
    }
}

void QEglFSKmsCursor::initCursorAtlas()
{
    static QByteArray json = qgetenv("QT_QPA_EGLFS_CURSOR");
    if (json.isEmpty())
        json = ":/cursor.json";

    QFile file(QString::fromUtf8(json));
    if (!file.open(QFile::ReadOnly)) {
        drmModeSetCursor(m_screen->device()->fd(), m_screen->output().crtc_id, 0, 0, 0);
        drmModeMoveCursor(m_screen->device()->fd(), m_screen->output().crtc_id, 0, 0);
        m_visible = false;
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject object = doc.object();

    QString atlas = object.value(QLatin1String("image")).toString();
    Q_ASSERT(!atlas.isEmpty());

    const int cursorsPerRow = object.value(QLatin1String("cursorsPerRow")).toDouble();
    Q_ASSERT(cursorsPerRow);
    m_cursorAtlas.cursorsPerRow = cursorsPerRow;

    const QJsonArray hotSpots = object.value(QLatin1String("hotSpots")).toArray();
    Q_ASSERT(hotSpots.count() == Qt::LastCursor + 1);
    for (int i = 0; i < hotSpots.count(); i++) {
        QPoint hotSpot(hotSpots[i].toArray()[0].toDouble(), hotSpots[i].toArray()[1].toDouble());
        m_cursorAtlas.hotSpots << hotSpot;
    }

    QImage image = QImage(atlas).convertToFormat(QImage::Format_ARGB32);
    m_cursorAtlas.cursorWidth = image.width() / m_cursorAtlas.cursorsPerRow;
    m_cursorAtlas.cursorHeight = image.height() / ((Qt::LastCursor + cursorsPerRow) / cursorsPerRow);
    m_cursorAtlas.width = image.width();
    m_cursorAtlas.height = image.height();
    m_cursorAtlas.image = image;
}

void QEglFSKmsScreen::bufferDestroyedHandler(gbm_bo *bo, void *data)
{
    FrameBuffer *fb = static_cast<FrameBuffer *>(data);

    if (fb->fb) {
        gbm_device *device = gbm_bo_get_device(bo);
        drmModeRmFB(gbm_device_get_fd(device), fb->fb);
    }

    delete fb;
}

QEglFSKmsScreen::FrameBuffer *QEglFSKmsScreen::framebufferForBufferObject(gbm_bo *bo)
{
    {
        FrameBuffer *fb = static_cast<FrameBuffer *>(gbm_bo_get_user_data(bo));
        if (fb)
            return fb;
    }

    uint32_t width = gbm_bo_get_width(bo);
    uint32_t height = gbm_bo_get_height(bo);
    uint32_t stride = gbm_bo_get_stride(bo);
    uint32_t handle = gbm_bo_get_handle(bo).u32;

    QScopedPointer<FrameBuffer> fb(new FrameBuffer);

    int ret = drmModeAddFB(m_device->fd(), width, height, 24, 32,
                           stride, handle, &fb->fb);

    if (ret) {
        qWarning("Failed to create KMS FB!");
        return Q_NULLPTR;
    }

    gbm_bo_set_user_data(bo, fb.data(), bufferDestroyedHandler);
    return fb.take();

}

QEglFSKmsScreen::QEglFSKmsScreen(QEglFSKmsDevice *device, QEglFSKmsOutput output, QPoint position)
    : QEglFSScreen(eglGetDisplay(device->device()))
    , m_device(device)
    , m_gbm_surface(Q_NULLPTR)
    , m_gbm_bo_current(Q_NULLPTR)
    , m_gbm_bo_next(Q_NULLPTR)
    , m_output(output)
    , m_pos(position)
    , m_cursor(new QEglFSKmsCursor(this))
{
}

QEglFSKmsScreen::~QEglFSKmsScreen()
{
    restoreMode();
}

QRect QEglFSKmsScreen::geometry() const
{
    return QRect(m_pos.x(), m_pos.y(),
                 m_output.mode.hdisplay,
                 m_output.mode.vdisplay);
}

int QEglFSKmsScreen::depth() const
{
    return 32;
}

QImage::Format QEglFSKmsScreen::format() const
{
    return QImage::Format_RGB32;
}

QSizeF QEglFSKmsScreen::physicalSize() const
{
    return m_output.physical_size;
}

QDpi QEglFSKmsScreen::logicalDpi() const
{
    QSizeF ps = physicalSize();
    QSize s = geometry().size();

    if (ps.isValid() && s.isValid())
        return QDpi(25.4 * s.width() / ps.width(),
                    25.4 * s.height() / ps.height());
    else
        return QDpi(100, 100);
}

Qt::ScreenOrientation QEglFSKmsScreen::nativeOrientation() const
{
    return Qt::PrimaryOrientation;
}

Qt::ScreenOrientation QEglFSKmsScreen::orientation() const
{
    return Qt::PrimaryOrientation;
}

QPlatformCursor *QEglFSKmsScreen::cursor() const
{
    return m_cursor.data();
}

gbm_surface *QEglFSKmsScreen::createSurface()
{
    if (!m_gbm_surface)
        m_gbm_surface = gbm_surface_create(m_device->device(),
                                           geometry().width(),
                                           geometry().height(),
                                           GBM_FORMAT_XRGB8888,
                                           GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    return m_gbm_surface;
}

void QEglFSKmsScreen::destroySurface()
{
    if (m_gbm_bo_current) {
        gbm_bo_destroy(m_gbm_bo_current);
        m_gbm_bo_current = Q_NULLPTR;
    }

    if (m_gbm_bo_next) {
        gbm_bo_destroy(m_gbm_bo_next);
        m_gbm_bo_next = Q_NULLPTR;
    }

    if (m_gbm_surface) {
        gbm_surface_destroy(m_gbm_surface);
        m_gbm_surface = Q_NULLPTR;
    }
}

void QEglFSKmsScreen::waitForFlip()
{
    // Don't lock the mutex unless we actually need to
    if (!m_gbm_bo_next)
        return;

    QMutexLocker lock(&m_waitForFlipMutex);
    while (m_gbm_bo_next)
        m_device->handleDrmEvent();
}

void QEglFSKmsScreen::flip()
{
    if (!m_gbm_surface) {
        qWarning("Cannot sync before platform init!");
        return;
    }

    m_gbm_bo_next = gbm_surface_lock_front_buffer(m_gbm_surface);
    if (!m_gbm_bo_next) {
        qWarning("Could not lock GBM surface front buffer!");
        return;
    }

    FrameBuffer *fb = framebufferForBufferObject(m_gbm_bo_next);

    if (!m_mode_set) {
        m_output.saved_crtc = drmModeGetCrtc(m_device->fd(), m_output.crtc_id);
        int ret = drmModeSetCrtc(m_device->fd(),
                                 m_output.crtc_id,
                                 fb->fb,
                                 0, 0,
                                 &m_output.conn_id, 1,
                                 &m_output.mode);
        if (ret) {
            qErrnoWarning("Could not set DRM mode!");
        } else {
            m_mode_set = true;
        }
    }

    int ret = drmModePageFlip(m_device->fd(),
                              m_output.crtc_id,
                              fb->fb,
                              DRM_MODE_PAGE_FLIP_EVENT,
                              this);
    if (ret) {
        qErrnoWarning("Could not queue DRM page flip!");
        gbm_surface_release_buffer(m_gbm_surface, m_gbm_bo_next);
        m_gbm_bo_next = Q_NULLPTR;
    }
}

void QEglFSKmsScreen::flipFinished()
{
    if (m_gbm_bo_current)
        gbm_surface_release_buffer(m_gbm_surface,
                                   m_gbm_bo_current);

    m_gbm_bo_current = m_gbm_bo_next;
    m_gbm_bo_next = Q_NULLPTR;
}

void QEglFSKmsScreen::restoreMode()
{
    if (m_mode_set && m_output.saved_crtc) {
        drmModeSetCrtc(m_device->fd(),
                       m_output.saved_crtc->crtc_id,
                       m_output.saved_crtc->buffer_id,
                       0, 0,
                       &m_output.conn_id, 1,
                       &m_output.saved_crtc->mode);
        drmModeFreeCrtc(m_output.saved_crtc);
        m_output.saved_crtc = Q_NULLPTR;
        m_mode_set = false;
    }
}

static QList<drmModeConnector *> findConnectors(int dri_fd, drmModeRes *resources)
{
    QList<drmModeConnector *> connectors;

    for (int i = 0; i < resources->count_connectors; i++) {
        drmModeConnector *connector = drmModeGetConnector(dri_fd, resources->connectors[i]);
        if (!connector)
            continue;

        if (connector->connection == DRM_MODE_CONNECTED && connector->count_modes > 0) {
            connectors.append(connector);
            continue;
        }

        drmModeFreeConnector(connector);
    }

    return connectors;
}

static drmModeEncoder *findEncoder(int dri_fd, drmModeRes *resources, uint32_t encoder_id)
{
    for (int i = 0; i < resources->count_encoders; i++) {
        drmModeEncoder *encoder = drmModeGetEncoder(dri_fd, resources->encoders[i]);

        if (!encoder)
            continue;

        if (encoder->encoder_id == encoder_id)
            return encoder;

        drmModeFreeEncoder(encoder);
    }

    return Q_NULLPTR;
}

bool QEglFSKmsDevice::setup_kms()
{
    drmModeRes *resources = drmModeGetResources(m_dri_fd);
    if (!resources) {
       qWarning("drmModeGetResources failed");
       return false;
    }

    QList<drmModeConnector *> connectors = findConnectors(m_dri_fd, resources);
    if (connectors.isEmpty()) {
        qWarning("No currently active connectors found");
        return false;
    }

    while (!connectors.isEmpty()) {
        drmModeConnector *connector = connectors.takeFirst();
        drmModeEncoder *encoder = findEncoder(m_dri_fd, resources, connector->encoder_id);

        if (!encoder) {
            drmModeFreeConnector(connector);
            continue;
        }

        QEglFSKmsOutput output = {
            connector->connector_id,
            encoder->crtc_id,
            QSizeF(connector->mmWidth, connector->mmHeight),
            connector->modes[0],
            false,
            Q_NULLPTR
        };

        drmModeFreeEncoder(encoder);
        drmModeFreeConnector(connector);

        m_validOutputs.append(output);
    }

    drmModeFreeResources(resources);

    return m_validOutputs.size() > 0;
}

void QEglFSKmsDevice::pageFlipHandler(int fd, unsigned int sequence, unsigned int tv_sec, unsigned int tv_usec, void *user_data)
{
    Q_UNUSED(fd);
    Q_UNUSED(sequence);
    Q_UNUSED(tv_sec);
    Q_UNUSED(tv_usec);

    QEglFSKmsScreen *screen = static_cast<QEglFSKmsScreen *>(user_data);
    screen->flipFinished();
}

QEglFSKmsDevice::QEglFSKmsDevice(const QString &path)
    : m_path(path)
    , m_dri_fd(-1)
    , m_gbm_device(Q_NULLPTR)
{
}

bool QEglFSKmsDevice::open()
{
    Q_ASSERT(m_dri_fd == -1);
    Q_ASSERT(m_gbm_device == Q_NULLPTR);

    m_dri_fd = qt_safe_open(m_path.toLocal8Bit().constData(), O_RDWR | O_CLOEXEC);
    if (m_dri_fd == -1) {
        qErrnoWarning("Could not open DRM device %s", qPrintable(m_path));
        return false;
    }

    m_gbm_device = gbm_create_device(m_dri_fd);
    if (!m_gbm_device) {
        qErrnoWarning("Could not create GBM device");
        qt_safe_close(m_dri_fd);
        m_dri_fd = -1;
        return false;
    }

    return true;
}

void QEglFSKmsDevice::close()
{
    if (m_gbm_device) {
        gbm_device_destroy(m_gbm_device);
        m_gbm_device = Q_NULLPTR;
    }

    if (m_dri_fd != -1) {
        qt_safe_close(m_dri_fd);
        m_dri_fd = -1;
    }
}

void QEglFSKmsDevice::createScreens()
{
    if (!setup_kms())
        return;

    QEglFSIntegration *integration = static_cast<QEglFSIntegration *>(QGuiApplicationPrivate::platformIntegration());
    QPoint pos;
    foreach (const QEglFSKmsOutput &output, m_validOutputs) {
        integration->addScreen(new QEglFSKmsScreen(this, output, pos));
        pos.rx() += output.mode.hdisplay;
    }
}

gbm_device *QEglFSKmsDevice::device() const
{
    return m_gbm_device;
}

int QEglFSKmsDevice::fd() const
{
    return m_dri_fd;
}

void QEglFSKmsDevice::handleDrmEvent()
{
    drmEventContext drmEvent = {
        DRM_EVENT_CONTEXT_VERSION,
        Q_NULLPTR,      // vblank handler
        pageFlipHandler // page flip handler
    };

    drmHandleEvent(m_dri_fd, &drmEvent);
}

#include "qeglfshooks_kms.moc"
