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
#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qvariant.h>
#include <QtGui/qcolor.h>
#include <QtGui/qimage.h>
#include <QtHarmonyExtras/private/qohosbundlemanager_p.h>
#include <QtHarmonyExtras/private/qtharmonyextrasglobal_p.h>
#include <functional>
#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QtHarmonyExtras {

class Q_HARMONYEXTRAS_EXPORT WindowCreateParams
{
    // Q_GADGET added to Q_ENUM the nested enum
    Q_GADGET

public:
    enum class AnimationType {
        FadeInOut = 0,
    };
    Q_ENUM(AnimationType)

    virtual ~WindowCreateParams();

    virtual void setAnimationType(AnimationType animationType) = 0;

protected:
    WindowCreateParams();

private:
    Q_DISABLE_COPY(WindowCreateParams)
};

class Q_HARMONYEXTRAS_EXPORT StartOptions
{
    // Q_GADGET added to Q_ENUM the nested enums
    Q_GADGET

public:
    enum class ProcessMode {
        NewProcessAttachToParent,
        NewProcessAttachToStatusBarItem,
    };
    Q_ENUM(ProcessMode)

    enum class StartupVisibility {
        Hide,
        Show,
    };
    Q_ENUM(StartupVisibility)

    enum class WindowMode {
        SplitPrimary,
        SplitSecondary,
        Fullscreen,
    };
    Q_ENUM(WindowMode)

    enum class SupportWindowMode {
        FullScreen,
        Split,
        Floating,
    };
    Q_ENUM(SupportWindowMode)

    virtual ~StartOptions();

    virtual void setWindowMode(WindowMode windowMode) = 0;
    virtual void setDisplayId(int displayId) = 0;
    virtual void setWithAnimation(bool withAnimation) = 0;
    virtual void setWindowLeft(int windowLeft) = 0;
    virtual void setWindowTop(int windowTop) = 0;
    virtual void setWindowWidth(int windowWidth) = 0;
    virtual void setWindowHeight(int windowHeight) = 0;
    virtual void setProcessMode(ProcessMode processMode) = 0;
    virtual void setStartupVisibility(StartupVisibility startupVisibility) = 0;
    virtual void setStartWindowIcon(const QImage &startWindowIcon) = 0;
    virtual void setStartWindowBackgroundColor(const QColor &startWindowBackgroundColor) = 0;
    virtual void setSupportWindowModes(const QList<SupportWindowMode> &supportWindowModes) = 0;
    virtual void setMinWindowWidth(int minWindowWidth) = 0;
    virtual void setMinWindowHeight(int minWindowHeight) = 0;
    virtual void setMaxWindowWidth(int maxWindowWidth) = 0;
    virtual void setMaxWindowHeight(int maxWindowHeight) = 0;
    virtual void setHideStartWindow(bool hideStartWindow) = 0;
    virtual void setWindowCreateParams(const WindowCreateParams &windowCreateParams) = 0;
    virtual void setCompletionHandler(
        QObject *context,
        std::function<void(bool, ElementName, QString)> callback) = 0;

protected:
    StartOptions();

private:
    Q_DISABLE_COPY(StartOptions)
};

Q_HARMONYEXTRAS_EXPORT std::shared_ptr<WindowCreateParams> createWindowCreateParams();

Q_HARMONYEXTRAS_EXPORT std::shared_ptr<StartOptions> createStartOptions();

}

namespace QtHarmonyExtras::Private {

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

    struct WindowCreateParamsPriv
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
    std::optional<WindowCreateParamsPriv> windowCreateParams;
};

struct QOhosAbilityResult
{
    int resultCode;
    std::optional<QJsonObject> want;
};

std::optional<QOhosStartOptionsData> tryConvertStartOptionsToQpaFunctionsStruct(
    const StartOptions &options);

}

QT_END_NAMESPACE

#endif
