// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosappcontext.h"
#include "qohoswantutils_p.h"
#include <QtCore/private/qohoscommon_p.h>
#include "qohosqpafunctionspart1_p.h"
#include <QtOhosAppKit/private/qohosappbundleinfo_p.h>
#include <QtOhosAppKit/private/qohoswantutils_p.h>
#include <QtOhosAppKit/qohosabilitycontext.h>
#include <functional>
#include <memory>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

namespace {

template<typename T>
using QOhosSupplier = std::function<T()>;

class QOhosAppContextImpl : public QOhosAppContext
{
public:
    QOhosAppContextImpl();

    bool hasSerialPortAccessRight(const QString &portName) const override;
    void requestSerialPortAccessRightIfNeeded(const QString &portName) override;
    QSharedPointer<QOhosBundleInfo> getBundleInfo() const override;
    Q_NORETURN void restartApp() override;
    Q_NORETURN void restartApp(const QOhosWant &want) override;

private:
};

template<typename T>
QSharedPointer<QObject> makeQObjectLifetimeHandleOrNull(std::shared_ptr<T> handle)
{
    if (!handle)
        return {};

    return QSharedPointer<QObject>(
        new QObject(),
        [handle](QObject *ptr) {
            ptr->deleteLater();
        });
}

}

/*!
    \class QtOhosAppKit::QOhosAppContext
    \inmodule QtOhosAppKit
    \since 5.12.12
    \brief The QOhosAppContext class contains API to manage native application context.
*/

/*!
    \fn void QtOhosAppKit::QOhosAppContext::serialPortAccessRightResponseReceived(const QString &portName, QSharedPointer<QObject> serialPortAccessRightContext)

    Emitted when the result of requestSerialPortAccessRightIfNeeded(const QString &portName) becomes available.

    The signal provides the \a portName corresponding to the response.
    Additionally, it includes a \a serialPortAccessRightContext that defines the access rights granted for the requested serial port.
    The caller must keep this context alive for as long as access to the serial port is required.

    Destroying or releasing the context automatically cancels the access rights for the associated serial port.

    If access was not granted, the context will be nullptr.

    \note The lifetime of the access permission is strictly tied to the lifetime of the provided context.
    Once the last reference to the context is destroyed, the permission request is revoked internally.

    \sa requestSerialPortAccessRightIfNeeded(const QString &portName)

*/

QOhosAppContext::QOhosAppContext() = default;

QOhosAppContext::~QOhosAppContext() = default;

/*!
    \fn static QOhosAppContext *QtOhosAppKit::QOhosAppContext::instance()

    Gets QOhosAppContext global instance.
*/
QOhosAppContext *QOhosAppContext::instance()
{
    static QOhosAppContextImpl instanceObj;
    return &instanceObj;
}

/*!
    \fn static void QtOhosAppKit::QOhosAppContext::startNoUiChildProcess(QString libraryName, QStringList args)

    Starts "No UI" child process for a given \a libraryName and \a args. Arguments passed to the
    startNoUiChildProcess() function are forwarded to the child's main() function.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-app-ability-childprocessmanager-V5}
    {Child Process Manager}.

    \code
    QtOhosAppKit::QOhosAppContext::startNoUiChildProcess(
        "libapp.so",
        QStringList{
            "first arg",
            "second arg",
        });
    \endcode
*/
void QOhosAppContext::startNoUiChildProcess(QString libraryName, QStringList args)
{
    QtOhos::getQOhosQpaFunctions().startNoUiChildProcess(libraryName, args);
}

/*!
  Returns the Want object that was used to launch initial instance of the application's QAbility.
  It is recommended to use newer getAppLaunchWantInfo() method.

  \sa getAppLaunchWantInfo()
*/
QOhosWant QOhosAppContext::getAppLaunchWant()
{
    auto jsonAppLaunchWant = QtOhos::getQOhosQpaFunctions().getAppLaunchWant();
    return convertWantFromJsonObject(jsonAppLaunchWant);
}

/*!
    \fn static QSharedPointer<QtOhosAppKit::QOhosWantInfo> QtOhosAppKit::QOhosAppContext::getAppLaunchWantInfo()

    Returns the Want object that was used to launch initial instance of the application's QAbility.
*/
QSharedPointer<QOhosWantInfo> QOhosAppContext::getAppLaunchWantInfo()
{
    return convertToOhosAppKitWantInfo(QtOhos::getQOhosQpaFunctions().getAppLaunchWantInfo());
}

/*!
    \fn static void QtOhosAppKit::QOhosAppContext::restartApp()

    Restarts the Application using the app launch want.

    The current application will be killed using SIGKILL and a new instance of the application will
    be launched with the application start want.

    All abilities and sub-widnows created within this process will be closed.

    The application will be killed ungracefully. This function won't return to the caller.

    The caller must ensure, that the application has system focus when this function is called,
    otherwise the application will be killed but the new application won't be started. OHOS system
    treats some system dialogs (for example File Dialog) as separate from the application. If such
    dialog is open, the application loses the system focus.

    If restartApp is called too frequently, the system call will be throttled to avoid errors.

    Use this function, if you want to restart app with app launch want, instead of calling
    \sa QtOhosAppKit::QOhosAppContext::restartApp(const QtOhosAppKit::QOhosWant &requestWant) with
    \sa QtOhosAppKit::QOhosAppContext::getAppLaunchWant() as a parameter.
*/
Q_NORETURN void QOhosAppContextImpl::restartApp()
{
    QtOhos::getQOhosQpaFunctions().restartApp({});
}

/*!
    \fn static void QtOhosAppKit::QOhosAppContext::restartApp(const QtOhosAppKit::QOhosWant &requestWant)

    Restarts the Application using the \a requestWant.

    The current application will be killed using SIGKILL and a new instance of the application will
    be launched with the \a requestWant.

    All abilities and sub-widnows created within this process will be closed.

    The application will be killed ungracefully. This function won't return to the caller.

    The caller must ensure, that the application has system focus when this function is called,
    otherwise the application will be killed but the new application won't be started. OHOS system
    treats some system dialogs (for example File Dialog) as separate from the application. If such
    dialog is open, the application loses the system focus.

    If restartApp is called too frequently, the system call will be throttled to avoid errors.

    To call this function with the app launch want
    (\sa QtOhosAppKit::QOhosAppContext::getAppLaunchWant()), use
    \sa QtOhosAppKit::QOhosAppContext::restartApp()

    \code
    QtOhosAppKit::QOhosWant requestWant = QtOhosAppKit::QOhosAppContext::getAppLaunchWant();
    requestWant.parameters["first_parameter"] = "first_parameter_value";
    requestWant.parameters["second_parameter"] = "second_parameter_value";
    QtOhosAppKit::QOhosAppContext::restartApp(requestWant);
    \endcode
*/
Q_NORETURN void QOhosAppContextImpl::restartApp(const QOhosWant &requestWant)
{
    QtOhos::getQOhosQpaFunctions().restartApp(
        std::make_optional(convertWantToJsonObject(requestWant)));
}

QOhosAppContextImpl::QOhosAppContextImpl()
{
    qRegisterMetaType<QSharedPointer<QObject>>();
}

/*!
    \fn bool QtOhosAppKit::QOhosAppContext::hasSerialPortAccessRight(const QString &portName) const

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
    return QtOhos::getQOhosQpaFunctions().hasSerialPortAccessRight(portName);
}

/*!
    \fn void QtOhosAppKit::QOhosAppContext::requestSerialPortAccessRightIfNeeded(const QString &portName)

    Requests permission for the application to access the serial port identified by \a portName.

    This function performs an asynchronous permission request. The result of the request is delivered via the
    serialPortAccessRightResponseReceived() signal.

    The outcome of the request (granted or denied) is reported asynchronously through serialPortAccessRightResponseReceived().
    If access is granted, the signal provides a context object that must be kept alive for as long as the application
    requires access to the serial port.

    If access is not granted, serialPortAccessRightResponseReceived() signal delivers nullptr. This may happen if:
    \list
        \li a provided \a portName cannot be mapped to a valid system serial port,
        \li an error occurred while requesting the access right,
        \li user denied the access right.
    \endlist

    For details about the underlying platform API, see
    \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-serialmanager}
    {Serial Port Manager}.

    \sa serialPortAccessRightResponseReceived()
*/
void QOhosAppContextImpl::requestSerialPortAccessRightIfNeeded(const QString &portName)
{
    QtOhos::getQOhosQpaFunctions().requestSerialPortAccessRight(
        portName, this,
        [this, portName](std::shared_ptr<void> serialPortAccessRightContext) {
            Q_EMIT serialPortAccessRightResponseReceived(
                portName, makeQObjectLifetimeHandleOrNull(serialPortAccessRightContext));
        });
}

/*!
    \fn QSharedPointer<QOhosBundleInfo> QtOhosAppKit::QOhosAppContext::getBundleInfo() const

    Returns QOhosBundleInfo object for the current application. The obtained information does not
    contain information about the signature, HAP module, ability, ExtensionAbility, or permission.
*/
QSharedPointer<QOhosBundleInfo> QOhosAppContextImpl::getBundleInfo() const
{
    return createBundleInfo(QtOhos::getQOhosQpaFunctions().getCurrentApplicationVersionCode());
}

}

QT_END_NAMESPACE
