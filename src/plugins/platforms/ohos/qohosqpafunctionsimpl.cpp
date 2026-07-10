// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <qohosjsenv_p.h>
#include <QtCore/qobject.h>
#include <QtCore/qscopeguard.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qscreen.h>
#include <QtGui/qwindow.h>
#include <functional>
#include <info/application_target_sdk_version.h>
#include <memory>
#include <qohosapppermissions_p.h>
#include <qohosenums.h>
#include <qohosjsutils.h>
#include <qohosplatformclipboard.h>
#include <qohosplatformintegration.h>
#include <qohosplatformservices.h>
#include <qohosplatformwindow.h>
#include <qohosplugincore.h>
#include <qohosqpafunctions_p.h>
#include <qohossettings.h>
#include <qohosutils.h>
#include <qohoswindowmanager.h>
#include <qohoswindowproperty.h>
#include <render/qwindowproxyregistry.h>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QtOhos {

namespace {

const QOhosPropertyDescriptor<QOhosQpaFunctions::AudioStreamUsage> audioStreamUsageProperty{"_q_ohos_audioStreamUsage"};

class QOhosQpaFunctionsImpl : public QOhosQpaFunctions
{
public:
    void setInAppOnlyPasteboardShareOption(bool shareInAppOnly) override;
    QVariant getImageDataFromPasteboard() const override;
    QString getTextDataFromPasteboard() const override;

    void setMainWindowGeometryPersistencePolicy(WindowGeometryPersistencePolicy policy) override;

    std::optional<double> tryGetNativeWindowId(QObject *window) override;
    std::optional<double> tryGetScreenDisplayId(QObject *screenObject) override;

    QOhosSupplier<double> makeOhosConfigFontSizeScaleDataSource(
        QOhosConsumer<double> valueChangedHandler) override;

    bool readOhosNoUiChildMode() override;

    bool showFileDialogToAuthorizeFilePath(QObject *parentWindow, const QString &filePath) override;

    void setAudioStreamUsageHintProperty(QObject *qObject, AudioStreamUsage usage) override;
    std::optional<AudioStreamUsage> tryGetAudioStreamUsageHintProperty(QObject *qObject) override;
};

void QOhosQpaFunctionsImpl::setInAppOnlyPasteboardShareOption(bool shareInAppOnly)
{
    QOhosPlatformClipboard::setInAppOnlyPasteboardShareOption(shareInAppOnly);
}

QVariant QOhosQpaFunctionsImpl::getImageDataFromPasteboard() const
{
    return QOhosPlatformIntegration::instance()->clipboard()->getPasteboardDataWithLazyFetchOrLocalIfOwner()->imageData();
}

QString QOhosQpaFunctionsImpl::getTextDataFromPasteboard() const
{
    return QOhosPlatformIntegration::instance()->clipboard()->getPasteboardDataWithLazyFetchOrLocalIfOwner()->text();
}

void QOhosQpaFunctionsImpl::setMainWindowGeometryPersistencePolicy(
    WindowGeometryPersistencePolicy geometryPolicyHint)
{
    std::optional<QOhosPlatformIntegration::WindowGeometryPersistencePolicy> policy;
    switch (geometryPolicyHint) {
    case QOhosQpaFunctions::WindowGeometryPersistencePolicy::Disabled:
        policy = QOhosPlatformIntegration::WindowGeometryPersistencePolicy::Disabled;
        break;
    case QOhosQpaFunctions::WindowGeometryPersistencePolicy::Enabled:
        policy = QOhosPlatformIntegration::WindowGeometryPersistencePolicy::Enabled;
        break;
    case QOhosQpaFunctions::WindowGeometryPersistencePolicy::FollowSystemSetting:
        policy = QOhosPlatformIntegration::WindowGeometryPersistencePolicy::FollowSystemSetting;
        break;
    }

    if (policy.has_value()) {
        QOhosPlatformIntegration::setMainWindowGeometryPersistencePolicy(policy.value());
    } else {
        qOhosReportFatalErrorAndAbort(
            "%s: Failed to convert persistence geometry policy hint to QOhosPlatformIntegration enum",
            Q_FUNC_INFO);
    }
}

std::optional<double> QOhosQpaFunctionsImpl::tryGetNativeWindowId(QObject *window)
{
    auto *qWindow = qobject_cast<QWindow *>(window);
    if (qWindow == nullptr)
        return {};

    auto *platformWindow = QOhosPlatformWindow::fromQWindowOrNull(qWindow);
    if (platformWindow == nullptr)
        return {};

    auto internalId = platformWindow->internalWindowId();
    auto jsWinId = QWindowProxyRegistry::instance().tryMapInternalWindowIdToJsWindowId(internalId);
    if (!jsWinId.has_value())
        return {};

    qOhosPrintfInfo(
        "PlatformWindow WIID: %s is returning JsWindowId: %f to the user",
        qPrintable(internalId.toString()), jsWinId.value().value());

    return jsWinId.value().value();
}

std::optional<double> QOhosQpaFunctionsImpl::tryGetScreenDisplayId(QObject *screenObject)
{
    auto *qScreen = qobject_cast<QScreen *>(screenObject);
    if (qScreen == nullptr) {
        qOhosPrintfWarning("%s: screenObject argument is not a QScreen", Q_FUNC_INFO);
        return {};
    }
    auto *ohosPlatformScreen = static_cast<QOhosPlatformScreen *>(qScreen->handle());

    return ohosPlatformScreen != nullptr
        ? std::optional(ohosPlatformScreen->displayInfo().id.value())
        : std::nullopt;
}

QOhosSupplier<double> QOhosQpaFunctionsImpl::makeOhosConfigFontSizeScaleDataSource(
    QOhosConsumer<double> valueChangedHandler)
{
    auto initFontSizeScale = QOhosPlatformIntegration::instance()->settings()->fontSizeScale();
    return makeOhosConfigValueDataSource<double>(
        [initFontSizeScale](QtOhos::JsState &) {
            return initFontSizeScale;
        },
        [](QtOhos::JsState &, const QNapi::Object &config) {
            return config.get<QNapi::Number>("fontSizeScale").DoubleValue();
        },
        std::move(valueChangedHandler));
}

bool QOhosQpaFunctionsImpl::readOhosNoUiChildMode()
{
    return QtOhos::evalInJsThread(
        [&](auto &jsState) {
            return jsState.defaultQAbilityPeer()->instanceId().empty();
        },
        Q_FUNC_INFO);
}

bool QOhosQpaFunctionsImpl::showFileDialogToAuthorizeFilePath(QObject *parentWindow, const QString &filePath)
{
    auto *qWindow = qobject_cast<QWindow *>(parentWindow);
    if (qWindow == nullptr)
        qOhosReportFatalErrorAndAbort("%s: window argument is null or not a QWindow", Q_FUNC_INFO);

    auto *platformWindow = QOhosPlatformWindow::fromQWindowOrNull(qWindow);
    if (platformWindow == nullptr)
        qOhosReportFatalErrorAndAbort("%s: failed to get platform window", Q_FUNC_INFO);

    auto eventLoop = std::make_shared<QEventLoop>();
    auto filePathAuthorized = std::make_shared<bool>(false);

    QOhosWindowManager::showFileDialogAuthorization(
       platformWindow->internalWindowId(), filePath,
       [filePathAuthorized, eventLoop](bool result) {
            *filePathAuthorized = result;
            eventLoop->quit();
       });

    eventLoop->exec();

    return *filePathAuthorized;
}

void QOhosQpaFunctionsImpl::setAudioStreamUsageHintProperty(QObject *qObject, AudioStreamUsage usage)
{
    setQOhosPropertyOnQObject<QOhosQpaFunctions::AudioStreamUsage, &audioStreamUsageProperty>(qObject, usage);
}

std::optional<QOhosQpaFunctions::AudioStreamUsage> QOhosQpaFunctionsImpl::tryGetAudioStreamUsageHintProperty(QObject *qObject)
{
    return tryGetQOhosPropertyFromQObject<QOhosQpaFunctions::AudioStreamUsage, &audioStreamUsageProperty>(qObject);
}

}

QOhosQpaFunctions::QOhosQpaFunctions() = default;

QOhosQpaFunctions::~QOhosQpaFunctions() = default;

}

QT_END_NAMESPACE
