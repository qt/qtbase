// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosfileshare.h"
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include "qohosqpafunctionspart1_p.h"
#include <QtCore/qmetaobject.h>
#include <QtOhosAppKit/private/qohosoperationstatus_p.h>
#include <cstring>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

using QOhosQpaFunctions = QtOhos::QOhosQpaFunctionsPart1;

/*!
    \namespace QtOhosAppKit::FileShare
    \inmodule QtOhosAppKit
    \since 5.12.12
    \brief The FileShare to expose file permission API.

    This API uses file system paths, which differ from the URIs used internally by HarmonyOS. For example, the path
    \c /data/storage/el1/bundle/entry/resources/resfile/test.txt
    will be converted to the URI
    \c file://com.your.example/data/storage/el1/bundle/entry/resources/resfile/test.txt , where \c com.your.example is app bundle name.

    To learn more about file paths and URIs in HarmonyOS, see
    \l{https://developer.huawei.com/consumer/en/doc/harmonyos-guides-V5/app-sandbox-directory-V5} {sandbox documentation}.

    \warning
    This API relies on the underlying OHOS implementation.
    The following functions: persistPermission(), revokePermission(), activatePermission(), deactivatePermission(), and checkPersistent() may be unavailable on some devices.
    Additionally, there may be limitations on the maximum number of policies, and certain permissions may need to be explicitly granted to the application.

    For more details and limitations, refer to the
    \l{https://developer.huawei.com/consumer/en/doc/harmonyos-guides-V14/native-fileshare-guidelines-V14} {API documentation}.
*/
namespace FileShare {

namespace {

std::optional<QOhosQpaFunctions::FileShare::OperationMode> tryMapOperationModeToInternal(OperationMode operationMode)
{
    switch (operationMode) {
    case OperationMode::Read:
        return std::make_optional(QOhosQpaFunctions::FileShare::OperationMode::Read);
    case OperationMode::Write:
        return std::make_optional(QOhosQpaFunctions::FileShare::OperationMode::Write);
    }
    return {};
}

std::set<QOhosQpaFunctions::FileShare::OperationMode> mapOperationModesToInternalSet(OperationModes operationModes)
{
    auto operationModeMeta = QMetaEnum::fromType<OperationMode>();

    std::set<QOhosQpaFunctions::FileShare::OperationMode> internalSet;
    for (int i = 0; i < operationModeMeta.keyCount(); ++i) {
        auto testedOperationMode = static_cast<OperationMode>(operationModeMeta.value(i));
        auto optInternalValue = tryMapOperationModeToInternal(testedOperationMode);
        if (operationModes.testFlag(testedOperationMode) && optInternalValue.has_value())
            internalSet.insert(optInternalValue.value());
    }

    return internalSet;
}

QList<QOhosQpaFunctions::FileShare::PolicyInfo> convertToPolicyInfos(const QList<PathPolicy> &policies)
{
    QList<QOhosQpaFunctions::FileShare::PolicyInfo> policyInfos;
    for (const auto &policy : policies) {
        policyInfos.append(
            {
                .path = policy.path,
                .operationModes = mapOperationModesToInternalSet(policy.operationModes),
            });
    }

    return policyInfos;
}

std::optional<PathPolicyError> tryMapPolicyErrorCodeToPathPolicyError(
    QOhosQpaFunctions::FileShare::PolicyErrorCode errorCode)
{
    using PolicyErrorCode = QOhosQpaFunctions::FileShare::PolicyErrorCode;
    switch (errorCode) {
    case PolicyErrorCode::PERSISTENCE_FORBIDDEN:
        return std::make_optional(PathPolicyError::PersistenceForbidden);
    case PolicyErrorCode::INVALID_MODE:
        return std::make_optional(PathPolicyError::InvalidMode);
    case PolicyErrorCode::INVALID_PATH:
        return std::make_optional(PathPolicyError::InvalidPath);
    case PolicyErrorCode::PERMISSION_NOT_PERSISTED:
        return std::make_optional(PathPolicyError::PermissionNotPersisted);
    }
    return std::nullopt;
}

QList<PathPolicyErrorInfo> convertToPathPolicyErrorInfos(
    const QList<QOhosQpaFunctions::FileShare::PolicyErrorResult> &policyErrorResults)
{
    QList<PathPolicyErrorInfo> result;
    for (const auto &policyErrorResult : policyErrorResults) {
        result.push_back({
            .path = policyErrorResult.path,
            .error =
                policyErrorResult.error.has_value()
                ? tryMapPolicyErrorCodeToPathPolicyError(policyErrorResult.error.value())
                      .value_or(PathPolicyError::Unknown)
                : PathPolicyError::Unknown,
            .errorMessage = policyErrorResult.errorMessage,
        });
    }

    return result;
}

QList<PathPolicyCheckResult> convertToPathPolicyCheckResults(
    const std::vector<bool> &checkResults,
    const QList<PathPolicy> &policies)
{
    QList<PathPolicyCheckResult> result;
    for (std::size_t i = 0; i < checkResults.size() && policies.size(); ++i)
        result.push_back({policies[i], checkResults[i]});

    return result;
}

bool validateCheckResultsAgainstPolicies(
    const std::vector<bool> &checkResults,
    const QList<PathPolicy> &policies)
{
    return checkResults.size() == static_cast<std::size_t>(policies.size());
}

/*!
    \class QtOhosAppKit::FileShare::ActionResult
    \inmodule QtOhosAppKit
    \since 5.12.12
    \brief The ActionResult class encapsulates the result of all file requested access permission actions.

    It contains any errors that occurred during the execution of these actions.

    \sa persistPermission(), revokePermission(), activatePermission(), deactivatePermission()
*/
class ActionResultImpl : public ActionResult
{
public:
    ActionResultImpl(
        bool successFlag, const QList<QOhosQpaFunctions::FileShare::PolicyErrorResult> &errorResults);
    QSharedPointer<QOhosOperationStatus> operationStatus() const override;
    QList<PathPolicyErrorInfo> errorInfoList() const override;

private:
    QSharedPointer<QOhosOperationStatus> m_operationStatus;
    QList<PathPolicyErrorInfo> m_errorInfoList;
};

ActionResultImpl::ActionResultImpl(
    bool successFlag, const QList<QOhosQpaFunctions::FileShare::PolicyErrorResult> &errorResults)
    : m_operationStatus(createOperationStatus(successFlag))
    , m_errorInfoList(convertToPathPolicyErrorInfos(errorResults))
{
}

/*!
    \fn QSharedPointer<QOhosOperationStatus> QtOhosAppKit::FileShare::ActionResult::operationStatus() const

    Returns the overall result of all requested file access permission actions.

    \sa QOhosOperationStatus
*/
QSharedPointer<QOhosOperationStatus> ActionResultImpl::operationStatus() const
{
    return m_operationStatus;
}

/*!
    \fn QList<PathPolicyErrorInfo> QtOhosAppKit::FileShare::ActionResult::errorInfoList() const

    Returns a list of errors for each file access action.

    Each entry in the list corresponds to a specific file operation that encountered an error.

    \sa PathPolicyErrorInfo
*/
QList<PathPolicyErrorInfo> ActionResultImpl::errorInfoList() const
{
    return m_errorInfoList;
}

/*!
    \class QtOhosAppKit::FileShare::CheckResult
    \inmodule QtOhosAppKit
    \since 5.12.12
    \brief The CheckResult class encapsulates the result of all requested file access permission checks.

    \sa checkPersistent()
*/
class CheckResultImpl : public CheckResult
{
public:
    CheckResultImpl(
        bool successFlag, const std::vector<bool> &checkResults,
        const QList<PathPolicy> &policies);
    QSharedPointer<QOhosOperationStatus> operationStatus() const override;
    QList<PathPolicyCheckResult> checkResultList() const override;

private:
    QSharedPointer<QOhosOperationStatus> m_operationStatus;
    QList<PathPolicyCheckResult> m_checkResultList;
};

CheckResultImpl::CheckResultImpl(
    bool successFlag, const std::vector<bool> &checkResults,
    const QList<PathPolicy> &policies)
{
    if (validateCheckResultsAgainstPolicies(checkResults, policies)) {
        m_operationStatus = createOperationStatus(successFlag);
        m_checkResultList = convertToPathPolicyCheckResults(checkResults, policies);
    } else {
        qOhosPrintfWarning(
            "%s: didn't get check results for all requested paths. "
            "Got %lld check results, but requested checks for %lld paths.",
            Q_FUNC_INFO, static_cast<long long>(checkResults.size()),
            static_cast<long long>(policies.size()));
        m_operationStatus = createOperationStatus(false);
    }
}

/*!
    \fn QSharedPointer<QOhosOperationStatus> QtOhosAppKit::FileShare::CheckResult::operationStatus() const

    Returns the overall result of all requested file access permission checks.

    \sa QOhosOperationStatus
*/
QSharedPointer<QOhosOperationStatus> CheckResultImpl::operationStatus() const
{
    return m_operationStatus;
}

/*!
    \fn QList<PathPolicyCheckResult> QtOhosAppKit::FileShare::CheckResult::checkResultList() const

    Returns a list of results for each individual file access permission check.

    Each entry in the list corresponds to a specific file and its check result.

    \sa PathPolicyCheckResult
*/
QList<PathPolicyCheckResult> CheckResultImpl::checkResultList() const
{
    return m_checkResultList;
}

}

/*!
    \enum QtOhosAppKit::FileShare::OperationMode
    \since 5.12.12

    Defines permissions on path. See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/file_share-V5#fileshare_operationmode}
    {FileShare_OperationMode}. The flags can be combined using bitwise operations.

    \value Read Read on path
    \value Write Write on path

    \sa PathPolicy
*/

/*!
    \enum QtOhosAppKit::FileShare::PathPolicyError
    \since 5.12.12

    Defines permission policy error codes. See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/file_share-V5#fileshare_policyerrorcode}
    {FileShare_PolicyErrorCode}.

    \value Unknown Error type is unknown or not recognized
    \value PersistenceForbidden The permission on the path cannot be persisted
    \value InvalidMode Invalid operation mode
    \value InvalidPath Invalid path
    \value PermissionNotPersisted The permission is not persisted

    \sa PathPolicyErrorInfo
*/

/*!
    \class QtOhosAppKit::FileShare::PathPolicy
    \inmodule QtOhosAppKit
    \since 5.12.12
    \brief The PathPolicy struct encapsulates information required to request file access permissions for a specified path.
*/

/*!
    \variable QtOhosAppKit::FileShare::PathPolicy::path
    \brief The path for which access permission is requested.
*/

/*!
    \variable QtOhosAppKit::FileShare::PathPolicy::operationModes
    \brief The types of access being requested for the file.

    \sa OperationMode
*/

/*!
    \class QtOhosAppKit::FileShare::PathPolicyErrorInfo
    \inmodule QtOhosAppKit
    \since 5.12.12
    \brief The PathPolicyErrorInfo struct contains the result of a file access permission request for a specified path.

    \sa ActionResult
*/

/*!
    \variable QtOhosAppKit::FileShare::PathPolicyErrorInfo::path
    \brief The path for which access permission was requested.
*/

/*!
    \variable QtOhosAppKit::FileShare::PathPolicyErrorInfo::error
    \brief The error code indicating the result of the file access permission request.

    \sa PathPolicyError
*/

/*!
    \variable QtOhosAppKit::FileShare::PathPolicyErrorInfo::errorMessage
    \brief Detailed error message.
*/

/*!
    \class QtOhosAppKit::FileShare::PathPolicyCheckResult
    \inmodule QtOhosAppKit
    \since 5.12.12
    \brief The PathPolicyCheckResult struct contains the result of a file permission check request for a specified path, indicating whether access is granted or denied.
*/

/*!
    \variable QtOhosAppKit::FileShare::PathPolicyCheckResult::policy
    \brief The requested path and the corresponding operation mode information.

    \sa PathPolicy
*/

/*!
    \variable QtOhosAppKit::FileShare::PathPolicyCheckResult::result
    \brief The outcome of the permission check for the requested path and operation mode.
*/

ActionResult::ActionResult() = default;

ActionResult::~ActionResult() = default;

CheckResult::CheckResult() = default;

CheckResult::~CheckResult() = default;

/*!
    \fn QSharedPointer<ActionResult> QtOhosAppKit::FileShare::persistPermission(const QList<PathPolicy> &policies)

    Persists file or folder permissions from \a policies.
    The success of the action can be determined from ActionResult.

    This function calls \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/file_share-V5#oh_fileshare_persistpermission} {OH_FileShare_PersistPermission}.
    persistPermission() stores persistence data in the system database. After persistPermission() call, permissions are not yet active. Use activatePermission() to activate permissions.
    persistPermission() must be called at least once before activatePermission().

    To revoke a persistent permission, use revokePermission().

    \sa ActionResult, PathPolicy, revokePermission(), activatePermission()
*/
QSharedPointer<ActionResult> persistPermission(const QList<PathPolicy> &policies)
{
    bool successFlag;
    QList<QOhosQpaFunctions::FileShare::PolicyErrorResult> errorResults;
    std::tie(successFlag, errorResults) =
        QtOhos::getQOhosQpaFunctions().persistPermission(convertToPolicyInfos(policies));
    return QSharedPointer<ActionResultImpl>::create(successFlag, errorResults);
}

/*!
    \fn QSharedPointer<ActionResult> QtOhosAppKit::FileShare::revokePermission(const QList<PathPolicy> &policies)

    Revokes file or folder permissions from \a policies.
    The success of the action can be determined from ActionResult.

    This function calls \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/file_share-V5#oh_fileshare_revokepermission} {OH_FileShare_RevokePermission}.

    To persist permission, use persistPermission().

    \sa ActionResult, PathPolicy, persistPermission()
*/
QSharedPointer<ActionResult> revokePermission(const QList<PathPolicy> &policies)
{
    bool successFlag;
    QList<QOhosQpaFunctions::FileShare::PolicyErrorResult> errorResults;
    std::tie(successFlag, errorResults) =
        QtOhos::getQOhosQpaFunctions().revokePermission(convertToPolicyInfos(policies));
    return QSharedPointer<ActionResultImpl>::create(successFlag, errorResults);
}

/*!
    \fn QSharedPointer<ActionResult> QtOhosAppKit::FileShare::activatePermission(const QList<PathPolicy> &policies)

    Activates file or folder permissions from \a policies.
    The success of the action can be determined from ActionResult.

    This function calls \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/file_share-V5#oh_fileshare_activatepermission} {OH_FileShare_ActivatePermission}.
    Each time an application is started, its persistent permissions have not been loaded to the memory. Use this function to make a persistent permission active after the application is started.
    If the activation fails because the permission has not been persisted, use persistPermission() first.

    To deactivate an active permission, use deactivatePermission().

    \sa ActionResult, PathPolicy, persistPermission(), deactivatePermission()
*/
QSharedPointer<ActionResult> activatePermission(const QList<PathPolicy> &policies)
{
    bool successFlag;
    QList<QOhosQpaFunctions::FileShare::PolicyErrorResult> errorResults;
    std::tie(successFlag, errorResults) =
        QtOhos::getQOhosQpaFunctions().activatePermission(convertToPolicyInfos(policies));
    return QSharedPointer<ActionResultImpl>::create(successFlag, errorResults);
}

/*!
    \fn QSharedPointer<ActionResult> QtOhosAppKit::FileShare::deactivatePermission(const QList<PathPolicy> &policies)

    Deactivates file or folder permissions from \a policies.
    The success of the action can be determined from ActionResult.

    This function calls \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/file_share-V5#oh_fileshare_deactivatepermission} {OH_FileShare_DeactivatePermission}.

    To activate permission, use activatePermission().

    \sa ActionResult, PathPolicy, activatePermission()
*/
QSharedPointer<ActionResult> deactivatePermission(const QList<PathPolicy> &policies)
{
    bool successFlag;
    QList<QOhosQpaFunctions::FileShare::PolicyErrorResult> errorResults;
    std::tie(successFlag, errorResults) =
        QtOhos::getQOhosQpaFunctions().deactivatePermission(convertToPolicyInfos(policies));
    return QSharedPointer<ActionResultImpl>::create(successFlag, errorResults);
}

/*!
    \fn QSharedPointer<CheckResult> QtOhosAppKit::FileShare::checkPersistent(const QList<PathPolicy> &policies)

    Checks if file or folder permissions from \a policies have persistent permissions.
    The result check for each file or folder can be determined from CheckResult.
    CheckResult::PathPolicyCheckResult::result will be set to \c true for each file or folder only for succesufull check and persistent permission found, \c false otherwise.

    This function calls \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/file_share-V5#oh_fileshare_checkpersistentpermission} {OH_FileShare_CheckPersistentPermission}.

    To persist permission, use persistPermission().

    \sa CheckResult, PathPolicy, persistPermission()
*/
QSharedPointer<CheckResult> checkPersistent(const QList<PathPolicy> &policies)
{
    bool successFlag;
    std::vector<bool> checkResults;
    std::tie(successFlag, checkResults) =
        QtOhos::getQOhosQpaFunctions().checkPersistent(convertToPolicyInfos(policies));
    return QSharedPointer<CheckResultImpl>::create(successFlag, checkResults, policies);
}

}

}

QT_END_NAMESPACE
