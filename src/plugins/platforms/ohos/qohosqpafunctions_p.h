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

#include <QtCore/qglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qvariant.h>
#include <optional>
#include <qohosplugincore.h>

QT_BEGIN_NAMESPACE

namespace QtOhos {

class QOhosQpaFunctions
{
public:
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

    virtual QVariant getImageDataFromPasteboard() const = 0;
    virtual QString getTextDataFromPasteboard() const = 0;

    virtual void setAudioStreamUsageHintProperty(QObject *qObject, AudioStreamUsage usage) = 0;
    virtual std::optional<AudioStreamUsage> tryGetAudioStreamUsageHintProperty(QObject *qObject) = 0;

protected:
    QOhosQpaFunctions();
};

}

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QT_PREPEND_NAMESPACE(QtOhos::QOhosQpaFunctions::AudioStreamUsage));

#endif // QOHOSQPAFUNCTIONS_H
