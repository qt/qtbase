// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qvkkhrdisplayintegration.h"
#include "qvkkhrdisplayvulkaninstance.h"

#include <qpa/qplatformwindow.h>
#include <qpa/qplatformbackingstore.h>
#include <qpa/qplatforminputcontextfactory_p.h>
#include <qpa/qwindowsysteminterface.h>

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qwindow_p.h>
#include <QtGui/private/qgenericunixeventdispatcher_p.h>
#include <QtGui/private/qgenericunixfontdatabase_p.h>
#include <QtGui/private/qgenericunixthemes_p.h>
#include <QtGui/private/qgenericunixservices_p.h>

#include <QtFbSupport/private/qfbvthandler_p.h>

#if QT_CONFIG(libinput)
#include <QtInputSupport/private/qlibinputhandler_p.h>
#endif

#if QT_CONFIG(evdev)
#include <QtInputSupport/private/qevdevmousemanager_p.h>
#include <QtInputSupport/private/qevdevkeyboardmanager_p.h>
#include <QtInputSupport/private/qevdevtouchmanager_p.h>
#endif

#if QT_CONFIG(tslib)
#include <QtInputSupport/private/qtslib_p.h>
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QVkKhrDisplayScreen : public QPlatformScreen
{
public:
    QRect geometry() const override { return m_geometry; }
    int depth() const override { return m_depth; }
    QImage::Format format() const override { return m_format; }
    void setVk(QVkKhrDisplayVulkanInstance *inst);

private:
    QVkKhrDisplayVulkanInstance *m_vk = nullptr;
    QRect m_geometry;
    int m_depth = 32;
    QImage::Format m_format = QImage::Format_ARGB32_Premultiplied;
    friend class QVkKhrDisplayIntegration;
};

void QVkKhrDisplayScreen::setVk(QVkKhrDisplayVulkanInstance *inst)
{
    m_vk = inst;
    m_geometry = QRect(QPoint(0, 0), m_vk->displaySize());
    QWindowSystemInterface::handleScreenGeometryChange(screen(), m_geometry, m_geometry);
    qDebug() << "Screen will report geometry" << m_geometry;

    // Thanks to this deferred screen setup, a QWindow with a size based on the
    // dummy screen size may already exist. Try to resize it.
    QScreen *thisScreen = screen();
    for (QWindow *window : QGuiApplication::allWindows()) {
        if (window->isTopLevel() && window->screen() == thisScreen)
            window->handle()->setGeometry(QRect()); // set fullscreen geometry
    }
}

class QVkKhrDisplayWindow : public QPlatformWindow
{
public:
    QVkKhrDisplayWindow(QWindow *window) : QPlatformWindow(window) { }
    ~QVkKhrDisplayWindow();

    void *vulkanSurfacePtr();

    void setGeometry(const QRect &r) override;

private:
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
};

QVkKhrDisplayWindow::~QVkKhrDisplayWindow()
{
    if (m_surface) {
        QVulkanInstance *inst = window()->vulkanInstance();
        if (inst)
            static_cast<QVkKhrDisplayVulkanInstance *>(inst->handle())->destroySurface(m_surface);
    }
}

void *QVkKhrDisplayWindow::vulkanSurfacePtr()
{
    if (m_surface)
        return &m_surface;

    QVulkanInstance *inst = window()->vulkanInstance();
    if (!inst) {
        qWarning("Attempted to create Vulkan surface without an instance; was QWindow::setVulkanInstance() called?");
        return nullptr;
    }
    QVkKhrDisplayVulkanInstance *vkdinst = static_cast<QVkKhrDisplayVulkanInstance *>(inst->handle());
    m_surface = vkdinst->createSurface(window());

    return &m_surface;
}

void QVkKhrDisplayWindow::setGeometry(const QRect &)
{
    // We only support full-screen windows
    QRect rect(screen()->availableGeometry());
    QWindowSystemInterface::handleGeometryChange(window(), rect);
    QPlatformWindow::setGeometry(rect);

    const QRect lastReportedGeometry = qt_window_private(window())->geometry;
    if (rect != lastReportedGeometry)
        QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(0, 0), rect.size()));
}

// does not actually support raster content, just paint into a QImage and that's it for now
class QVkKhrDisplayBackingStore : public QPlatformBackingStore
{
public:
    QVkKhrDisplayBackingStore(QWindow *window) : QPlatformBackingStore(window) { }

    QPaintDevice *paintDevice() override { return &m_image; }
    void flush(QWindow *window, const QRegion &region, const QPoint &offset) override {
        Q_UNUSED(window);
        Q_UNUSED(region);
        Q_UNUSED(offset);
    }
    void resize(const QSize &size, const QRegion &staticContents) override {
        Q_UNUSED(staticContents);
        QImage::Format format = QGuiApplication::primaryScreen()->handle()->format();
        if (m_image.size() != size)
            m_image = QImage(size, format);
    }

private:
    QImage m_image;
};

QVkKhrDisplayIntegration::QVkKhrDisplayIntegration(const QStringList &parameters)
{
    Q_UNUSED(parameters);
}

QVkKhrDisplayIntegration::~QVkKhrDisplayIntegration()
{
    QWindowSystemInterface::handleScreenRemoved(m_primaryScreen);
    delete m_services;
    delete m_fontDatabase;
    delete m_vtHandler;
}

bool QVkKhrDisplayIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    case WindowManagement: return false;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

void QVkKhrDisplayIntegration::initialize()
{
    m_primaryScreen = new QVkKhrDisplayScreen;

    // The real values are only known when the QVulkanInstance initializes, use
    // dummy values until then.
    m_primaryScreen->m_geometry = QRect(0, 0, 1920, 1080);
    m_primaryScreen->m_depth = 32;
    m_primaryScreen->m_format = QImage::Format_ARGB32_Premultiplied;

    QWindowSystemInterface::handleScreenAdded(m_primaryScreen);

    m_inputContext = QPlatformInputContextFactory::create();

    m_vtHandler = new QFbVtHandler;

    if (!qEnvironmentVariableIntValue("QT_QPA_DISABLE_INPUT"))
        createInputHandlers();
}

QPlatformFontDatabase *QVkKhrDisplayIntegration::fontDatabase() const
{
    if (!m_fontDatabase)
        m_fontDatabase = new QGenericUnixFontDatabase;

    return m_fontDatabase;
}

QPlatformServices *QVkKhrDisplayIntegration::services() const
{
    if (!m_services)
        m_services = new QGenericUnixServices;

    return m_services;
}

QPlatformInputContext *QVkKhrDisplayIntegration::inputContext() const
{
    return m_inputContext;
}

QPlatformTheme *QVkKhrDisplayIntegration::createPlatformTheme(const QString &name) const
{
    return QGenericUnixTheme::createUnixTheme(name);
}

QPlatformNativeInterface *QVkKhrDisplayIntegration::nativeInterface() const
{
    return const_cast<QVkKhrDisplayIntegration *>(this);
}

QPlatformWindow *QVkKhrDisplayIntegration::createPlatformWindow(QWindow *window) const
{
    if (window->surfaceType() != QSurface::VulkanSurface) {
        qWarning("vkkhrdisplay platform plugin only supports QWindow with surfaceType == VulkanSurface");
        // Assume VulkanSurface, better than crashing. Consider e.g. an autotest
        // creating a default QWindow just to have something to be used with
        // QRhi's Null backend.  Continuing to set up a Vulkan window (even
        // though the request was Raster or something) is better than failing to
        // create a platform window, and may even be sufficient in some cases.
    }

    QVkKhrDisplayWindow *w = new QVkKhrDisplayWindow(window);
    w->setGeometry(QRect()); // set fullscreen geometry
    w->requestActivateWindow();
    return w;
}

QPlatformBackingStore *QVkKhrDisplayIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QVkKhrDisplayBackingStore(window);
}

QAbstractEventDispatcher *QVkKhrDisplayIntegration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}

void QVkKhrDisplayIntegration::handleInstanceCreated(QVkKhrDisplayVulkanInstance *inst, void *userData)
{
    QVkKhrDisplayIntegration *self = static_cast<QVkKhrDisplayIntegration *>(userData);
    self->m_primaryScreen->setVk(inst);
}

QPlatformVulkanInstance *QVkKhrDisplayIntegration::createPlatformVulkanInstance(QVulkanInstance *instance) const
{
    QVkKhrDisplayVulkanInstance *inst = new QVkKhrDisplayVulkanInstance(instance);
    inst->setCreatedCallback(handleInstanceCreated, const_cast<QVkKhrDisplayIntegration *>(this));
    return inst;
}

enum ResourceType {
    VkSurface
};

static int resourceType(const QByteArray &key)
{
    static const QByteArray names[] = { // match ResourceType
        QByteArrayLiteral("vksurface")
    };
    const QByteArray *end = names + sizeof(names) / sizeof(names[0]);
    const QByteArray *result = std::find(names, end, key);
    if (result == end)
        result = std::find(names, end, key.toLower());
    return int(result - names);
}

void *QVkKhrDisplayIntegration::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    void *result = nullptr;

    switch (resourceType(resource)) {
    case VkSurface:
        if (window && window->handle() && window->surfaceType() == QSurface::VulkanSurface)
            result = static_cast<QVkKhrDisplayWindow *>(window->handle())->vulkanSurfacePtr();
        break;
    default:
        break;
    }

    return result;
}

void QVkKhrDisplayIntegration::createInputHandlers()
{
#if QT_CONFIG(libinput)
    if (!qEnvironmentVariableIntValue("QT_QPA_NO_LIBINPUT")) {
        new QLibInputHandler("libinput"_L1, QString());
        return;
    }
#endif

#if QT_CONFIG(tslib)
    bool useTslib = qEnvironmentVariableIntValue("QT_QPA_TSLIB");
    if (useTslib)
        new QTsLibMouseHandler("TsLib"_L1, QString());
#endif

#if QT_CONFIG(evdev)
    new QEvdevKeyboardManager("EvdevKeyboard"_L1, QString(), this);
    new QEvdevMouseManager("EvdevMouse"_L1, QString(), this);
#if QT_CONFIG(tslib)
    if (!useTslib)
#endif
        new QEvdevTouchManager("EvdevTouch"_L1, QString() /* spec */, this);
#endif
}

QT_END_NAMESPACE
