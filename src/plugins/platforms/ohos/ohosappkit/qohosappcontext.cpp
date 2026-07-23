// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosappcontext.h"
#include "qohosjsenv_p.h"
#include "qohoswantinfo_p.h"
#include "qohoswantutils_p.h"
#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohosjstools_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <QtOhosAppKit/private/qohosappbundleinfo_p.h>
#include <QtOhosAppKit/private/qohoswantutils_p.h>
#include <QtOhosAppKit/qohosabilitycontext.h>
#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <unistd.h>
#include <utility>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

namespace {

template<typename T>
using QOhosSupplier = std::function<T()>;

int getCurrentApplicationVersionCode()
{
    return QOhosJsThreadGateway::eval(
        [](QOhosJsState &jsState) {
            auto applicationInfoFlag = jsState.eval<QNapi::Number>(
                "@ohos.bundle.bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_APPLICATION");
            auto bundleInfo = jsState.eval<QNapi::Object>(
                "@ohos.bundle.bundleManager.getBundleInfoForSelfSync(*)", {applicationInfoFlag});
            int versionCode = bundleInfo.get<QNapi::Number>("versionCode");

            return versionCode;
        },
        Q_FUNC_INFO);
}

Q_NORETURN void killCurrentProcess()
{
    ::kill(getpid(), SIGKILL);
    std::abort();
}

std::optional<std::uint32_t> tryGetCodeFromJsBusinessError(const Napi::Error &error)
{
    if (!error.Value().IsObject())
        return std::nullopt;

    auto errorObject = QNapi::checkedCast<QNapi::Object>(error.Value());
    auto optErrorCode = QNapi::getOptionalPropOrEmpty<QNapi::Number>(errorObject, "code");

    return !optErrorCode.IsEmpty()
        ? std::make_optional(optErrorCode.Uint32Value())
        : std::nullopt;
}

Q_NORETURN void restartAppImpl(std::optional<QJsonObject> want)
{
    QOhosJsThreadGateway::runAndWait(
        [&](QOhosJsState &jsState) {
            auto napiWant = want.has_value()
                ? QNapi::checkedCast<QNapi::Object>(QOhosJsEnv::toNapiValue(jsState.env(), want.value()))
                : jsState.appLaunchWant();

            constexpr auto sleepTimeBeforeRetry = std::chrono::seconds(3);

            unsigned remainingTries = 3;

            while (true) {
                --remainingTries;

                qOhosPrintfInfo(
                    "%s: calling restartApp() using Want: %s",
                    Q_FUNC_INFO, QNapi::toJsonString(napiWant).c_str());

                auto optQAbility = jsState.defaultQAbility();
                if (!optQAbility.has_value())
                    qOhosReportFatalErrorAndAbort("%s: no default UIAbility available to restart the app", Q_FUNC_INFO);

                try {
                    optQAbility.value().eval(
                        "context.getApplicationContext().restartApp(*)", {napiWant});

                    qOhosPrintfWarning("%s: restartApp() call unexpectedly returned, killing self", Q_FUNC_INFO);
                    killCurrentProcess();
                } catch (const Napi::Error &error) {
                    constexpr std::uint32_t restartTooFrequentlyErrorCode = 16000064;

                    auto errorCode = tryGetCodeFromJsBusinessError(error);

                    if (errorCode == restartTooFrequentlyErrorCode && remainingTries != 0) {
                        qOhosPrintfWarning(
                            "%s: restartApp() returned with error %u, sleeping before retry",
                            Q_FUNC_INFO, restartTooFrequentlyErrorCode);

                        std::this_thread::sleep_for(sleepTimeBeforeRetry);
                    } else {
                        auto errorCodeStr = errorCode.has_value()
                            ? std::to_string(errorCode.value())
                            : "?";
                        qOhosPrintfWarning(
                            "%s: restartApp() returned with error %s, killing self",
                            Q_FUNC_INFO, errorCodeStr.c_str());

                        killCurrentProcess();
                    }
                }
            }
        },
        Q_FUNC_INFO);

    qOhosReportFatalErrorAndAbort("%s: unexpected return from the JS thread call", Q_FUNC_INFO);
}

struct SerialPortPermissionsState
{
    std::unordered_map<std::uint32_t, std::vector<QOhosConsumer<std::shared_ptr<void>>>> m_pendingSerialPortsPermissionRequestsConsumers;
    std::unordered_map<std::uint32_t, std::weak_ptr<void>> m_grantedSerialPortsPermissionContexts;
};

std::shared_ptr<SerialPortPermissionsState> serialPortPermissionsState()
{
    static auto state = std::make_shared<SerialPortPermissionsState>();
    return state;
}

std::optional<std::uint32_t> tryConvertPortNameToSystemPortId(const QString &portName)
{
    constexpr const char *serialPortPrefix = "COM";
    const QString prefix = QLatin1String(serialPortPrefix);

    if (!portName.startsWith(prefix))
        return {};

    bool parsedOk = false;
    const uint parsedValue = portName.mid(prefix.length()).toUInt(&parsedOk);
    if (!parsedOk)
        return {};

    return static_cast<std::uint32_t>(parsedValue);
}

bool hasSerialPortAccessRightJsImpl(QOhosJsState &jsState, std::uint32_t serialPortId)
{
    try {
        return jsState.eval<QNapi::Boolean>("@ohos.usbManager.serial.hasSerialRight(*)", {serialPortId});
    } catch (const Napi::Error &error) {
        qOhosPrintfError(
            "%s: hasSerialRight for port %d failed with error: %s",
            Q_FUNC_INFO, serialPortId, error.what());
        return false;
    }
}

void requestSerialPortAccessRightJsImpl(
    QOhosJsState &jsState, std::uint32_t serialPortId, QOhosConsumer<bool> resultConsumer)
{
    jsState.evalToPromiseOrRejectOnThrow(
        "@ohos.usbManager.serial.requestSerialRight(*)", {serialPortId})
    .withContext(std::move(resultConsumer))
    .onThenWithContext(
        [](const QOhosCallbackInfo &cbInfo, auto &resultConsumer) {
            bool granted = cbInfo.getFirstArg<QNapi::Boolean>(Q_FUNC_INFO);
            resultConsumer(granted);
        })
    .onCatchWithContext(
        [](const QOhosCallbackInfo &cbInfo, auto &resultConsumer) {
            QtOhos::logJsCallbackError(
                cbInfo, "@ohos.usbManager.serial.requestSerialRight() failed");
            resultConsumer(false);
        });
}

void cancelSerialPortAccessRightJsImpl(QOhosJsState &jsState, std::uint32_t serialPortId)
{
    if (!hasSerialPortAccessRightJsImpl(jsState, serialPortId))
        return;

    try {
        jsState.eval("@ohos.usbManager.serial.cancelSerialRight(*)", {serialPortId});
    } catch (const Napi::Error &error) {
        qOhosPrintfError(
            "%s: cancelSerialRight(%u) failed with error (ignoring): %s",
            Q_FUNC_INFO, serialPortId, error.what());
    }
}

void processSerialPortPermissionResponse(std::uint32_t serialPortId, bool granted)
{
    auto self = serialPortPermissionsState();

    auto permissionContext = granted
        ? QtOhos::makeDestroyNotifier(
            [serialPortId, weakSelf = QtOhos::makeWeakPtr(serialPortPermissionsState())]() {
                QtOhos::invokeInQtThread(
                    [serialPortId, weakSelf]() {
                        QOhosJsThreadGateway::runAndWait(
                            [&](QOhosJsState &jsState) {
                                cancelSerialPortAccessRightJsImpl(jsState, serialPortId);
                            },
                            Q_FUNC_INFO);

                        auto self = weakSelf.lock();
                        if (self)
                            self->m_grantedSerialPortsPermissionContexts.erase(serialPortId);
                    });
            })
        : nullptr;

    if (permissionContext)
        self->m_grantedSerialPortsPermissionContexts[serialPortId] = permissionContext;

    for (const auto &asyncPermissionRequestConsumer : self->m_pendingSerialPortsPermissionRequestsConsumers[serialPortId])
        asyncPermissionRequestConsumer(permissionContext);

    self->m_pendingSerialPortsPermissionRequestsConsumers.erase(serialPortId);
}

void requestSerialPortAccessRight(
    const QString &portName, QObject *resultConsumerQtContext,
    QOhosConsumer<std::shared_ptr<void>> resultConsumer)
{
    auto resultConsumerQtContextRef = QtOhos::makeQThreadSafeRef(resultConsumerQtContext);
    auto asyncResultConsumer = [resultConsumerQtContextRef, resultConsumer = std::move(resultConsumer)](std::shared_ptr<void> permissionContext) {
        resultConsumerQtContextRef.visitInQtThreadIfAlive(
            [resultConsumer = std::move(resultConsumer), permissionContext](auto &resultConsumerQtContext) {
                QMetaObject::invokeMethod(
                    &resultConsumerQtContext,
                    [resultConsumer = std::move(resultConsumer), permissionContext]() {
                        resultConsumer(permissionContext);
                    },
                    Qt::QueuedConnection);
            });
    };

    const auto optSerialPortId = tryConvertPortNameToSystemPortId(portName);
    if (!optSerialPortId.has_value()) {
        qOhosPrintfError(
            "%s: cannot convert serial port name '%s' to port id.",
            Q_FUNC_INFO, portName.toStdString().c_str());

        asyncResultConsumer(nullptr);
        return;
    }

    QtOhos::invokeInQtThread(
        [serialPortId = optSerialPortId.value(), weakSelf = QtOhos::makeWeakPtr(serialPortPermissionsState()), asyncResultConsumer = std::move(asyncResultConsumer)]() {
            auto self = weakSelf.lock();
            if (!self)
                return;

            auto alreadyGrantedPermissionContextIt =
                self->m_grantedSerialPortsPermissionContexts.find(serialPortId);
            auto optAlreadyGrantedPermissionContext =
                alreadyGrantedPermissionContextIt != self->m_grantedSerialPortsPermissionContexts.end()
                    ? alreadyGrantedPermissionContextIt->second.lock()
                    : nullptr;

            if (optAlreadyGrantedPermissionContext) {
                asyncResultConsumer(optAlreadyGrantedPermissionContext);
                return;
            }

            self->m_pendingSerialPortsPermissionRequestsConsumers[serialPortId].push_back(
                std::move(asyncResultConsumer));

            if (self->m_pendingSerialPortsPermissionRequestsConsumers[serialPortId].size() == 1) {
                QOhosJsThreadGateway::invoke(
                    [serialPortId, weakSelf](QOhosJsState &jsState) {
                        requestSerialPortAccessRightJsImpl(
                            jsState,
                            serialPortId,
                            [serialPortId, weakSelf](bool granted) {
                                QtOhos::invokeInQtThread(
                                    [serialPortId, weakSelf, granted]() {
                                        auto self = weakSelf.lock();
                                        if (self)
                                            processSerialPortPermissionResponse(serialPortId, granted);
                                    });
                        });
                    });
            }
        });
}

class QOhosAppContextImpl : public AppContext
{
public:
    QOhosAppContextImpl();

    bool hasSerialPortAccessRight(const QString &portName) const override;
    void requestSerialPortAccessRightIfNeeded(
        const QString &portName, QObject *context,
        std::function<void(std::shared_ptr<QObject>)> callback) override;
    std::shared_ptr<BundleInfo> bundleInfo() const override;
    Q_NORETURN void restartApp(const std::optional<Want> &want) override;

    double fontSizeScale() const override;

private:
    QOhosSupplier<double> m_fontSizeScaleSupplier;
};

template<typename T>
std::shared_ptr<QObject> makeQObjectLifetimeHandleOrNull(std::shared_ptr<T> handle)
{
    if (!handle)
        return {};

    return std::shared_ptr<QObject>(
        new QObject(),
        [handle](QObject *ptr) {
            ptr->deleteLater();
        });
}

}

/*!
    \class QtOhosAppKit::AppContext
    \inmodule QtOhosAppKit
    \since 5.12.12
    \brief The AppContext class contains API to manage native application context.
*/

AppContext::AppContext() = default;

AppContext::~AppContext() = default;

/*!
    \fn static AppContext *QtOhosAppKit::AppContext::instance()

    Gets AppContext global instance.
*/
AppContext *AppContext::instance()
{
    static QOhosAppContextImpl instanceObj;
    return &instanceObj;
}

/*!
    \fn static bool QtOhosAppKit::AppContext::isNoUiChildMode()

    Returns \c true if the current process was started as a "No UI" child
    process (see startNoUiChildProcess()), otherwise returns \c false.
*/
bool AppContext::isNoUiChildMode()
{
    static const bool noUiChildMode = QOhosJsThreadGateway::eval(
        [](QOhosJsState &jsState) {
            return !jsState.defaultQAbility();
        },
        Q_FUNC_INFO);
    return noUiChildMode;
}

/*!
    \fn static void QtOhosAppKit::AppContext::startNoUiChildProcess(const QString &libraryName, const QStringList &args)

    Starts "No UI" child process for a given \a libraryName and \a args. Arguments passed to the
    startNoUiChildProcess() function are forwarded to the child's main() function.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-app-ability-childprocessmanager-V5}
    {Child Process Manager}.

    \code
    QtOhosAppKit::AppContext::startNoUiChildProcess(
        "libapp.so",
        QStringList{
            "first arg",
            "second arg",
        });
    \endcode
*/
void AppContext::startNoUiChildProcess(const QString &libraryName, const QStringList &args)
{
    QOhosJsThreadGateway::runAndWait(
        [&](QOhosJsState &jsState) {
            std::vector<std::string> argsVector;
            std::transform(
                args.begin(), args.end(), std::back_inserter(argsVector),
                std::mem_fn(&QString::toStdString));
            jsState.startNoUiChildProcess(libraryName.toStdString(), argsVector);
        },
        Q_FUNC_INFO);
}

/*!
    \fn static std::shared_ptr<QtOhosAppKit::WantInfo> QtOhosAppKit::AppContext::appLaunchWantInfo()

    Returns the Want object that was used to launch initial instance of the application's QAbility.
*/
std::shared_ptr<WantInfo> AppContext::appLaunchWantInfo()
{
    return convertToOhosAppKitWantInfo(makeAppLaunchWantInfo());
}

/*!
    \fn static void QtOhosAppKit::AppContext::restartApp(const std::optional<QtOhosAppKit::Want> &want)

    Restarts the Application. If \a want is set, the new instance is launched with it; if \a want is
    empty, the application is restarted with the app launch want.

    The current application will be killed using SIGKILL and a new instance of the application will
    be launched.

    All abilities and sub-widnows created within this process will be closed.

    The application will be killed ungracefully. This function won't return to the caller.

    The caller must ensure, that the application has system focus when this function is called,
    otherwise the application will be killed but the new application won't be started. OHOS system
    treats some system dialogs (for example File Dialog) as separate from the application. If such
    dialog is open, the application loses the system focus.

    If restartApp is called too frequently, the system call will be throttled to avoid errors.

    \code
    QtOhosAppKit::Want requestWant = QtOhosAppKit::AppContext::getAppLaunchWant();
    requestWant.parameters["first_parameter"] = "first_parameter_value";
    requestWant.parameters["second_parameter"] = "second_parameter_value";
    QtOhosAppKit::AppContext::restartApp(requestWant);
    \endcode
*/
Q_NORETURN void QOhosAppContextImpl::restartApp(const std::optional<Want> &want)
{
    restartAppImpl(want ? std::optional(convertWantToJsonObject(*want)) : std::nullopt);
}

QOhosAppContextImpl::QOhosAppContextImpl()
{
    qRegisterMetaType<std::shared_ptr<QObject>>();

    m_fontSizeScaleSupplier = makeQOhosDataSource<double>(
        [](QOhosJsState &jsState) -> double {
            auto optQAbility = jsState.defaultQAbility();
            if (!optQAbility.has_value())
                return 1.0;
            return optQAbility->eval<QNapi::Number>("context.config.fontSizeScale");
        },
        [](QOhosJsState &jsState, QOhosConsumer<double> valueUpdatesConsumer) {
            return registerOhosAppContextEnvironmentCallback(
                jsState,
                {
                    {
                        "onConfigurationUpdated",
                        [valueUpdatesConsumer = std::move(valueUpdatesConsumer)](const QOhosCallbackInfo &cbInfo) {
                            auto config = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);
                            valueUpdatesConsumer(config.get<QNapi::Number>("fontSizeScale"));
                        }
                    },
                });
        },
        [this](double fontSizeScale) {
            Q_EMIT fontSizeScaleChanged(fontSizeScale);
        },
        QtOhos::invokeInQtThread,
        Q_FUNC_INFO);
}

double QOhosAppContextImpl::fontSizeScale() const
{
    return m_fontSizeScaleSupplier();
}

/*!
    \fn bool QtOhosAppKit::AppContext::hasSerialPortAccessRight(const QString &portName) const

    Checks whether the application currently has permission to access the serial port identified by \a portName.

    Returns \c true if access rights for the specified serial port are currently granted, otherwise returns \c false.

    This function performs a synchronous check of the current permission state and does not trigger
    any permission request. To request access rights when they are not yet granted, use
    requestSerialPortAccessRightIfNeeded().

    For details about the underlying platform API, see
    \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-serialmanager}
    {Serial Port Manager}.

    \sa requestSerialPortAccessRightIfNeeded()
*/
bool QOhosAppContextImpl::hasSerialPortAccessRight(const QString &portName) const
{
    const auto optSerialPortId = tryConvertPortNameToSystemPortId(portName);
    if (!optSerialPortId.has_value()) {
        qOhosPrintfError(
            "%s: cannot convert serial port name '%s' to port id.",
            Q_FUNC_INFO, portName.toStdString().c_str());
        return false;
    }

    return QOhosJsThreadGateway::eval(
        [&](QOhosJsState &jsState) {
            return hasSerialPortAccessRightJsImpl(jsState, optSerialPortId.value());
        },
        Q_FUNC_INFO);
}

/*!
    \fn void QtOhosAppKit::AppContext::requestSerialPortAccessRightIfNeeded(const QString &portName, QObject *context, std::function<void(std::shared_ptr<QObject>)> callback)

    Requests permission for the application to access the serial port identified by \a portName.

    This function performs an asynchronous permission request. The result of the request is delivered by
    invoking \a callback on the thread of \a context; if \a context is destroyed before the response
    arrives, \a callback is not invoked.

    The outcome of the request (granted or denied) is reported asynchronously through \a callback.
    If access is granted, \a callback provides a context object that must be kept alive for as long as the application
    requires access to the serial port.

    If access is not granted, \a callback delivers nullptr. This may happen if:
    \list
        \li a provided \a portName cannot be mapped to a valid system serial port,
        \li an error occurred while requesting the access right,
        \li user denied the access right.
    \endlist

    For details about the underlying platform API, see
    \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-serialmanager}
    {Serial Port Manager}.
*/
void QOhosAppContextImpl::requestSerialPortAccessRightIfNeeded(
    const QString &portName, QObject *context,
    std::function<void(std::shared_ptr<QObject>)> callback)
{
    requestSerialPortAccessRight(
        portName, context,
        [callback = std::move(callback)](std::shared_ptr<void> serialPortAccessRightContext) {
            callback(makeQObjectLifetimeHandleOrNull(serialPortAccessRightContext));
        });
}

/*!
    \fn std::shared_ptr<BundleInfo> QtOhosAppKit::AppContext::bundleInfo() const

    Returns BundleInfo object for the current application. The obtained information does not
    contain information about the signature, HAP module, ability, ExtensionAbility, or permission.
*/
std::shared_ptr<BundleInfo> QOhosAppContextImpl::bundleInfo() const
{
    return createBundleInfo(getCurrentApplicationVersionCode());
}

}

QT_END_NAMESPACE
