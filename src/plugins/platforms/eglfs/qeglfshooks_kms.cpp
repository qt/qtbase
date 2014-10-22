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

#include <QtPlatformSupport/private/qeglplatformcursor_p.h>
#include <QtPlatformSupport/private/qdevicediscovery_p.h>
#include <QtCore/private/qcore_unix_p.h>
#include <QtCore/QScopedPointer>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QLoggingCategory>
#include <QtGui/qpa/qplatformwindow.h>
#include <QtGui/qpa/qplatformcursor.h>
#include <QtGui/QPainter>
#include <QtGui/private/qguiapplication_p.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>

#ifndef DRM_CAP_CURSOR_WIDTH
#define DRM_CAP_CURSOR_WIDTH 0x8
#endif

#ifndef DRM_CAP_CURSOR_HEIGHT
#define DRM_CAP_CURSOR_HEIGHT 0x9
#endif

#define ARRAY_LENGTH(a) (sizeof (a) / sizeof (a)[0])

QT_USE_NAMESPACE

Q_LOGGING_CATEGORY(qLcEglfsKmsDebug, "qt.qpa.eglfs.kms")

class QEglFSKmsCursor;
class QEglFSKmsScreen;

enum OutputConfiguration {
    OutputConfigOff,
    OutputConfigPreferred,
    OutputConfigCurrent,
    OutputConfigMode,
    OutputConfigModeline
};

struct QEglFSKmsOutput {
    QString name;
    uint32_t connector_id;
    uint32_t crtc_id;
    QSizeF physical_size;
    int mode; // index of selected mode in list below
    bool mode_set;
    drmModeCrtcPtr saved_crtc;
    QList<drmModeModeInfo> modes;
};

class QEglFSKmsDevice
{
    Q_DISABLE_COPY(QEglFSKmsDevice)

    QString m_path;
    int m_dri_fd;
    gbm_device *m_gbm_device;

    quint32 m_crtc_allocator;
    quint32 m_connector_allocator;

    int crtcForConnector(drmModeResPtr resources, drmModeConnectorPtr connector);
    QEglFSKmsScreen *screenForConnector(drmModeResPtr resources, drmModeConnectorPtr connector, QPoint pos);

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

class QEglFSKmsScreen : public QEglFSScreen
{
    QEglFSKmsDevice *m_device;
    gbm_surface *m_gbm_surface;

    gbm_bo *m_gbm_bo_current;
    gbm_bo *m_gbm_bo_next;

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

    QString name() const Q_DECL_OVERRIDE;

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
    QSize m_cursorSize;
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

    bool hwCursor() const;
    QMap<QString, QVariantMap> outputSettings() const;

private:
    void loadConfig();

    QEglFSKmsDevice *m_device;

    bool m_hwCursor;
    bool m_pbuffers;
    QString m_devicePath;
    QMap<QString, QVariantMap> m_outputSettings;
};

static QEglKmsHooks kms_hooks;
QEglFSHooks *platformHooks = &kms_hooks;

QEglKmsHooks::QEglKmsHooks()
    : m_device(Q_NULLPTR)
    , m_hwCursor(true)
    , m_pbuffers(false)
{}

void QEglKmsHooks::platformInit()
{
    loadConfig();

    if (!m_devicePath.isEmpty()) {
        qCDebug(qLcEglfsKmsDebug) << "Using DRM device" << m_devicePath << "specified in config file";
    } else {

        QDeviceDiscovery *d = QDeviceDiscovery::create(QDeviceDiscovery::Device_VideoMask);
        QStringList devices = d->scanConnectedDevices();
        qCDebug(qLcEglfsKmsDebug) << "Found the following video devices:" << devices;
        d->deleteLater();

        if (devices.isEmpty())
            qFatal("Could not find DRM device!");

        m_devicePath = devices.first();
        qCDebug(qLcEglfsKmsDebug) << "Using" << m_devicePath;
    }

    m_device = new QEglFSKmsDevice(m_devicePath);
    if (!m_device->open())
        qFatal("Could not open device %s - aborting!", qPrintable(m_devicePath));
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

    qCDebug(qLcEglfsKmsDebug) << "Creating native off screen window";
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
    if (m_hwCursor)
        return Q_NULLPTR;
    else
        return new QEGLPlatformCursor(screen);
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
    return m_pbuffers;
}

bool QEglKmsHooks::hwCursor() const
{
    return m_hwCursor;
}

QMap<QString, QVariantMap> QEglKmsHooks::outputSettings() const
{
    return m_outputSettings;
}

void QEglKmsHooks::loadConfig()
{
    static QByteArray json = qgetenv("QT_QPA_EGLFS_KMS_CONFIG");
    if (json.isEmpty())
        return;

    qCDebug(qLcEglfsKmsDebug) << "Loading KMS setup from" << json;

    QFile file(QString::fromUtf8(json));
    if (!file.open(QFile::ReadOnly)) {
        qCDebug(qLcEglfsKmsDebug) << "Could not open config file"
                                  << json << "for reading";
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        qCDebug(qLcEglfsKmsDebug) << "Invalid config file" << json
                                  << "- no top-level JSON object";
        return;
    }

    QJsonObject object = doc.object();

    m_hwCursor = object.value("hwcursor").toBool(m_hwCursor);
    m_pbuffers = object.value("pbuffers").toBool(m_pbuffers);
    m_devicePath = object.value("device").toString();

    QJsonArray outputs = object.value("outputs").toArray();
    for (int i = 0; i < outputs.size(); i++) {
        QVariantMap outputSettings = outputs.at(i).toObject().toVariantMap();

        if (outputSettings.contains("name")) {
            QString name = outputSettings.value("name").toString();

            if (m_outputSettings.contains(name)) {
                qCDebug(qLcEglfsKmsDebug) << "Output" << name << "configured multiple times!";
            }

            m_outputSettings.insert(name, outputSettings);
        }
    }

    qCDebug(qLcEglfsKmsDebug) << "Configuration:\n"
                              << "\thwcursor:" << m_hwCursor << "\n"
                              << "\tpbuffers:" << m_pbuffers << "\n"
                              << "\toutputs:" << m_outputSettings;
}

QEglFSKmsCursor::QEglFSKmsCursor(QEglFSKmsScreen *screen)
    : m_screen(screen)
    , m_cursorSize(64, 64) // 64x64 is the old standard size, we now try to query the real size below
    , m_bo(Q_NULLPTR)
    , m_cursorImage(0, 0, 0, 0, 0, 0)
    , m_visible(true)
{
    uint64_t width, height;
    if ((drmGetCap(m_screen->device()->fd(), DRM_CAP_CURSOR_WIDTH, &width) == 0)
        && (drmGetCap(m_screen->device()->fd(), DRM_CAP_CURSOR_HEIGHT, &height) == 0)) {
        m_cursorSize.setWidth(width);
        m_cursorSize.setHeight(height);
    }

    m_bo = gbm_bo_create(m_screen->device()->device(), m_cursorSize.width(), m_cursorSize.height(),
                         GBM_FORMAT_ARGB8888, GBM_BO_USE_CURSOR_64X64 | GBM_BO_USE_WRITE);
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

    if (m_cursorImage.image()->width() > m_cursorSize.width() || m_cursorImage.image()->height() > m_cursorSize.height())
        qWarning("Cursor larger than %dx%d, cursor will be clipped.", m_cursorSize.width(), m_cursorSize.height());

    QImage cursorImage(m_cursorSize, QImage::Format_ARGB32);
    cursorImage.fill(Qt::transparent);

    QPainter painter;
    painter.begin(&cursorImage);
    painter.drawImage(0, 0, *m_cursorImage.image());
    painter.end();

    gbm_bo_write(m_bo, cursorImage.constBits(), cursorImage.byteCount());

    uint32_t handle = gbm_bo_get_handle(m_bo).u32;
    int status = drmModeSetCursor(m_screen->device()->fd(), m_screen->output().crtc_id, handle,
                                  m_cursorSize.width(), m_cursorSize.height());
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

    qCDebug(qLcEglfsKmsDebug) << "Initializing cursor atlas from" << json;

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
    : QEglFSScreen(eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(device->device())))
    , m_device(device)
    , m_gbm_surface(Q_NULLPTR)
    , m_gbm_bo_current(Q_NULLPTR)
    , m_gbm_bo_next(Q_NULLPTR)
    , m_output(output)
    , m_pos(position)
    , m_cursor(Q_NULLPTR)
{
}

QEglFSKmsScreen::~QEglFSKmsScreen()
{
    restoreMode();
}

QRect QEglFSKmsScreen::geometry() const
{
    const int mode = m_output.mode;
    return QRect(m_pos.x(), m_pos.y(),
                 m_output.modes[mode].hdisplay,
                 m_output.modes[mode].vdisplay);
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

QString QEglFSKmsScreen::name() const
{
    return m_output.name;
}

QPlatformCursor *QEglFSKmsScreen::cursor() const
{
    if (kms_hooks.hwCursor()) {
        if (m_cursor.isNull()) {
            QEglFSKmsScreen *that = const_cast<QEglFSKmsScreen *>(this);
            that->m_cursor.reset(new QEglFSKmsCursor(that));
        }

        return m_cursor.data();
    } else {
        return QEglFSScreen::cursor();
    }
}

gbm_surface *QEglFSKmsScreen::createSurface()
{
    if (!m_gbm_surface) {
        qCDebug(qLcEglfsKmsDebug) << "Creating window for screen" << name();
        m_gbm_surface = gbm_surface_create(m_device->device(),
                                           geometry().width(),
                                           geometry().height(),
                                           GBM_FORMAT_XRGB8888,
                                           GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    }
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

    if (!m_output.mode_set) {
        int ret = drmModeSetCrtc(m_device->fd(),
                                 m_output.crtc_id,
                                 fb->fb,
                                 0, 0,
                                 &m_output.connector_id, 1,
                                 &m_output.modes[m_output.mode]);

        if (ret)
            qErrnoWarning("Could not set DRM mode!");
        else
            m_output.mode_set = true;
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
    if (m_output.mode_set && m_output.saved_crtc) {
        drmModeSetCrtc(m_device->fd(),
                       m_output.saved_crtc->crtc_id,
                       m_output.saved_crtc->buffer_id,
                       0, 0,
                       &m_output.connector_id, 1,
                       &m_output.saved_crtc->mode);

        drmModeFreeCrtc(m_output.saved_crtc);
        m_output.saved_crtc = Q_NULLPTR;

        m_output.mode_set = false;
    }
}

int QEglFSKmsDevice::crtcForConnector(drmModeResPtr resources, drmModeConnectorPtr connector)
{
    for (int i = 0; i < connector->count_encoders; i++) {
        drmModeEncoderPtr encoder = drmModeGetEncoder(m_dri_fd, connector->encoders[i]);
        if (!encoder) {
            qWarning("Failed to get encoder");
            continue;
        }

        quint32 possibleCrtcs = encoder->possible_crtcs;
        drmModeFreeEncoder(encoder);

        for (int j = 0; j < resources->count_crtcs; j++) {
            bool isPossible = possibleCrtcs & (1 << j);
            bool isAvailable = !(m_crtc_allocator & 1 << resources->crtcs[j]);

            if (isPossible && isAvailable)
                return j;
        }
    }

    return -1;
}

static const char * const connector_type_names[] = {
    "None",
    "VGA",
    "DVI",
    "DVI",
    "DVI",
    "Composite",
    "TV",
    "LVDS",
    "CTV",
    "DIN",
    "DP",
    "HDMI",
    "HDMI",
    "TV",
    "eDP",
};

static QString nameForConnector(const drmModeConnectorPtr connector)
{
    QString connectorName = "UNKNOWN";

    if (connector->connector_type < ARRAY_LENGTH(connector_type_names))
        connectorName = connector_type_names[connector->connector_type];

    connectorName += QString::number(connector->connector_type_id);

    return connectorName;
}

static bool parseModeline(const QString &s, drmModeModeInfoPtr mode)
{
    char hsync[16];
    char vsync[16];
    float fclock;

    mode->type = DRM_MODE_TYPE_USERDEF;
    mode->hskew = 0;
    mode->vscan = 0;
    mode->vrefresh = 0;
    mode->flags = 0;

    if (sscanf(qPrintable(s), "%f %hd %hd %hd %hd %hd %hd %hd %hd %15s %15s",
               &fclock,
               &mode->hdisplay,
               &mode->hsync_start,
               &mode->hsync_end,
               &mode->htotal,
               &mode->vdisplay,
               &mode->vsync_start,
               &mode->vsync_end,
               &mode->vtotal, hsync, vsync) != 11)
        return false;

    mode->clock = fclock * 1000;

    if (strcmp(hsync, "+hsync") == 0)
        mode->flags |= DRM_MODE_FLAG_PHSYNC;
    else if (strcmp(hsync, "-hsync") == 0)
        mode->flags |= DRM_MODE_FLAG_NHSYNC;
    else
        return false;

    if (strcmp(vsync, "+vsync") == 0)
        mode->flags |= DRM_MODE_FLAG_PVSYNC;
    else if (strcmp(vsync, "-vsync") == 0)
        mode->flags |= DRM_MODE_FLAG_NVSYNC;
    else
        return false;

    return true;
}

QEglFSKmsScreen *QEglFSKmsDevice::screenForConnector(drmModeResPtr resources, drmModeConnectorPtr connector, QPoint pos)
{
    const QString connectorName = nameForConnector(connector);

    const int crtc = crtcForConnector(resources, connector);
    if (crtc < 0) {
        qWarning() << "No usable crtc/encoder pair for connector" << connectorName;
        return Q_NULLPTR;
    }

    OutputConfiguration configuration;
    QSize configurationSize;
    drmModeModeInfo configurationModeline;

    const QString mode = kms_hooks.outputSettings().value(connectorName).value("mode", "preferred").toString().toLower();
    if (mode == "off") {
        configuration = OutputConfigOff;
    } else if (mode == "preferred") {
        configuration = OutputConfigPreferred;
    } else if (mode == "current") {
        configuration = OutputConfigCurrent;
    } else if (sscanf(qPrintable(mode), "%dx%d", &configurationSize.rwidth(), &configurationSize.rheight()) == 2) {
        configuration = OutputConfigMode;
    } else if (parseModeline(mode, &configurationModeline)) {
        configuration = OutputConfigModeline;
    } else {
        qWarning("Invalid mode \"%s\" for output %s", qPrintable(mode), qPrintable(connectorName));
        configuration = OutputConfigPreferred;
    }

    const uint32_t crtc_id = resources->crtcs[crtc];

    if (configuration == OutputConfigOff) {
        qCDebug(qLcEglfsKmsDebug) << "Turning off output" << connectorName;
        drmModeSetCrtc(m_dri_fd, crtc_id, 0, 0, 0, 0, 0, Q_NULLPTR);
        return Q_NULLPTR;
    }

    // Get the current mode on the current crtc
    drmModeModeInfo crtc_mode;
    memset(&crtc_mode, 0, sizeof crtc_mode);
    if (drmModeEncoderPtr encoder = drmModeGetEncoder(m_dri_fd, connector->connector_id)) {
        drmModeCrtcPtr crtc = drmModeGetCrtc(m_dri_fd, encoder->crtc_id);
        drmModeFreeEncoder(encoder);

        if (!crtc)
            return Q_NULLPTR;

        if (crtc->mode_valid)
            crtc_mode = crtc->mode;

        drmModeFreeCrtc(crtc);
    }

    QList<drmModeModeInfo> modes;
    qCDebug(qLcEglfsKmsDebug) << connectorName << "mode count:" << connector->count_modes;
    for (int i = 0; i < connector->count_modes; i++) {
        const drmModeModeInfo &mode = connector->modes[i];
        qCDebug(qLcEglfsKmsDebug) << "mode" << i << mode.hdisplay << "x" << mode.vdisplay
                                  << "@" << mode.vrefresh << "hz";
        modes << connector->modes[i];
    }

    int preferred = -1;
    int current = -1;
    int configured = -1;
    int best = -1;

    for (int i = modes.size() - 1; i >= 0; i--) {
        const drmModeModeInfo &m = modes.at(i);

        if (configuration == OutputConfigMode &&
                m.hdisplay == configurationSize.width() &&
                m.vdisplay == configurationSize.height()) {
            configured = i;
        }

        if (!memcmp(&crtc_mode, &m, sizeof m))
            current = i;

        if (m.type & DRM_MODE_TYPE_PREFERRED)
            preferred = i;

        best = i;
    }

    if (configuration == OutputConfigModeline) {
        modes << configurationModeline;
        configured = modes.size() - 1;
    }

    if (current < 0 && crtc_mode.clock != 0) {
        modes << crtc_mode;
        current = mode.size() - 1;
    }

    if (configuration == OutputConfigCurrent)
        configured = current;

    int selected_mode = -1;

    if (configured >= 0)
        selected_mode = configured;
    else if (preferred >= 0)
        selected_mode = preferred;
    else if (current >= 0)
        selected_mode = current;
    else if (best >= 0)
        selected_mode = best;

    if (selected_mode < 0) {
        qWarning() << "No modes available for output" << connectorName;
        return Q_NULLPTR;
    } else {
        int width = modes[selected_mode].hdisplay;
        int height = modes[selected_mode].vdisplay;
        int refresh = modes[selected_mode].vrefresh;
        qCDebug(qLcEglfsKmsDebug) << "Selected mode" << selected_mode << ":" << width << "x" << height
                                  << "@" << refresh << "hz for output" << connectorName;
    }

    QEglFSKmsOutput output = {
        connectorName,
        connector->connector_id,
        crtc_id,
        QSizeF(connector->mmWidth, connector->mmHeight),
        selected_mode,
        false,
        drmModeGetCrtc(m_dri_fd, crtc_id),
        modes
    };

    m_crtc_allocator |= (1 << output.crtc_id);
    m_connector_allocator |= (1 << output.connector_id);

    return new QEglFSKmsScreen(this, output, pos);
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
    , m_crtc_allocator(0)
    , m_connector_allocator(0)
{
}

bool QEglFSKmsDevice::open()
{
    Q_ASSERT(m_dri_fd == -1);
    Q_ASSERT(m_gbm_device == Q_NULLPTR);

    qCDebug(qLcEglfsKmsDebug) << "Opening device" << m_path;
    m_dri_fd = qt_safe_open(m_path.toLocal8Bit().constData(), O_RDWR | O_CLOEXEC);
    if (m_dri_fd == -1) {
        qErrnoWarning("Could not open DRM device %s", qPrintable(m_path));
        return false;
    }

    qCDebug(qLcEglfsKmsDebug) << "Creating GBM device for file descriptor" << m_dri_fd
                              << "obtained from" << m_path;
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
    drmModeResPtr resources = drmModeGetResources(m_dri_fd);
    if (!resources) {
        qWarning("drmModeGetResources failed");
        return;
    }

    QPoint pos(0, 0);
    QEglFSIntegration *integration = static_cast<QEglFSIntegration *>(QGuiApplicationPrivate::platformIntegration());

    for (int i = 0; i < resources->count_connectors; i++) {
        drmModeConnectorPtr connector = drmModeGetConnector(m_dri_fd, resources->connectors[i]);
        if (!connector)
            continue;

        QEglFSKmsScreen *screen = screenForConnector(resources, connector, pos);
        if (screen) {
            integration->addScreen(screen);
            pos.rx() += screen->geometry().width();
        }

        drmModeFreeConnector(connector);
    }

    drmModeFreeResources(resources);
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
