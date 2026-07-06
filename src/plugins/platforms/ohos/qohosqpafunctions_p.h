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
#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qvariant.h>
#include <optional>
#include <qohosplugincore.h>

QT_BEGIN_NAMESPACE

class QColor;

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

    virtual QOhosSupplier<double> makeOhosConfigFontSizeScaleDataSource(
        QOhosConsumer<double> valueChangedHandler) = 0;

    virtual bool readOhosNoUiChildMode() = 0;

    virtual bool showFileDialogToAuthorizeFilePath(QObject *parentWindow, const QString &filePath) = 0;

    virtual void setWindowBrightness(QObject *window, int brightness) = 0;
    virtual void setWindowContrast(QObject *window, int contrast) = 0;
    virtual void setWindowSaturation(QObject *window, int saturation) = 0;

    virtual void setAudioStreamUsageHintProperty(QObject *qObject, AudioStreamUsage usage) = 0;
    virtual std::optional<AudioStreamUsage> tryGetAudioStreamUsageHintProperty(QObject *qObject) = 0;

protected:
    QOhosQpaFunctions();
};

}

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::QOhosQpaFunctions::AudioStreamUsage));

#endif // QOHOSQPAFUNCTIONS_H
