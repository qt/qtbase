// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <qohosjsenv_p.h>
#include <QtCore/qobject.h>
#include <QtCore/qscopeguard.h>
#include <QtGui/private/qguiapplication_p.h>
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
#include <qohoswindowproperty.h>
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

    QOhosSupplier<double> makeOhosConfigFontSizeScaleDataSource(
        QOhosConsumer<double> valueChangedHandler) override;

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
