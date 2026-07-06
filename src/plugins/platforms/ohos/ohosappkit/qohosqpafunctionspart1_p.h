// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSQPAFUNCTIONSPART1_P_H
#define QOHOSQPAFUNCTIONSPART1_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qglobal.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qmimedata.h>
#include <QtCore/qobject.h>
#include <QtCore/qpoint.h>
#include <QtCore/qsize.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qvariant.h>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <utility>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QtOhos {

class QOhosQpaFunctionsPart1
{
public:
    enum class AbilityOnContinueResponseStatus
    {
        Agree,
        Reject,
        Mismatch,
    };

    struct AbilityOnContinueRequest
    {
        int sourceApplicationVersionCode;
    };

    struct AbilityOnContinueResponse
    {
        AbilityOnContinueResponseStatus status;
        QMap<QString, QString> wantObjectParams;
        std::optional<bool> exitAppOnSourceDeviceAfterMigration;
    };

    struct FileShare {
        enum class PolicyErrorCode
        {
            PERSISTENCE_FORBIDDEN,
            INVALID_MODE,
            INVALID_PATH,
            PERMISSION_NOT_PERSISTED,
        };

        enum class OperationMode {
            Read = 1 << 0,
            Write = 1 << 1,
        };

        struct PolicyInfo
        {
            QString path;
            std::set<OperationMode> operationModes;
        };

        struct PolicyErrorResult
        {
            QString path;
            std::optional<PolicyErrorCode> error;
            QString errorMessage;
        };
    };

    struct StartOptions
    {
        enum class ProcessMode
        {
            NEW_PROCESS_ATTACH_TO_PARENT,
            NEW_PROCESS_ATTACH_TO_STATUS_BAR_ITEM,
        };

        enum class StartupVisibility
        {
            STARTUP_HIDE,
            STARTUP_SHOW,
        };

        enum class WindowMode
        {
            WINDOW_MODE_SPLIT_PRIMARY,
            WINDOW_MODE_SPLIT_SECONDARY,
            WINDOW_MODE_FULLSCREEN,
        };

        enum class SupportWindowMode
        {
            FULL_SCREEN,
            SPLIT,
            FLOATING,
        };

        struct WindowCreateParams
        {
            bool setWindowFadeInOutAnimation = false;
        };

        std::optional<WindowMode> windowMode;
        std::optional<int> displayId;
        std::optional<bool> withAnimation;
        std::optional<int> windowLeft;
        std::optional<int> windowTop;
        std::optional<int> windowWidth;
        std::optional<int> windowHeight;
        std::optional<ProcessMode> processMode;
        std::optional<StartupVisibility> startupVisibility;
        std::optional<QVariant> windowIcon;
        std::optional<QString> windowBackgroundColorHex;
        std::optional<QList<SupportWindowMode>> supportWindowModes;
        std::optional<int> minWindowWidth;
        std::optional<int> minWindowHeight;
        std::optional<int> maxWindowWidth;
        std::optional<int> maxWindowHeight;
        std::shared_ptr<QOhosConsumer<bool, QJsonObject, QString>> optCompletionHandler;
        std::optional<bool> hideStartWindow;
        std::optional<WindowCreateParams> windowCreateParams;
    };

    struct AppPermissionResult
    {
        bool permissionGranted;
        bool dialogShown;
    };

    struct AbilityResult
    {
        int resultCode;
        std::optional<QJsonObject> want;
    };

    struct ShareKit
    {
        enum class ShareAbilityType
        {
            COPY_TO_PASTEBOARD,
            SAVE_TO_MEDIA_ASSET,
            SAVE_AS_FILE,
            PRINT,
            SAVE_TO_SUPERHUB,
        };

        struct SharedRecord
        {
            QString mimeType;

            std::optional<QString> content;
            std::optional<QString> filePath;

            std::optional<QString> title;
            std::optional<QString> label;
            std::optional<QString> description;
            std::optional<QByteArray> thumbnail;
            std::optional<QString> thumbnailFilePath;
            std::optional<QVariantMap> extraData;
        };

        struct ShareControllerOptions
        {
            std::optional<QPoint> anchorOffset;
            std::optional<QSize> anchorSize;
            std::optional<bool> useSingleSelectionMode;
            std::optional<bool> useDefaultPreviewMode;
            std::optional<QList<ShareAbilityType>> excludedAbilities;
        };

        struct ShareOperationResult
        {
            QString targetAbilityName;
        };
    };

    class WantInfo
    {
    public:
        enum class LaunchReason
        {
            UNKNOWN,
            START_ABILITY,
            CONTINUATION,
            PREPARE_CONTINUATION,
            PRELOAD,
        };

        struct ContactInfo
        {
            QString contactType;
            QString contactId;
        };

        virtual ~WantInfo();

        virtual QJsonObject jsonObject() const = 0;

        virtual std::optional<QList<ShareKit::SharedRecord>> tryGetSharedDataRecords() const = 0;

        virtual std::optional<ContactInfo> tryGetContactInfo() const = 0;

        virtual LaunchReason launchReason() const = 0;

    protected:
        WantInfo();

    private:
        Q_DISABLE_COPY(WantInfo)
    };

    void setOnContinueRequestsHandlerForAbilityInstanceWindow(
        QObject *windowObject, std::function<void(AbilityOnContinueRequest, QOhosConsumer<AbilityOnContinueResponse>)> requestsHandler);

    void setAbilityContinuationActive(
        QObject *optInstanceMainWindow, bool continuationActive);

    Q_NORETURN void restartApp(std::optional<QJsonObject> want);

    QJsonObject getAppLaunchWant();
    QSharedPointer<WantInfo> getAppLaunchWantInfo() const;

    void addNewWantConsumer(QObject *context, QOhosConsumer<QJsonObject> wantConsumer);
    void addNewWantConsumer(
        QObject *context, QOhosConsumer<QSharedPointer<WantInfo>> wantConsumer);

    void startAppProcess(
        const QString &processId, const QJsonObject &requestWant,
        const std::optional<StartOptions> &optStartOptions);

    bool startAbility(const QJsonObject &want, const std::optional<StartOptions> &options);

    bool startAbilityByType(const QString &appType, const QJsonObject &wantParameters);

    void startAbilityForResult(
        const QJsonObject &want, const std::optional<StartOptions> &options,
        QObject *optInstanceMainWindow, QObject *resultConsumerQtContext,
        QOhosConsumer<std::optional<AbilityResult>> resultConsumer);

    void setDestroyAllowedFlagForAbilityInstances(
        std::vector<QObject *> instancesMainWindows, bool destroyEnabled);

    int getCurrentApplicationVersionCode();

    void startNoUiChildProcess(QString libraryName, QStringList args);

    bool hasSerialPortAccessRight(const QString &portName);

    void requestSerialPortAccessRight(
        const QString &portName, QObject *resultConsumerQtContext,
        QOhosConsumer<std::shared_ptr<void>> resultConsumer);

    std::pair<bool, QList<FileShare::PolicyErrorResult>> persistPermission(
        const QList<FileShare::PolicyInfo> &policyInfos);

    std::pair<bool, QList<FileShare::PolicyErrorResult>> revokePermission(
        const QList<FileShare::PolicyInfo> &policyInfos);

    std::pair<bool, QList<FileShare::PolicyErrorResult>> activatePermission(
        const QList<FileShare::PolicyInfo> &policyInfos);

    std::pair<bool, QList<FileShare::PolicyErrorResult>> deactivatePermission(
        const QList<FileShare::PolicyInfo> &policyInfos);

    std::pair<bool, std::vector<bool>> checkPersistent(const QList<FileShare::PolicyInfo> &policyInfos);

    std::shared_ptr<void> shareDataUsingShareKit(
        QObject *optWindowObject, const QList<ShareKit::SharedRecord> &recordsToShare,
        const ShareKit::ShareControllerOptions &controllerOptions,
        std::function<void()> panelClosedCallback,
        QOhosConsumer<ShareKit::ShareOperationResult> optShareCompletedCallback = nullptr);

    bool tryOpenLink(QObject *optInstanceMainWindow, const QString &link, std::optional<bool> appLinkingOnly);
};

QOhosQpaFunctionsPart1 &getQOhosQpaFunctions();

}

QT_END_NAMESPACE

#endif // QOHOSQPAFUNCTIONSPART1_P_H
