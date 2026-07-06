// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosabilitycontext.h"
#include "qohoswantutils_p.h"
#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include "qohosqpafunctionspart1_p.h"
#include <QtCore/qeventloop.h>
#include <QtCore/qhash.h>
#include <QtCore/qrandom.h>
#include <QtOhosAppKit/private/qohosoperationstatus_p.h>
#include <QtOhosAppKit/private/qohossharekit_p.h>
#include <QtOhosAppKit/private/qohosstartoptions_p.h>
#include <QtOhosAppKit/private/qohosstartrequest_p.h>
#include <QtOhosAppKit/private/qohoswantutils_p.h>
#include <QtGui/qwindow.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <utility>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

using QOhosQpaFunctions = QtOhos::QOhosQpaFunctionsPart1;

namespace {

constexpr const char *qtOnContinueMigrationDataPropertyName = "__io_qt_on_continue_migration_data";

QOhosSupplier<QByteArray> makeUniqueIdsGenerator()
{
    return [sequenceNumber = std::uint64_t(0)]() mutable {
        QByteArray idBytes(32, Qt::Uninitialized);
        std::memcpy(idBytes.data(), &sequenceNumber, sizeof(sequenceNumber));
        QRandomGenerator::global()->generate(idBytes.begin() + sizeof(sequenceNumber), idBytes.end());
        ++sequenceNumber;
        return idBytes;
    };
}

class QOhosOpenLinkOptionsImpl : public QOhosOpenLinkOptions
{
public:
    QOhosOpenLinkOptionsImpl();

    void setAppLinkingOnly(bool appLinkingOnly) override;

    std::optional<bool> appLinkingOnly() const;

private:
    std::optional<bool> m_appLinkingOnly;
};

QOhosOpenLinkOptionsImpl::QOhosOpenLinkOptionsImpl() = default;

void QOhosOpenLinkOptionsImpl::setAppLinkingOnly(bool appLinkingOnly)
{
    m_appLinkingOnly = appLinkingOnly;
}

std::optional<bool> QOhosOpenLinkOptionsImpl::appLinkingOnly() const
{
    return m_appLinkingOnly;
}

class QOhosOnContinueContextImpl : public QOhosOnContinueContext
{
public:
    QOhosOnContinueContextImpl(int sourceApplicationVersionCode);

    void setAgreeResponse(const QByteArray &responseData) override;
    void setRejectResponse() override;
    void setMismatchResponse() override;

    void setExitAppOnSourceDeviceAfterMigration(bool exitAfterMigration) override;

    int sourceApplicationVersionCode() const override;

    QOhosQpaFunctions::AbilityOnContinueResponse response() const;

private:
    int m_sourceApplicationVersionCode;
    QOhosQpaFunctions::AbilityOnContinueResponse m_baseResponse;
    std::optional<bool> m_exitAppOnSourceDeviceAfterMigration;
};

QOhosOnContinueContextImpl::QOhosOnContinueContextImpl(int sourceApplicationVersionCode)
    : QOhosOnContinueContext()
    , m_sourceApplicationVersionCode(sourceApplicationVersionCode)
{
    setRejectResponse();
}

/*!
    \fn void QtOhosAppKit::QOhosOnContinueContext::setAgreeResponse(const QByteArray &responseData)

    Sets On Continue action as agreed with a given \a responseData.
    Agreed response means that on continuation process is accepted.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V13/js-apis-app-ability-abilityconstant-V13#oncontinueresult}
    {OnContinueResult}
*/
void QOhosOnContinueContextImpl::setAgreeResponse(const QByteArray &responseData)
{
    m_baseResponse = QOhosQpaFunctions::AbilityOnContinueResponse{
        .status = QOhosQpaFunctions::AbilityOnContinueResponseStatus::Agree,
        .wantObjectParams = {
            {
                QString::fromUtf8(qtOnContinueMigrationDataPropertyName),
                QString::fromUtf8(responseData.toBase64())
            },
        },
    };
}

/*!
    \fn void QtOhosAppKit::QOhosOnContinueContext::setRejectResponse()

    Sets On Continue action as rejected.
    Rejected responses means that on continuation process should not be continued.
    This is typically used when the target application cannot handle the continuation
    request for any reason.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V13/js-apis-app-ability-abilityconstant-V13#oncontinueresult}
    {OnContinueResult}
*/
void QOhosOnContinueContextImpl::setRejectResponse()
{
    m_baseResponse = QOhosQpaFunctions::AbilityOnContinueResponse{
        .status = QOhosQpaFunctions::AbilityOnContinueResponseStatus::Reject,
        .wantObjectParams = {},
    };
}

/*!
    \fn void QOhosOnContinueContext::setMismatchResponse()

    Sets On Continue action as mismatched.
    Mismatched responses means that on continuation process should not be continued - most probably due to
    source and target application version code difference.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V13/js-apis-app-ability-abilityconstant-V13#oncontinueresult}
    {OnContinueResult}

    \sa sourceApplicationVersionCode()
*/
void QOhosOnContinueContextImpl::setMismatchResponse()
{
    m_baseResponse = QOhosQpaFunctions::AbilityOnContinueResponse{
        .status = QOhosQpaFunctions::AbilityOnContinueResponseStatus::Mismatch,
        .wantObjectParams = {},
    };
}

/*!
    \fn void QOhosOnContinueContext::setExitAppOnSourceDeviceAfterMigration(bool exitAfterMigration)

    Decides whether the application should automatically exit on the source device after successful
    migration to the target device.

    The default setting is determined by the platform. As of API17 it's set to "true" by default.
*/
void QOhosOnContinueContextImpl::setExitAppOnSourceDeviceAfterMigration(bool exitAfterMigration)
{
    m_exitAppOnSourceDeviceAfterMigration = exitAfterMigration;
}

QOhosQpaFunctions::AbilityOnContinueResponse QOhosOnContinueContextImpl::response() const
{
    auto response = m_baseResponse;
    response.exitAppOnSourceDeviceAfterMigration = m_exitAppOnSourceDeviceAfterMigration;
    return response;
}

/*!
    \fn int QtOhosAppKit::QOhosOnContinueContext::sourceApplicationVersionCode() const

    Returns source application version code. Source application is the one which has started
    the continuation process. This version code is devlivered as "version" property in
    onContinue wantParam.
*/
int QOhosOnContinueContextImpl::sourceApplicationVersionCode() const
{
    return m_sourceApplicationVersionCode;
}

QByteArray requestStartAbilityForResult(
    const QOhosWant &want, std::optional<QOhosQpaFunctions::StartOptions> options,
    QWindow *optInstanceMainWindow, QObject *callerContext, QByteArray requestId,
    QOhosConsumer<QByteArray, int, QSharedPointer<QOhosWant>> successConsumer,
    QOhosConsumer<QByteArray> errorConsumer)
{
    struct Context
    {
        QByteArray requestId;
        QOhosConsumer<QByteArray, int, QSharedPointer<QOhosWant>> successConsumer;
        QOhosConsumer<QByteArray> errorConsumer;
    };

    auto context = QtOhos::moveToSharedPtr(
        Context{
            .requestId = requestId,
            .successConsumer = std::move(successConsumer),
            .errorConsumer = std::move(errorConsumer),
        });

    QtOhos::getQOhosQpaFunctions().startAbilityForResult(
        convertWantToJsonObject(want), options, optInstanceMainWindow, callerContext,
        [context](std::optional<QOhosQpaFunctions::AbilityResult> optAbilityResult) {
            if (optAbilityResult.has_value()) {
                auto abilityResult = optAbilityResult.value();
                auto want = abilityResult.want.has_value()
                    ? QSharedPointer<QOhosWant>::create(convertWantFromJsonObject(abilityResult.want.value()))
                    : nullptr;
                context->successConsumer(context->requestId, abilityResult.resultCode, want);
            } else {
                context->errorConsumer(context->requestId);
            }
        });

    return context->requestId;
}

class QOhosBaseAbilityContextImpl : public QOhosAbilityContext
{
protected:
    QOhosBaseAbilityContextImpl();

    QByteArray generateUniqueId();

    QByteArray startAbilityForResultImpl(
        const QOhosWant &want, QWindow *optInstanceMainWindow,
        std::optional<QOhosQpaFunctions::StartOptions> qpaStartOptions);

    QByteArray shareDataWithShareKitImpl(
        QWindow *optMainWindow, const QList<QSharedPointer<ShareKit::QOhosSharedRecord>> &records,
        QSharedPointer<ShareKit::QOhosShareControllerOptions> controllerOptions);

private:
    QOhosSupplier<QByteArray> m_uniqueIdsGenerator;
    QHash<QByteArray, std::shared_ptr<void>> m_shareDataRequestsHandles;
};

class QOhosDefaultAbilityContextImpl : public QOhosBaseAbilityContextImpl
{
public:
    QOhosDefaultAbilityContextImpl();

    void setDestroyFromSystemEnabled(bool destroyEnabled) override;

    QByteArray startAbilityForResult(const QOhosWant &want) override;
    QByteArray startAbilityForResult(const QOhosWant &want, const QOhosStartOptions &options) override;
    QByteArray startAbilityForResult(const QOhosWant &want, const QOhosStartRequest &startRequest) override;

    QByteArray shareDataWithShareKit(
        const QList<QSharedPointer<ShareKit::QOhosSharedRecord>> &records,
        QSharedPointer<ShareKit::QOhosShareControllerOptions> controllerOptions) override;

    bool tryOpenLink(const QString &link) override;
    bool tryOpenLink(const QString &link, const QOhosOpenLinkOptions &options) override;

    void setContinuationActive(bool continuationActive) override;
};

class QOhosAbilityContextImpl : public QOhosBaseAbilityContextImpl
{
public:
    QOhosAbilityContextImpl(QWindow *instanceMainWindow);

    void setDestroyFromSystemEnabled(bool destroyEnabled) override;

    QByteArray startAbilityForResult(const QOhosWant &want) override;
    QByteArray startAbilityForResult(const QOhosWant &want, const QOhosStartOptions &options) override;
    QByteArray startAbilityForResult(const QOhosWant &want, const QOhosStartRequest &startRequest) override;

    QByteArray shareDataWithShareKit(
        const QList<QSharedPointer<ShareKit::QOhosSharedRecord>> &records,
        QSharedPointer<ShareKit::QOhosShareControllerOptions> controllerOptions) override;

    bool tryOpenLink(const QString &link) override;
    bool tryOpenLink(const QString &link, const QOhosOpenLinkOptions &options) override;

    void setContinuationActive(bool continuationActive) override;

private:
    QPointer<QWindow> m_instanceMainWindow;
};

std::map<QWindow *, QSharedPointer<QOhosAbilityContextImpl>> abilityContextsMap;

QOhosBaseAbilityContextImpl::QOhosBaseAbilityContextImpl()
    : m_uniqueIdsGenerator(makeUniqueIdsGenerator())
{
}

QByteArray QOhosBaseAbilityContextImpl::generateUniqueId()
{
    return m_uniqueIdsGenerator();
}

QByteArray QOhosBaseAbilityContextImpl::startAbilityForResultImpl(
    const QOhosWant &want, QWindow *optInstanceMainWindow,
    std::optional<QOhosQpaFunctions::StartOptions> qpaStartOptions)
{
    return requestStartAbilityForResult(
        want, std::move(qpaStartOptions), optInstanceMainWindow, this,
        generateUniqueId(),
        [this](auto requestIdentifier, auto resultCode, auto want) {
            Q_EMIT startAbilityForResultResponseReceived(requestIdentifier, resultCode, want);
        },
        [this](auto requestIdentifier) {
            Q_EMIT startAbilityForResultErrorResponseReceived(requestIdentifier);
        });
}

QByteArray QOhosBaseAbilityContextImpl::shareDataWithShareKitImpl(
    QWindow *optMainWindow, const QList<QSharedPointer<ShareKit::QOhosSharedRecord>> &records,
    QSharedPointer<ShareKit::QOhosShareControllerOptions> controllerOptions)
{
    auto selfPtr = QPointer<QOhosBaseAbilityContextImpl>(this);
    auto requestId = generateUniqueId();
    auto requestHandle = ShareKit::shareData(
        optMainWindow, records, controllerOptions,
        [selfPtr, requestId]() {
            if (!selfPtr.isNull()) {
                selfPtr->m_shareDataRequestsHandles.remove(requestId);
                Q_EMIT selfPtr->shareKitPanelClosed(requestId);
            }
        },
        [selfPtr, requestId](auto shareOperationResult) {
            if (!selfPtr.isNull())
                Q_EMIT selfPtr->shareKitCompleted(requestId, shareOperationResult);
        }
    );
    m_shareDataRequestsHandles.insert(requestId, requestHandle);

    return requestId;
}

QOhosDefaultAbilityContextImpl::QOhosDefaultAbilityContextImpl() = default;

void QOhosDefaultAbilityContextImpl::setDestroyFromSystemEnabled(bool destroyEnabled)
{
    qCDebug(QtForOhos, "%s: setting destroyEnabled=%d for all instances", Q_FUNC_INFO, destroyEnabled);

    std::vector<QObject *> instancesMainWindows;
    for (const auto &contextEntry : abilityContextsMap)
        instancesMainWindows.push_back(contextEntry.first);

    QtOhos::getQOhosQpaFunctions().setDestroyAllowedFlagForAbilityInstances(instancesMainWindows, destroyEnabled);
}

QByteArray QOhosDefaultAbilityContextImpl::startAbilityForResult(const QOhosWant &want)
{
    return startAbilityForResultImpl(want, nullptr, std::nullopt);
}

QByteArray QOhosDefaultAbilityContextImpl::startAbilityForResult(
    const QOhosWant &want, const QOhosStartOptions &options)
{
    return startAbilityForResultImpl(
        want, nullptr, tryConvertStartOptionsToQpaFunctionsStruct(options));
}

QByteArray QOhosDefaultAbilityContextImpl::startAbilityForResult(
    const QOhosWant &want, const QOhosStartRequest &startRequest)
{
    return startAbilityForResultImpl(
        want, nullptr, tryConvertStartRequestToQpaFunctionsStruct(startRequest));
}

QByteArray QOhosDefaultAbilityContextImpl::shareDataWithShareKit(
    const QList<QSharedPointer<ShareKit::QOhosSharedRecord>> &records,
    QSharedPointer<ShareKit::QOhosShareControllerOptions> controllerOptions)
{
    return shareDataWithShareKitImpl(nullptr, records, controllerOptions);
}

bool QOhosDefaultAbilityContextImpl::tryOpenLink(const QString &link)
{
    return QtOhos::getQOhosQpaFunctions().tryOpenLink(nullptr, link, {});
}

bool QOhosDefaultAbilityContextImpl::tryOpenLink(const QString &link, const QOhosOpenLinkOptions &options)
{
    const auto &optionsImpl = static_cast<const QOhosOpenLinkOptionsImpl &>(options);
    return QtOhos::getQOhosQpaFunctions().tryOpenLink(nullptr, link, optionsImpl.appLinkingOnly());
}

void QOhosDefaultAbilityContextImpl::setContinuationActive(bool continuationActive)
{
    QtOhos::getQOhosQpaFunctions().setAbilityContinuationActive(nullptr, continuationActive);
}

QOhosAbilityContextImpl::QOhosAbilityContextImpl(QWindow *instanceMainWindow)
    : m_instanceMainWindow(instanceMainWindow)
{
    QtOhos::getQOhosQpaFunctions().setOnContinueRequestsHandlerForAbilityInstanceWindow(
        instanceMainWindow,
        [self = QPointer<QOhosAbilityContextImpl>(this)](auto request, auto responseConsumer) {
            auto context = QSharedPointer<QOhosOnContinueContextImpl>::create(request.sourceApplicationVersionCode);

            if (!self.isNull())
                Q_EMIT self->continueRequestReceived(context);
            else
                context->setRejectResponse();

            QtOhos::invokeInQtThread(
                [response = context->response(), responseConsumer = std::move(responseConsumer)]() {
                    responseConsumer(response);
                });
        });
}

/*!
    \fn void QtOhosAppKit::QOhosAbilityContext::setDestroyFromSystemEnabled(bool destroyEnabled)

    Sets whether the Ability can be automatically destroyed by the system when the user clicks on
    the window's "close" button. If \a destroyEnabled is \c true, the system destroys the Ability
    automatically (and Qt needs to adapt to this). If \a destroyEnabled = \c false, the window is
    not automatically destroyed, but instead standard Qt path for window close is triggered, i.e.
    QWindow::close().

    By default, the flag is set to \c false.

    When called on the default QOhosAbilityContext instance it sets the flag to \a destroyEnabled
    for all Ability instances.
*/
void QOhosAbilityContextImpl::setDestroyFromSystemEnabled(bool destroyEnabled)
{
    if (m_instanceMainWindow.isNull()) {
        qCWarning(QtForOhos, "%s: called on destroyed instance, ignoring", Q_FUNC_INFO);
        return;
    }

    qCDebug(
        QtForOhos, "%s: setting destroyEnabled=%d for window %p",
        Q_FUNC_INFO, destroyEnabled, m_instanceMainWindow.data());

    QtOhos::getQOhosQpaFunctions().setDestroyAllowedFlagForAbilityInstances(
        {m_instanceMainWindow.data()}, destroyEnabled);
}

/*!
    \fn QByteArray QtOhosAppKit::QOhosAbilityContext::startAbilityForResult(const QtOhosAppKit::QOhosWant &want)

    Starts a UIAbility with a given \a want and delivers the result using startAbilityForResultResponseReceived() or
    startAbilityForResultErrorResponseReceived() signal. To start the UIAbility, at least bundleName and abilityName properties must be set.
    Returns request identifier which will be also provided by emitted signal \sa startAbilityForResultResponseReceived(), startAbilityForResultErrorResponseReceived().
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-inner-application-uiabilitycontext#startabilityforresult-2}
    {UIAbilityContext.startAbilityForResult}
*/
QByteArray QOhosAbilityContextImpl::startAbilityForResult(const QOhosWant &want)
{
    return startAbilityForResultImpl(want, m_instanceMainWindow, std::nullopt);
}

/*!
    \fn QByteArray QtOhosAppKit::QOhosAbilityContext::startAbilityForResult(const QtOhosAppKit::QOhosWant &want, const QtOhosAppKit::QOhosStartOptions &options)

    Starts a UIAbility with a given \a want and \a options and delivers the result using startAbilityForResultResponseReceived() or
    startAbilityForResultErrorResponseReceived() signal. To start the UIAbility, at least bundleName and abilityName properties must be set.
    Returns request identifier which will be also provided by emitted signal \sa startAbilityForResultResponseReceived(), startAbilityForResultErrorResponseReceived().
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-inner-application-uiabilitycontext#startabilityforresult-2}
    {UIAbilityContext.startAbilityForResult}
*/
QByteArray QOhosAbilityContextImpl::startAbilityForResult(
    const QOhosWant &want, const QOhosStartOptions &options)
{
    return startAbilityForResultImpl(
        want, m_instanceMainWindow, tryConvertStartOptionsToQpaFunctionsStruct(options));
}

/*!
    \fn QByteArray QtOhosAppKit::QOhosAbilityContext::startAbilityForResult(const QtOhosAppKit::QOhosWant &want,
    const QtOhosAppKit::QOhosStartRequest &startRequest)

    Starts a UIAbility with a given \a want and \a startRequest and delivers the result using startAbilityForResultResponseReceived() or
    startAbilityForResultErrorResponseReceived() signal. To start the UIAbility, at least bundleName and abilityName properties must be set.

    The \a startRequest carries the completion handler from start options. Connect to
    QOhosStartRequest::requestSucceeded() and QOhosStartRequest::requestFailed() to receive
    completion handler results.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-inner-application-uiabilitycontext#startabilityforresult-2}
    {UIAbilityContext.startAbilityForResult}
*/
QByteArray QOhosAbilityContextImpl::startAbilityForResult(
    const QOhosWant &want, const QOhosStartRequest &startRequest)
{
    return startAbilityForResultImpl(
        want, m_instanceMainWindow, tryConvertStartRequestToQpaFunctionsStruct(startRequest));
}

/*!
    \fn virtual void QtOhosAppKit::QOhosAbilityContext::shareDataWithShareKit(const QList<QSharedPointer<ShareKit::QOhosSharedRecord>> &records, QSharedPointer<ShareKit::QOhosShareControllerOptions> controllerOptions) = 0

    Share provided \a records with other applications using an inter-application mechanism called ShareKit. Share Kit panel can be controlled
    with a given \a controllerOptions. When called on the default QOhosAbilityContext instance, it shares \a records using the default UiAbility.

    Returns request identifier which will be passed as an argument of the corresponding \sa shareKitPanelClosed() or \sa shareKitCompleted() signal.
    shareKitPanelClosed() signal is emitted when the sharing panel is closed.
    shareKitCompleted() signal is called when User selects application for sharing (can be called multiple times).

    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-guides-V5/share-introduction-V5}{Share Kit}
*/
QByteArray QOhosAbilityContextImpl::shareDataWithShareKit(
    const QList<QSharedPointer<ShareKit::QOhosSharedRecord>> &records,
    QSharedPointer<ShareKit::QOhosShareControllerOptions> controllerOptions)
{
    return shareDataWithShareKitImpl(m_instanceMainWindow, records, controllerOptions);
}

/*!
    \fn virtual bool QtOhosAppKit::QOhosAbilityContext::tryOpenLink(const QString &link) = 0

    Use provided \a link to open application with a deep link.
    Returns true if successful, false otherwise.

    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-guides/app-linking-startup}{App Linking}
*/
bool QOhosAbilityContextImpl::tryOpenLink(const QString &link)
{
    return QtOhos::getQOhosQpaFunctions().tryOpenLink(m_instanceMainWindow, link, {});
}

/*!
    \fn virtual bool QtOhosAppKit::QOhosAbilityContext::tryOpenLink(const QString &link, const QOhosOpenLinkOptions &options) = 0

    Use provided \a link and \a options to open application with a deep link.
    Returns true if successful, false otherwise.

    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-guides/app-linking-startup}{App Linking}
*/
bool QOhosAbilityContextImpl::tryOpenLink(const QString &link, const QOhosOpenLinkOptions &options)
{
    const auto &optionsImpl = static_cast<const QOhosOpenLinkOptionsImpl &>(options);
    return QtOhos::getQOhosQpaFunctions().tryOpenLink(m_instanceMainWindow, link, optionsImpl.appLinkingOnly());
}

/*!
    \fn virtual void QtOhosAppKit::QOhosAbilityContext::setContinuationActive(bool continuationActive)

    Sets the mission continuation state of the underlying Ability instance as per the
    \a continuationActive parameter (\c true = active, \c false = inactive).

    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-inner-application-uiabilitycontext#setmissioncontinuestate10-1}{setMissionContinueState}
*/
void QOhosAbilityContextImpl::setContinuationActive(bool continuationActive)
{
    QtOhos::getQOhosQpaFunctions().setAbilityContinuationActive(m_instanceMainWindow, continuationActive);
}

QSharedPointer<QOhosOperationStatus> startAbilityImpl(
    const QOhosWant &want, std::optional<QOhosQpaFunctions::StartOptions> qpaStartOptions)
{
    bool success = QtOhos::getQOhosQpaFunctions().startAbility(
        convertWantToJsonObject(want), std::move(qpaStartOptions));
    return createOperationStatus(success);
}

void startAppProcessImpl(
    const QString &processId, const QOhosWant &requestWant,
    std::optional<QOhosQpaFunctions::StartOptions> qpaStartOptions)
{
    QtOhos::getQOhosQpaFunctions().startAppProcess(
        processId, convertWantToJsonObject(requestWant), std::move(qpaStartOptions));
}

}

/*!
    \class QtOhosAppKit::QOhosAbilityContext
    \inmodule QtOhosAppKit
    \since 5.12.12
    \brief The QOhosAbilityContext class is to manage native UI Ability context. See
    \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-inner-application-uiabilitycontext-V5}
    {UIAbilityContext}.
*/

/*!
    \fn void QtOhosAppKit::QOhosAbilityContext::newWantReceived(QOhosWant want)

    Singal is emitted when an ability gets new \a want. It is recommended to use newer newWantInfoReceived() signal.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-app-ability-uiability-V5#uiabilityonnewwant}
    {On New Want}.

    \sa newWantInfoReceived()
*/

/*!
    \fn void QtOhosAppKit::QOhosAbilityContext::newWantInfoReceived(QSharedPointer<QtOhosAppKit::QOhosWantInfo> wantInfo)

    Singal is emitted when an ability gets new \a want. See
    \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-app-ability-uiability-V5#uiabilityonnewwant}
    {On New Want}.
*/

/*!
    \fn void QtOhosAppKit::QOhosAbilityContext::continueRequestReceived(QSharedPointer<QtOhosAppKit::QOhosOnContinueContext> onContinueContext)

    Signal emitted when the ability receives onContinue request. The signal delivers \a onContinueContext on which User
    can set onContinue response.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V13/js-apis-app-ability-uiability-V13#uiabilityoncontinue}
    {UIAbility onContinue}.

    \sa setAgreeResponse(), setMismatchResponse(), setRejectResponse()
*/

/*!
    \fn void QtOhosAppKit::QOhosAbilityContext::startAbilityForResultResponseReceived(QByteArray requestId, int resultCode, QSharedPointer<QOhosWant> optWant)

    Signal emitted when started UIAbility is terminated. User request can be identified with \a requestId.
    External application sets the result which is provided by \a resultCode and optionally \a optWant with extra data (the \a optWant is null if Want object is not available).
    See {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-inner-application-uiabilitycontext#startabilityforresult-2}
    {UIAbilityContext.startAbilityForResult} and
    {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-inner-application-uiabilitycontext#terminateselfwithresult}
    {UIAbilityContext.terminateSelfWithResult}
*/

/*!
    \fn void QtOhosAppKit::QOhosAbilityContext::startAbilityForResultErrorResponseReceived(QByteArray requestId)

    Signal emitted when started UIAbility got an exception for example, it was killed. Start ability User request can be identified with \a requestId.
    See {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-inner-application-uiabilitycontext#startabilityforresult-2}
    {UIAbilityContext.startAbilityForResult} and
    {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-inner-application-uiabilitycontext#terminateselfwithresult}
    {UIAbilityContext.terminateSelfWithResult}
*/

/*!
    \fn void QtOhosAppKit::QOhosAbilityContext::shareKitPanelClosed(QByteArray requestId)

    Signal emitted when the sharing panel (invoked via shareDataWithShareKit) closes.
    It corresponds to OH ShareController's "dismiss" event.

    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section147001858124512}{ShareController}
*/

/*!
    \fn void QtOhosAppKit::QOhosAbilityContext::shareKitCompleted(QByteArray requestId, QSharedPointer<ShareKit::QOhosShareOperationResult> shareOperationResult)

    Signal emitted when share operation is completed.
    Please note that the shareKitCompleted() can be called multiple times as the User can change the target application.

    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section238319917154}{ShareController}
*/

QOhosAbilityContext::QOhosAbilityContext()
{
    QtOhos::getQOhosQpaFunctions().addNewWantConsumer(
        this,
        [this](QJsonObject jsonWant) {
            auto want = convertWantFromJsonObject(jsonWant);
            Q_EMIT newWantReceived(want);
        });

    QtOhos::getQOhosQpaFunctions().addNewWantConsumer(
        this,
        [this](QSharedPointer<QOhosQpaFunctions::WantInfo> wantInfo) {
            Q_EMIT newWantInfoReceived(convertToOhosAppKitWantInfo(wantInfo));
        });
}

/*!
    \fn QOhosAbilityContext *QtOhosAppKit::QOhosAbilityContext::instance()

    Gets QOhosAbilityContext global default instance.

    \sa getDefaultInstance()
*/
QOhosAbilityContext *QOhosAbilityContext::instance()
{
    return QOhosAbilityContext::getDefaultInstance().data();
}

/*!
    \fn QSharedPointer<QOhosAbilityContext> QtOhosAppKit::QOhosAbilityContext::getDefaultInstance()

    Returns instance of the class which is not connected to any specific Ability instance. It should
    be used when the application needs to perform some operations without selecting specific
    Ability instance (via the corresponding main window).

    See descriptions of specific class methods for information about their behavior for the default
    instance.
*/
QSharedPointer<QOhosAbilityContext> QOhosAbilityContext::getDefaultInstance()
{
    static auto instance = QSharedPointer<QOhosDefaultAbilityContextImpl>::create();
    return instance;
}

/*!
    \fn QSharedPointer<QOhosAbilityContext> QtOhosAppKit::QOhosAbilityContext::getInstanceForMainWindow(QWindow *instanceMainWindow)

    Returns instance of the class which is connected to Ability instance identified by the
    \a instanceMainWindow. Methods called on the returned object will only affect the
    corresponding Ability instance.
*/
QSharedPointer<QOhosAbilityContext> QOhosAbilityContext::getInstanceForMainWindow(QWindow *instanceMainWindow)
{
    if (instanceMainWindow == nullptr) {
        qCWarning(QtForOhos, "%s: got null QWindow", Q_FUNC_INFO);
        return QSharedPointer<QOhosAbilityContextImpl>::create(nullptr);
    }

    auto abilityContextIter = abilityContextsMap.find(instanceMainWindow);
    if (abilityContextIter == abilityContextsMap.end()) {
        std::tie(abilityContextIter, std::ignore) = abilityContextsMap.emplace(
            instanceMainWindow, QSharedPointer<QOhosAbilityContextImpl>::create(instanceMainWindow));
            QObject::connect(
                instanceMainWindow, &QObject::destroyed,
                [instanceMainWindow](QObject *) {
                    abilityContextsMap.erase(instanceMainWindow);
                });
    }

    return abilityContextIter->second;
}

/*!
    \fn QSharedPointer<QOhosOperationStatus> QtOhosAppKit::startAbility(const QOhosWant &want)

    Starts an Ability for a given \a want. See
    \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-inner-application-uiabilitycontext-V5#uiabilitycontextstartability}
    {Start Ability}.

    \warning Currently, operation status result is hardcoded as "successful" (even if ability were
    not started).
*/
QSharedPointer<QOhosOperationStatus> startAbility(const QOhosWant &want)
{
    return startAbilityImpl(want, std::nullopt);
}

/*!
    \fn QSharedPointer<QOhosOperationStatus> QtOhosAppKit::startAbility(const QOhosWant &want,
    const QOhosStartOptions &options)

    Starts an Ability for a given \a want and \a options. See
    \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-inner-application-uiabilitycontext-V5#uiabilitycontextstartability-1}
    {Start Ability}.

    \warning Currently, operation status result is hardcoded as "successful" (even if ability were
    not started).
*/
QSharedPointer<QOhosOperationStatus> startAbility(const QOhosWant &want, const QOhosStartOptions &options)
{
    return startAbilityImpl(want, tryConvertStartOptionsToQpaFunctionsStruct(options));
}

/*!
    \fn QSharedPointer<QOhosOperationStatus> QtOhosAppKit::startAbility(const QOhosWant &want,
    const QOhosStartRequest &startRequest)

    Starts an Ability for a given \a want and \a startRequest.

    The \a startRequest carries the completion handler from start options. Connect to
    QOhosStartRequest::requestSucceeded() and QOhosStartRequest::requestFailed() to receive
    completion handler results. See
    \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-inner-application-uiabilitycontext-V5#uiabilitycontextstartability-1}
    {Start Ability}.
*/
QSharedPointer<QOhosOperationStatus> startAbility(
    const QOhosWant &want, const QOhosStartRequest &startRequest)
{
    return startAbilityImpl(want, tryConvertStartRequestToQpaFunctionsStruct(startRequest));
}

/*!
    \fn QSharedPointer<QOhosOperationStatus> QtOhosAppKit::startAbilityByType(const QString &appType,
    const QJsonObject &wantParameters)

    Starts an Ability for a given \a appType and \a wantParameters. See
    \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-inner-application-uiabilitycontext-V5#uiabilitycontextstartabilitybytype11-1}
    {Start Ability}.

    \return true on success
*/
QSharedPointer<QOhosOperationStatus> startAbilityByType(const QString &appType, const QJsonObject &wantParameters)
{
    bool success = QtOhos::getQOhosQpaFunctions().startAbilityByType(appType, wantParameters);
    return createOperationStatus(success);
}

/*!
    Starts another instance of the UIAbility used by the Qt app with specified widget inside

    The caller should pass newly created QWidget \a instanceWidget, without any setting any parent, calling show() or winId() on it.
*/
void startNewAbilityInstance(QWidget *instanceWidget)
{
    instanceWidget->show();
}

/*!
    \fn void QtOhosAppKit::startAppProcess(const QString &processId, const QOhosWant &requestWant)

    Starts application process for a given \a processId and \a requestWant.
*/
void startAppProcess(const QString &processId, const QOhosWant &requestWant)
{
    startAppProcessImpl(processId, requestWant, std::nullopt);
}

/*!
    \fn void QtOhosAppKit::startAppProcess(const QString &processId, const QOhosWant &requestWant, const QOhosStartOptions &options)

    Starts application process for a given \a processId, \a requestWant and \a options.
*/
void startAppProcess(const QString &processId, const QOhosWant &requestWant, const QOhosStartOptions &options)
{
    startAppProcessImpl(processId, requestWant, tryConvertStartOptionsToQpaFunctionsStruct(options));
}

/*!
    \fn void QtOhosAppKit::startAppProcess(const QString &processId, const QOhosWant &requestWant, const QOhosStartRequest &startRequest)

    Starts application process for a given \a processId, \a requestWant and \a startRequest.
    The \a startRequest carries the completion handler from start options. Connect to
    QOhosStartRequest::requestSucceeded() and QOhosStartRequest::requestFailed() to receive
    completion handler results.

    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-inner-application-uiabilitycontext-V5#uiabilitycontextstartability-1}
    {Start Ability}.

    \sa QOhosStartRequest
*/
void startAppProcess(
    const QString &processId, const QOhosWant &requestWant, const QOhosStartRequest &startRequest)
{
    startAppProcessImpl(
        processId, requestWant, tryConvertStartRequestToQpaFunctionsStruct(startRequest));
}

/*!
    \fn void QtOhosAppKit::setAbilityInstanceDestroyEnabled(QWindow *instanceWindow, bool destroyEnabled)

    Sets whether the Ability related with \a instanceWindow can be automatically destroyed by the
    system when the user clicks on the window's "close" button. If \a destroyEnabled is \c true,
    the system destroys the Ability automatically (and Qt needs to adapt to this).
    If \a destroyEnabled = \c false, the window is not automatically destroyed, but instead standard
    Qt path for window close is triggered, i.e. QWindow::close().

    By default, the flag is set to \c false.

    \sa QtOhosAppKit::QOhosAbilityContext::setDestroyFromSystemEnabled()
*/
void setAbilityInstanceDestroyEnabled(QWindow *instanceWindow, bool destroyEnabled)
{
    QOhosAbilityContext::getInstanceForMainWindow(instanceWindow)->setDestroyFromSystemEnabled(destroyEnabled);
}

/*!
    \fn QSharedPointer<QByteArray> tryGetOnContinueData(const QOhosWant &want)

    Tries to get continuation / migration related data that was provided on the source device.
    Returns \c nullptr if no such data found. The data is expected to be stored in the \a want parameters.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-guides/app-continuation-guide}
    {Application Continuation}.
*/
QSharedPointer<QByteArray> tryGetOnContinueData(const QOhosWant &want)
{
    auto key = QString::fromUtf8(qtOnContinueMigrationDataPropertyName);
    if (want.parameters.contains(key)) {
        auto base64String = want.parameters.value(key).toString();
        auto decodedData = QByteArray::fromBase64(base64String.toUtf8());
        return QSharedPointer<QByteArray>::create(decodedData);
    } else {
        return nullptr;
    }
}

/*!
    \class QtOhosAppKit::QOhosOnContinueContext
    \inmodule QtOhosAppKit
    \since 5.12.12
    \brief The QOhosOnContinueContext class manages onContinue context. It provides system
    data, like source application version code and set the onContinue result that is requested by the system.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V13/js-apis-app-ability-uiability-V13#uiabilityoncontinue}
    {UIAbility onContinue}.
*/
QtOhosAppKit::QOhosOnContinueContext::QOhosOnContinueContext() = default;
QtOhosAppKit::QOhosOnContinueContext::~QOhosOnContinueContext() = default;

QOhosOpenLinkOptions::QOhosOpenLinkOptions() = default;
QOhosOpenLinkOptions::~QOhosOpenLinkOptions() = default;

QSharedPointer<QOhosOpenLinkOptions> createOpenLinkOptions()
{
    return QSharedPointer<QOhosOpenLinkOptionsImpl>::create();
}

}

QT_END_NAMESPACE
