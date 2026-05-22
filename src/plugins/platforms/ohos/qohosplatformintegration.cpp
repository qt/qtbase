// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "private/qohosplatformtheme_p.h"
#include "qohosapplicationstatetracker.h"
#include "qohoseglplatformcontext.h"
#include "qohoseventdispatcher.h"
#include "qohosfloatingwindow.h"
#include "qohosforeignwindow.h"
#include "qohosjsmain.h"
#include "qohosplatformbackingstore.h"
#include "qohosplatformbackingstoregl.h"
#include "qohosplatformdrag.h"
#include "qohosplatformintegration.h"
#include "qohosplatformnativeinterface.h"
#include "qohosplatformscreen.h"
#include "qohosplatformservices.h"
#include "qohosplatformwindow.h"
#include "qohosinputcontext.h"
#include "qohosinputmethodeventhandler.h"
#include "qohosplatformtheme.h"
#include "qohossystemlocale.h"
#include "qohosutils.h"

#include <QtCore/qdebug.h>
#include <QtCore/qthread.h>
#include <QtCore/private/qnapi_p.h>
#include <QtGui/private/qeglpbuffer_p.h>
#include <QtGui/private/qrhibackingstore_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtCore/qcoreapplication.h>
#include <qohosdeviceinfo_p.h>
#include <qohosplugincore.h>

#include "qohosplatformfontdatabase_p.h"

#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatforminputcontextfactory_p.h>
#include <QtCore/qset.h>
#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#if QT_CONFIG(vulkan)
#include "qohosplatformvulkaninstance.h"
#endif

QT_BEGIN_NAMESPACE

namespace {

bool isInputDeviceOfType(QtOhos::JsState &jsState, std::uint32_t deviceId, const std::string &type)
{
    auto devSources = QNapi::getArrayElements<std::vector<std::string>, QNapi::String>(
        jsState.eval<QNapi::Array>(
            "@ohos.multimodalInput.inputDevice.getDeviceInfoSync(*).sources",
            {deviceId}));
    return std::find(devSources.begin(), devSources.end(), type) != devSources.end();
}

bool isInputDeviceWithTouchscreen(QtOhos::JsState &jsState, std::uint32_t deviceId)
{
    return isInputDeviceOfType(jsState, deviceId, "touchscreen");
}

bool isInputDeviceWithTouchpad(QtOhos::JsState &jsState, std::uint32_t deviceId)
{
    return isInputDeviceOfType(jsState, deviceId, "touchpad");
}

bool isDeviceTypeInDeviceIds(
    QtOhos::JsState &jsState, const std::vector<std::uint32_t> &deviceIds,
    const std::function<bool(QtOhos::JsState &, std::uint32_t)> &isDeviceTypeFunc)
{
    return std::any_of(
        deviceIds.begin(), deviceIds.end(),
        [&](std::uint32_t deviceId) {
            return isDeviceTypeFunc(jsState, deviceId);
        });
}

std::set<QInputDevice::DeviceType> getAvailableDeviceTypes()
{
    return QtOhos::evalInJsThreadWithConsumer<std::set<QInputDevice::DeviceType>>(
        [](QtOhos::JsState &jsState, QOhosConsumer<std::set<QInputDevice::DeviceType>> resultConsumer) {
            jsState.eval<QNapi::Promise>("@ohos.multimodalInput.inputDevice.getDeviceList()")
                .withContext(std::move(resultConsumer))
                .onThenWithContext(
                    [](const QtOhos::CallbackInfo &cbInfo, auto &resultConsumer) {
                        auto deviceIdsJsArray = cbInfo.getFirstArg<QNapi::Array>(Q_FUNC_INFO);
                        auto deviceIds = QNapi::getArrayElements<std::vector<std::uint32_t>, QNapi::Number>(deviceIdsJsArray);
                        std::set<QInputDevice::DeviceType> deviceTypes;
                        if (isDeviceTypeInDeviceIds(cbInfo.jsState(), deviceIds, isInputDeviceWithTouchscreen))
                            deviceTypes.insert(QInputDevice::DeviceType::TouchScreen);
                        if (isDeviceTypeInDeviceIds(cbInfo.jsState(), deviceIds, isInputDeviceWithTouchpad))
                            deviceTypes.insert(QInputDevice::DeviceType::TouchPad);
                        resultConsumer(deviceTypes);
                    })
                .onCatchWithContext(
                    [](const QtOhos::CallbackInfo &, auto &resultConsumer) {
                        qOhosPrintfError("Error while obtaining device list (@ohos.multimodalInput.inputDevice.getDeviceList())");
                        resultConsumer({});
                    });
        });
}

}

QScopedPointer<QOhosSystemLocale> QOhosPlatformIntegration::m_systemLocale;
QOhosPlatformIntegration::WindowGeometryPersistencePolicy QOhosPlatformIntegration::m_mainWindowPersistencePolicy =
    QOhosPlatformIntegration::WindowGeometryPersistencePolicy::Disabled;

QOhosPlatformIntegration *QOhosPlatformIntegration::instance()
{
    return static_cast<QOhosPlatformIntegration *>(QGuiApplicationPrivate::platformIntegration());
}

QOhosPlatformIntegration::QOhosPlatformIntegration(const QStringList &paramList)
{
    Q_UNUSED(paramList);
    auto __dbg = make_QCScopedDebug("QOhosPlatformIntegration::QOhosPlatformIntegration");
    m_ohosPlatformNativeInterface.reset(new QOhosPlatformNativeInterface());

    if (!QtOhos::isOhosNoUiChildMode()) {
        m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (Q_UNLIKELY(m_eglDisplay == EGL_NO_DISPLAY))
            qOhosReportFatalErrorAndAbort("Could not open egl display");

        EGLint major;
        EGLint minor;
        if (Q_UNLIKELY(!eglInitialize(m_eglDisplay, &major, &minor)))
            qOhosReportFatalErrorAndAbort("Could not initialize egl display");

        if (Q_UNLIKELY(!eglBindAPI(EGL_OPENGL_ES_API)))
            qOhosReportFatalErrorAndAbort("Could not bind GL_ES API");
    }

    if (!QtOhos::isOhosNoUiChildMode())
        m_screenManager = std::make_unique<QOhosScreenManager>();

    m_mainThread = QThread::currentThread();

    m_ohosFDB = std::make_unique<QOhosPlatformFontDatabase>();
    m_ohosPlatformServices = std::make_unique<QOhosPlatformServices>();
    if (!QtOhos::isOhosNoUiChildMode())
        m_ohosInputMethodEventHandler = std::make_unique<QOhosInputMethodEventHandler>(getAvailableDeviceTypes());

#ifndef QT_NO_CLIPBOARD
    if (!QtOhos::isOhosNoUiChildMode())
        m_platformClipboard = std::make_unique<QOhosPlatformClipboard>();
#endif // QT_NO_CLIPBOARD

#if QT_CONFIG(draganddrop)
    if (!QtOhos::isOhosNoUiChildMode())
        m_drag = makeQOhosPlatformDrag();
#endif // QT_CONFIG(draganddrop)

    // QCoreApplication::postEvent takes ownership of the created event.
    QCoreApplication::postEvent(m_ohosPlatformNativeInterface.data(), new QEvent(QEvent::User));
}

QOhosInputMethodEventHandler *QOhosPlatformIntegration::inputMethodEventHandler() const
{
    return m_ohosInputMethodEventHandler.get();
}

QAbstractEventDispatcher *QOhosPlatformIntegration::createEventDispatcher() const
{
    return new QOhosEventDispatcher;
}

QVariant QOhosPlatformIntegration::styleHint(StyleHint hint) const
{
    if (hint == ShowIsMaximized)
        return false;
    return QPlatformIntegration::styleHint(hint);
}

Qt::WindowState QOhosPlatformIntegration::defaultWindowState(Qt::WindowFlags flags) const
{
    // Don't maximize dialogs on Android
    if ((flags & Qt::Dialog & ~Qt::Window) != 0)
        return Qt::WindowNoState;

    return QPlatformIntegration::defaultWindowState(flags);
}

QPlatformWindow *QOhosPlatformIntegration::createPlatformWindow(QWindow *window) const
{
    auto __dbg = make_QCScopedDebug("QOhosPlatformIntegration::createPlatformWindow");

    static const QSet<QString> nativeDialogClass = {
        QString::fromUtf8("QFileDialogClassWindow"),
    };
    // FIXME: - System that decides the window class should be reworked
    // For now this behaviour avoids potential crashes related to the lack of surface
    if (window != nullptr && !nativeDialogClass.contains(window->objectName()))
        return new QOhosFloatingWindow(window);
    return new QOhosPlatformWindow(window);
}

QPlatformWindow *QOhosPlatformIntegration::createForeignWindow(QWindow *window, WId windowId) const
{
    return new QOhosForeignWindow(window, windowId);
}

QPlatformFontDatabase *QOhosPlatformIntegration::fontDatabase() const
{
    return m_ohosFDB.get();
}

#ifndef QT_NO_CLIPBOARD
QOhosPlatformClipboard *QOhosPlatformIntegration::clipboard() const
{
    return m_platformClipboard.get();
}
#endif

#if QT_CONFIG(draganddrop)
QPlatformDrag *QOhosPlatformIntegration::drag() const
{
    return m_drag.get();
}
#endif // QT_CONFIG(draganddrop)

QPlatformBackingStore *QOhosPlatformIntegration::createPlatformBackingStore(QWindow *window) const
{
    std::unique_ptr<QPlatformBackingStore> result;
    switch (window->surfaceType()) {
    case QSurface::RasterSurface:
        // NOTE - This is temporary change done so that tests can be performed
        // on the new implementation - if there are no problems
        // a switch to it as the default will be made
        result = QtOhos::isGlBackingStoreDefaultEnabled()
            ? makeGlOhosPlatformBackingStore(window)
            : std::make_unique<QOhosPlatformBackingStore>(
                window,
                QOhosPlatformBackingStore::CreateInfo{
                    .debugDrawFlushedRegion = QtOhos::isDebugDrawQtRasterBackingStoreFlushedRegionEnabled(),
                    .enableVsync = QtOhos::isVsyncOnSoftwareBackingStoreEnabled(),
                });
        break;
    case QSurface::OpenGLSurface:
    case QSurface::VulkanSurface:
        result = std::make_unique<QRhiBackingStore>(window);
        break;
    case QSurface::OpenVGSurface:
    case QSurface::MetalSurface:
    case QSurface::Direct3DSurface:
        qOhosReportFatalErrorAndAbort("Unsupported window surface type for backing store: %d", window->surfaceType());
        break;
    }

    return result.release();
}
#ifndef QT_NO_OPENGL
QPlatformOpenGLContext *QOhosPlatformIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    QSurfaceFormat format = context->format();
    format.setRedBufferSize(8);
    format.setGreenBufferSize(8);
    format.setBlueBufferSize(8);
    format.setAlphaBufferSize(8);
    auto *eglCtx = new QOhosEGLPlatformContext(format, context->shareHandle(), m_eglDisplay);
    return eglCtx;
}

QOpenGLContext *QOhosPlatformIntegration::createOpenGLContext(EGLContext context, EGLDisplay display, QOpenGLContext *shareContext) const
{
    return QEGLPlatformContext::createFrom<QOhosEGLPlatformContext>(context, display, m_eglDisplay, shareContext);
}
#endif // QT_NO_OPENGL

QPlatformOffscreenSurface *
QOhosPlatformIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    auto __dbg = make_QCScopedDebug("QOhosPlatformIntegration::createPlatformOffscreenSurface");
    return new QEGLPbuffer(m_eglDisplay, surface->requestedFormat(), surface);
}

bool QOhosPlatformIntegration::hasCapability(Capability cap) const
{
    qOhosDebug(QtForOhos) << "QOhosPlatformIntegration::hasCapability:" << cap;

    switch (cap) {
        case ApplicationState: return true;
        case ThreadedPixmaps: return true;
        case ForeignWindows: return !QtOhos::isOhosNoUiChildMode();
        case NativeWidgets: return !QtOhos::isOhosNoUiChildMode();
        case OpenGL: return !QtOhos::isOhosNoUiChildMode();
        case ThreadedOpenGL: return !QtOhos::isOhosNoUiChildMode();
        case OffscreenSurface: return true;
        // TODO: Enable the capability of OpenGLRasterSurface to have emulator
        // working for OHOS. It doesn't work with other rendering options
        case OpenGLOnRasterSurface: {
            return (QOhosDeviceInfo::getProperty(QOhosDeviceInfo::Type::productModel).toString() == QString::fromUtf8("emulator"));
        }
        case MultipleWindows: return true;
        case WindowManagement: return true;
        case TopStackedNativeChildWindows: return false;
        default:
            return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformInputContext *QOhosPlatformIntegration::inputContext() const
{
    return m_platformInputContext.data();
}

void QOhosPlatformIntegration::initialize()
{
    auto d = make_QCScopedDebug("QOhosPlatformIntegration::initialize");
    const auto requestedInputContext = QPlatformInputContextFactory::requested();
    if (requestedInputContext.isEmpty()) {
        QOhosInputContext *context = new QOhosInputContext;
        m_platformInputContext.reset(context);
    } else {
        m_platformInputContext.reset(QPlatformInputContextFactory::create(requestedInputContext));
    }

    if (QWindowSystemInterfacePrivate::eventHandler != nullptr)
        qOhosReportFatalErrorAndAbort("QWindowSystemInterfacePrivate::eventHandler was already registered.");

    auto tracker = makeApplicationStateTracker();
    QWindowSystemInterfacePrivate::installWindowSystemEventHandler(tracker.get());
    m_applicationStateTracker = QtOhos::makeDestroyNotifier(
        [tracker = std::move(tracker)]() {
            QWindowSystemInterfacePrivate::removeWindowSystemEventhandler(tracker.get());
        });
}

QPlatformTheme *QOhosPlatformIntegration::createPlatformTheme(const QString &name) const
{
    if (QtOhos::isDebugUseBasicStyleAndThemeEnabled())
        return std::make_unique<QPlatformTheme>().release();

    if (name == QString::fromUtf8(ohosThemeName))
        return new QOhosPlatformTheme();
    return nullptr;
}

QStringList QOhosPlatformIntegration::themeNames() const
{
    return {QString::fromUtf8(ohosThemeName)};
}

QOhosSystemLocale *QOhosPlatformIntegration::systemLocale()
{
    return m_systemLocale.data();
}

void QOhosPlatformIntegration::setSystemLocale(QOhosSystemLocale *systemLocale)
{
    m_systemLocale.reset(systemLocale);
}

void QOhosPlatformIntegration::setMainWindowGeometryPersistencePolicy(
    QOhosPlatformIntegration::WindowGeometryPersistencePolicy policy)
{
    m_mainWindowPersistencePolicy = policy;
}

QOhosPlatformIntegration::WindowGeometryPersistencePolicy QOhosPlatformIntegration::getMainWindowGeometryPersistencePolicy()
{
    return m_mainWindowPersistencePolicy;
}

QPlatformServices *QOhosPlatformIntegration::services() const
{
    return m_ohosPlatformServices.get();
}

QPlatformNativeInterface *QOhosPlatformIntegration::nativeInterface() const
{
    return m_ohosPlatformNativeInterface.get();
}

QOhosScreenManager *QOhosPlatformIntegration::screenManager() const
{
    return m_screenManager.get();
}

#if QT_CONFIG(vulkan)

QPlatformVulkanInstance *QOhosPlatformIntegration::createPlatformVulkanInstance(
    QVulkanInstance *instance) const
{
    return new QOhosPlatformVulkanInstance(instance);
}

#endif // QT_CONFIG(vulkan)

QT_END_NAMESPACE
