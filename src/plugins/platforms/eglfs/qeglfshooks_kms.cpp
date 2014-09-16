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
#include <QtPlatformSupport/private/qdevicediscovery_p.h>
#include <QtCore/private/qcore_unix_p.h>
#include <QtCore/QScopedPointer>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtGui/qpa/qplatformwindow.h>
#include <QtGui/qpa/qplatformcursor.h>
#include <QtGui/QPainter>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>

QT_USE_NAMESPACE

class QKmsCursor : public QPlatformCursor
{
    Q_OBJECT
public:
    QKmsCursor(gbm_device *gbm_device, int dri_fd, uint32_t crtcId);
    ~QKmsCursor();

    // input methods
    void pointerEvent(const QMouseEvent & event) Q_DECL_OVERRIDE;
#ifndef QT_NO_CURSOR
    void changeCursor(QCursor * windowCursor, QWindow * window) Q_DECL_OVERRIDE;
#endif
    QPoint pos() const Q_DECL_OVERRIDE;
    void setPos(const QPoint &pos) Q_DECL_OVERRIDE;

private:
    void initCursorAtlas();

    gbm_device *m_gbm_device;
    int m_dri_fd;
    uint32_t m_crtc;
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
    QSizeF physicalScreenSize() const Q_DECL_OVERRIDE;
    QSize screenSize() const Q_DECL_OVERRIDE;
    int screenDepth() const Q_DECL_OVERRIDE;
    QSurfaceFormat surfaceFormatFor(const QSurfaceFormat &inputFormat) const Q_DECL_OVERRIDE;
    EGLNativeWindowType createNativeWindow(QPlatformWindow *platformWindow,
                                           const QSize &size,
                                           const QSurfaceFormat &format) Q_DECL_OVERRIDE;
    EGLNativeWindowType createNativeOffscreenWindow(const QSurfaceFormat &format) Q_DECL_OVERRIDE;
    void destroyNativeWindow(EGLNativeWindowType window) Q_DECL_OVERRIDE;
    bool hasCapability(QPlatformIntegration::Capability cap) const Q_DECL_OVERRIDE;
    QPlatformCursor *createCursor(QPlatformScreen *screen) const Q_DECL_OVERRIDE;
    void presentBuffer() Q_DECL_OVERRIDE;
    bool supportsPBuffers() const Q_DECL_OVERRIDE;

private:
    bool setup_kms();

    struct FrameBuffer {
        FrameBuffer() : fb(0) {}
        uint32_t fb;
    };
    static void bufferDestroyedHandler(gbm_bo *bo, void *data);
    FrameBuffer *framebufferForBufferObject(gbm_bo *bo);

    static void pageFlipHandler(int fd,
                                unsigned int sequence,
                                unsigned int tv_sec,
                                unsigned int tv_usec,
                                void *user_data);

private:
    // device bits
    QByteArray m_device;
    int m_dri_fd;
    gbm_device *m_gbm_device;

    // KMS bits
    drmModeConnector *m_drm_connector;
    drmModeEncoder *m_drm_encoder;
    drmModeModeInfo m_drm_mode;
    quint32 m_drm_crtc;

    // Drawing bits
    gbm_surface *m_gbm_surface;
    gbm_bo *m_gbm_bo_current;
    gbm_bo *m_gbm_bo_next;
    bool m_flipping;
    bool m_mode_set;
};

static QEglKmsHooks kms_hooks;
QEglFSHooks *platformHooks = &kms_hooks;

QEglKmsHooks::QEglKmsHooks()
    : m_dri_fd(-1)
    , m_gbm_device(Q_NULLPTR)
    , m_drm_connector(Q_NULLPTR)
    , m_drm_encoder(Q_NULLPTR)
    , m_drm_crtc(0)
    , m_gbm_surface(Q_NULLPTR)
    , m_gbm_bo_current(Q_NULLPTR)
    , m_gbm_bo_next(Q_NULLPTR)
    , m_flipping(false)
    , m_mode_set(false)
{

}

void QEglKmsHooks::platformInit()
{
    QDeviceDiscovery *d = QDeviceDiscovery::create(QDeviceDiscovery::Device_VideoMask);
    QStringList devices = d->scanConnectedDevices();
    d->deleteLater();

    if (devices.isEmpty())
        qFatal("Could not find DRM device!");

    m_device = devices.first().toLocal8Bit();
    m_dri_fd = qt_safe_open(m_device.constData(), O_RDWR | O_CLOEXEC);
    if (m_dri_fd == -1) {
        qErrnoWarning("Could not open DRM device %s", m_device.constData());
        qFatal("DRM device required, aborting.");
    }

    if (!setup_kms())
        qFatal("Could not set up KMS on device %s!", m_device.constData());

    m_gbm_device = gbm_create_device(m_dri_fd);
    if (!m_gbm_device)
        qFatal("Could not initialize gbm on device %s!", m_device.constData());
}

void QEglKmsHooks::platformDestroy()
{
    gbm_device_destroy(m_gbm_device);
    m_gbm_device = Q_NULLPTR;

    if (qt_safe_close(m_dri_fd) == -1)
        qErrnoWarning("Could not close DRM device %s", m_device.constData());

    m_dri_fd = -1;
}

EGLNativeDisplayType QEglKmsHooks::platformDisplay() const
{
    return static_cast<EGLNativeDisplayType>(m_gbm_device);
}

QSizeF QEglKmsHooks::physicalScreenSize() const
{
    return QSizeF(m_drm_connector->mmWidth,
                  m_drm_connector->mmHeight);
}

QSize QEglKmsHooks::screenSize() const
{
    return QSize(m_drm_mode.hdisplay,
                 m_drm_mode.vdisplay);
}

int QEglKmsHooks::screenDepth() const
{
    return 32;
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
    Q_UNUSED(platformWindow);
    Q_UNUSED(size);
    Q_UNUSED(format);

    if (m_gbm_surface) {
        qWarning("Only single window apps supported!");
        return 0;
    }

    m_gbm_surface = gbm_surface_create(m_gbm_device,
                                       screenSize().width(),
                                       screenSize().height(),
                                       GBM_FORMAT_XRGB8888,
                                       GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (!m_gbm_surface)
        qFatal("Could not initialize GBM surface");

    return reinterpret_cast<EGLNativeWindowType>(m_gbm_surface);
}

EGLNativeWindowType QEglKmsHooks::createNativeOffscreenWindow(const QSurfaceFormat &format)
{
    Q_UNUSED(format);

    gbm_surface *surface = gbm_surface_create(m_gbm_device,
                                              1, 1,
                                              GBM_FORMAT_XRGB8888,
                                              GBM_BO_USE_RENDERING);

    return reinterpret_cast<EGLNativeWindowType>(surface);
}

void QEglKmsHooks::destroyNativeWindow(EGLNativeWindowType window)
{
    gbm_surface *surface = reinterpret_cast<gbm_surface *>(window);
    if (surface == m_gbm_surface)
        m_gbm_surface = Q_NULLPTR;
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
    return new QKmsCursor(m_gbm_device, m_dri_fd, m_drm_crtc);
}

void QEglKmsHooks::bufferDestroyedHandler(gbm_bo *bo, void *data)
{
    QEglKmsHooks::FrameBuffer *fb = static_cast<QEglKmsHooks::FrameBuffer *>(data);

    if (fb->fb) {
        gbm_device *device = gbm_bo_get_device(bo);
        drmModeRmFB(gbm_device_get_fd(device), fb->fb);
    }

    delete fb;
}

QEglKmsHooks::FrameBuffer *QEglKmsHooks::framebufferForBufferObject(gbm_bo *bo)
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

    int ret = drmModeAddFB(m_dri_fd, width, height, 24, 32,
                           stride, handle, &fb->fb);

    if (ret) {
        qWarning("Failed to create KMS FB!");
        return Q_NULLPTR;
    }

    gbm_bo_set_user_data(bo, fb.data(), bufferDestroyedHandler);
    return fb.take();
}

void QEglKmsHooks::pageFlipHandler(int fd,
                                   unsigned int sequence,
                                   unsigned int tv_sec,
                                   unsigned int tv_usec,
                                   void *user_data)
{
    Q_UNUSED(fd);
    Q_UNUSED(sequence);
    Q_UNUSED(tv_sec);
    Q_UNUSED(tv_usec);

    QEglKmsHooks *hooks = static_cast<QEglKmsHooks *>(user_data);

    if (hooks->m_gbm_bo_current)
        gbm_surface_release_buffer(hooks->m_gbm_surface,
                                   hooks->m_gbm_bo_current);

    hooks->m_gbm_bo_current = hooks->m_gbm_bo_next;
    hooks->m_gbm_bo_next = Q_NULLPTR;

    // We are no longer flipping
    hooks->m_flipping = false;
}

void QEglKmsHooks::presentBuffer()
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

    QEglKmsHooks::FrameBuffer *fb = framebufferForBufferObject(m_gbm_bo_next);

    if (!m_mode_set) {
        int ret = drmModeSetCrtc(m_dri_fd,
                                 m_drm_crtc,
                                 fb->fb,
                                 0, 0,
                                 &m_drm_connector->connector_id, 1,
                                 &m_drm_mode);
        if (ret) {
            qErrnoWarning("Could not set DRM mode!");
        } else {
            m_mode_set = true;
        }
    }

    int ret = drmModePageFlip(m_dri_fd,
                              m_drm_encoder->crtc_id,
                              fb->fb,
                              DRM_MODE_PAGE_FLIP_EVENT,
                              this);
    if (ret) {
        qErrnoWarning("Could not queue DRM page flip!");
        return;
    }

    m_flipping = true;

    drmEventContext drmEvent = {
        DRM_EVENT_CONTEXT_VERSION,
        Q_NULLPTR,          // vblank handler
        pageFlipHandler     // page flip handler
    };

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(m_dri_fd, &fds);

    while (m_flipping) {
        ret = qt_safe_select(m_dri_fd + 1, &fds, Q_NULLPTR, Q_NULLPTR, Q_NULLPTR);

        if (ret == 0) {
            // timeout
        } else if (ret == -1) {
            qErrnoWarning("Error while selecting on DRM fd");
            break;
        } else if (drmHandleEvent(m_dri_fd, &drmEvent)) {
            qWarning("Could not handle DRM event!");
        }
    }
}

bool QEglKmsHooks::supportsPBuffers() const
{
    return false;
}

bool QEglKmsHooks::setup_kms()
{
    drmModeRes *resources;
    drmModeConnector *connector;
    drmModeEncoder *encoder;
    quint32 crtc = 0;
    int i;

    resources = drmModeGetResources(m_dri_fd);
    if (!resources) {
       qWarning("drmModeGetResources failed");
       return false;
    }

    for (i = 0; i < resources->count_connectors; i++) {
       connector = drmModeGetConnector(m_dri_fd, resources->connectors[i]);
       if (connector == NULL)
          continue;

       if (connector->connection == DRM_MODE_CONNECTED &&
           connector->count_modes > 0) {
          break;
       }

       drmModeFreeConnector(connector);
    }

    if (i == resources->count_connectors) {
       qWarning("No currently active connector found.");
       return false;
    }

    for (i = 0; i < resources->count_encoders; i++) {
       encoder = drmModeGetEncoder(m_dri_fd, resources->encoders[i]);

       if (encoder == NULL)
          continue;

       if (encoder->encoder_id == connector->encoder_id)
          break;

       drmModeFreeEncoder(encoder);
    }

    for (int j = 0; j < resources->count_crtcs; j++) {
        if ((encoder->possible_crtcs & (1 << j))) {
            crtc = resources->crtcs[j];
            break;
        }
    }

    if (crtc == 0)
        qFatal("No suitable CRTC available");

    m_drm_connector = connector;
    m_drm_encoder = encoder;
    m_drm_mode = connector->modes[0];
    m_drm_crtc = crtc;

    drmModeFreeResources(resources);

    return true;
}


QKmsCursor::QKmsCursor(gbm_device *gbm_device, int dri_fd, uint32_t crtcId)
    : m_gbm_device(gbm_device)
    , m_dri_fd(dri_fd)
    , m_crtc(crtcId)
    , m_bo(gbm_bo_create(gbm_device, 64, 64, GBM_FORMAT_ARGB8888,
                         GBM_BO_USE_CURSOR_64X64 | GBM_BO_USE_WRITE))
    , m_cursorImage(0, 0, 0, 0, 0, 0)
    , m_visible(true)
{
    if (!m_bo) {
        qWarning("Could not create buffer for cursor!");
    } else {
        initCursorAtlas();
    }

    drmModeMoveCursor(m_dri_fd, m_crtc, 0, 0);
}

QKmsCursor::~QKmsCursor()
{
    drmModeSetCursor(m_dri_fd, m_crtc, 0, 0, 0);
    drmModeMoveCursor(m_dri_fd, m_crtc, 0, 0);

    gbm_bo_destroy(m_bo);
    m_bo = Q_NULLPTR;
}

void QKmsCursor::pointerEvent(const QMouseEvent &event)
{
    setPos(event.screenPos().toPoint());
}

#ifndef QT_NO_CURSOR
void QKmsCursor::changeCursor(QCursor *windowCursor, QWindow *window)
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
    int status = drmModeSetCursor2(m_dri_fd, m_crtc, handle, 64, 64, hot.x(), hot.y());
    if (status != 0)
        qWarning("Could not set cursor: %d", status);
}
#endif // QT_NO_CURSOR

QPoint QKmsCursor::pos() const
{
    return m_pos;
}

void QKmsCursor::setPos(const QPoint &pos)
{
    QPoint adjustedPos = pos - m_cursorImage.hotspot();
    int ret = drmModeMoveCursor(m_dri_fd, m_crtc, adjustedPos.x(), adjustedPos.y());
    if (ret == 0) {
        m_pos = pos;
    } else {
        qWarning("Failed to move cursor: %d", ret);
    }
}

void QKmsCursor::initCursorAtlas()
{
    static QByteArray json = qgetenv("QT_QPA_EGLFS_CURSOR");
    if (json.isEmpty())
        json = ":/cursor.json";

    QFile file(QString::fromUtf8(json));
    if (!file.open(QFile::ReadOnly)) {
        drmModeSetCursor(m_dri_fd, m_crtc, 0, 0, 0);
        drmModeMoveCursor(m_dri_fd, m_crtc, 0, 0);
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

#include "qeglfshooks_kms.moc"
