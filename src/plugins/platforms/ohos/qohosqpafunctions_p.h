// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSQPAFUNCTIONS_H
#define QOHOSQPAFUNCTIONS_H

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
#include <qohosplugincore.h>
#include <set>
#include <utility>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QtOhos {

class QOhosQpaFunctions
{
public:
    enum class NativeNodeRenderFitPolicy
    {
        TopLeft,
        Fill,
    };

    enum class WindowGeometryPersistencePolicy
    {
        Disabled,
        Enabled,
        FollowSystemSetting,
    };

    enum class AbilityOnContinueResponseStatus
    {
        Agree,
        Reject,
        Mismatch,
    };

    enum class AudioStreamUsage {
        Unknown,
        Music,
        VoiceCommunication,
        VoiceAssistant,
        Alarm,
        VoiceMessage,
        Ringtone,
        Notification,
        Accessibility,
        Movie,
        Game,
        Audiobook,
        Navigation,
        VideoCommunication,
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

    virtual ~QOhosQpaFunctions();

    virtual void setWindowPrivacyMode(QObject *window, bool privacyModeEnabled) = 0;
    virtual void setWindowCornerRadius(QObject *windowOrWidget, double radius) = 0;
    virtual void tagWindowOrWidgetAsFloatWindow(QObject *windowOrWidget, bool floatWindow) = 0;

    virtual void setInAppOnlyPasteboardShareOption(bool shareInAppOnly) = 0;
    virtual QVariant getImageDataFromPasteboard() const = 0;
    virtual QString getTextDataFromPasteboard() const = 0;

    virtual void setWindowOrWidgetNativeNodeRenderFitPolicyHint(QObject *windowOrWidget, NativeNodeRenderFitPolicy renderFitPolicy) = 0;

    virtual void setSurfaceBackgroundColor(QObject *windowOrWidget, const QColor &color) = 0;

    virtual void setMainWindowGeometryPersistencePolicy(WindowGeometryPersistencePolicy policy) = 0;

    virtual void setWindowKeepScreenOn(QObject *windowOrWidget, bool keepScreenOn) = 0;

    virtual void setWindowDragResizable(QObject *windowOrWidget, bool dragResizable) = 0;

    virtual std::optional<double> tryGetNativeWindowId(QObject *window) = 0;
    virtual std::optional<double> tryGetScreenDisplayId(QObject *screenObject) = 0;

    virtual void setOnContinueRequestsHandlerForAbilityInstanceWindow(
        QObject *windowObject, std::function<void(AbilityOnContinueRequest, QOhosConsumer<AbilityOnContinueResponse>)> requestsHandler) = 0;

    virtual void setAbilityContinuationActive(
        QObject *optInstanceMainWindow, bool continuationActive) = 0;

    Q_NORETURN virtual void restartApp(std::optional<QJsonObject> want) = 0;

    virtual QJsonObject getAppLaunchWant() = 0;
    virtual QSharedPointer<WantInfo> getAppLaunchWantInfo() const = 0;

    virtual void addNewWantConsumer(QObject *context, QOhosConsumer<QJsonObject> wantConsumer) = 0;
    virtual void addNewWantConsumer(
        QObject *context, QOhosConsumer<QSharedPointer<WantInfo>> wantConsumer) = 0;

    virtual void startAppProcess(
        const QString &processId, const QJsonObject &requestWant,
        const std::optional<StartOptions> &optStartOptions) = 0;

    virtual bool startAbility(const QJsonObject &want, const std::optional<StartOptions> &options) = 0;

    virtual bool startAbilityByType(const QString &appType, const QJsonObject &wantParameters) = 0;

    virtual void startAbilityForResult(
        const QJsonObject &want, const std::optional<StartOptions> &options,
        QObject *optInstanceMainWindow, QObject *resultConsumerQtContext,
        QOhosConsumer<std::optional<AbilityResult>> resultConsumer) = 0;

    virtual void setDestroyAllowedFlagForAbilityInstances(
        std::vector<QObject *> instancesMainWindows, bool destroyEnabled) = 0;

    virtual QOhosSupplier<double> makeOhosConfigFontSizeScaleDataSource(
        QOhosConsumer<double> valueChangedHandler) = 0;

    virtual int getCurrentApplicationVersionCode() = 0;

    virtual bool readOhosNoUiChildMode() = 0;

    virtual void startNoUiChildProcess(QString libraryName, QStringList args) = 0;

    virtual bool hasSerialPortAccessRight(const QString &portName) = 0;

    virtual void requestSerialPortAccessRight(
        const QString &portName, QObject *resultConsumerQtContext,
        QOhosConsumer<std::shared_ptr<void>> resultConsumer) = 0;

    virtual std::pair<bool, QList<FileShare::PolicyErrorResult>> persistPermission(
        const QList<FileShare::PolicyInfo> &policyInfos) = 0;

    virtual std::pair<bool, QList<FileShare::PolicyErrorResult>> revokePermission(
        const QList<FileShare::PolicyInfo> &policyInfos) = 0;

    virtual std::pair<bool, QList<FileShare::PolicyErrorResult>> activatePermission(
        const QList<FileShare::PolicyInfo> &policyInfos) = 0;

    virtual std::pair<bool, QList<FileShare::PolicyErrorResult>> deactivatePermission(
        const QList<FileShare::PolicyInfo> &policyInfos) = 0;

    virtual std::pair<bool, std::vector<bool>> checkPersistent(const QList<FileShare::PolicyInfo> &policyInfos) = 0;

    virtual bool showFileDialogToAuthorizeFilePath(QObject *parentWindow, const QString &filePath) = 0;

    virtual void setWindowBrightness(QObject *window, int brightness) = 0;
    virtual void setWindowContrast(QObject *window, int contrast) = 0;
    virtual void setWindowSaturation(QObject *window, int saturation) = 0;

    virtual std::shared_ptr<void> shareDataUsingShareKit(
        QObject *optWindowObject, const QList<ShareKit::SharedRecord> &recordsToShare,
        const ShareKit::ShareControllerOptions &controllerOptions,
        std::function<void()> panelClosedCallback,
        QOhosConsumer<ShareKit::ShareOperationResult> optShareCompletedCallback = nullptr) = 0;

    virtual bool tryOpenLink(QObject *optInstanceMainWindow, const QString &link, std::optional<bool> appLinkingOnly) = 0;

    virtual void setAudioStreamUsageHintProperty(QObject *qObject, AudioStreamUsage usage) = 0;
    virtual std::optional<AudioStreamUsage> tryGetAudioStreamUsageHintProperty(QObject *qObject) = 0;

protected:
    QOhosQpaFunctions();
};

QOhosQpaFunctions &getQOhosQpaFunctions();

}

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::QOhosQpaFunctions::AudioStreamUsage));

#endif // QOHOSQPAFUNCTIONS_H
