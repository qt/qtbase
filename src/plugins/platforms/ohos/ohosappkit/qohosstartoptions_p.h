// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSSTARTOPTIONS_P_H
#define QOHOSSTARTOPTIONS_P_H

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
#include <QtCore/qjsonobject.h>
#include <QtCore/qlist.h>
#include <QtCore/qstring.h>
#include <QtCore/qvariant.h>
#include <QtOhosAppKit/qohosstartoptions.h>
#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

struct QOhosStartOptionsData
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

struct QOhosAbilityResult
{
    int resultCode;
    std::optional<QJsonObject> want;
};

std::optional<QOhosStartOptionsData> tryConvertStartOptionsToQpaFunctionsStruct(
    const QOhosStartOptions &options);

}

QT_END_NAMESPACE

#endif
