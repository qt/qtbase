// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosstartoptions.h"
#include <QtOhosAppKit/private/qohosstartoptions_p.h>
#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

/*!
    \class QtOhosAppKit::StartOptions
    \inmodule QtOhosAppKit
    \since 5.12.12

    \brief The StartOptions class is to provide new options for new started ability or process.

    \sa createStartOptions()
    \sa startAbility()
    \sa startAppProcess()
*/

/*!
    \enum QtOhosAppKit::StartOptions::ProcessMode
    \since 5.12.12

    Enumerates the process modes. See
    \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V13/js-apis-app-ability-contextconstant-V13#processmode12}
    {Process Mode}.

    \value NEW_PROCESS_ATTACH_TO_PARENT A new process is created, the ability is started on the
    process, and the process exits along with the parent process.
    \value NEW_PROCESS_ATTACH_TO_STATUS_BAR_ITEM A new process is created, the ability is started
    on the process, and the process is bound to the status bar icon.
*/

/*!
    \enum QtOhosAppKit::StartOptions::StartupVisibility
    \since 5.12.12

    Enumerates the visibility statuses of an ability after it is started. See
    \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V13/js-apis-app-ability-contextconstant-V13#startupvisibility12}
    {Startup Visibility}.

    \value STARTUP_HIDE The target ability is hidden after it is started in the new process.
    \value STARTUP_SHOW The target ability is displayed normally after it is started in the new process.
*/

/*!
    \enum QtOhosAppKit::StartOptions::WindowMode
    \since 5.12.12

    Enumerates the window mode when the ability is started. See
    \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V13/js-apis-app-ability-abilityconstant-V13#windowmode12}
    {Window Mode}.

    \value WINDOW_MODE_SPLIT_PRIMARY Primary screen (left screen in the case of horizontal
    orientation) in split-screen mode.
    \value WINDOW_MODE_SPLIT_SECONDARY Secondary screen (right screen in the case of horizontal
    orientation) in split-screen mode.
    \value WINDOW_MODE_FULLSCREEN Full screen mode.
*/

/*!
    \enum QtOhosAppKit::StartOptions::SupportWindowMode
    \since 5.12.12

    Enumerates the supported window modes when the ability is started. See
    \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-bundlemanager#supportwindowmode}
    {Support Window Mode}.

    \value FULL_SCREEN Indicates support for full screen mode.
    \value SPLIT Indicates support for split mode.
    \value FLOATING Indicates support for floating mode.
*/

/*!
    \class QtOhosAppKit::WindowCreateParams
    \inmodule QtOhosAppKit
    \since 5.12.12

    \brief The WindowCreateParams class provides window creation parameters
    for a started ability window.

    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/arkts-apis-window-i#windowcreateparams20}
    {Window Create Params}.

    \sa createWindowCreateParams()
*/

/*!
    \enum QtOhosAppKit::WindowCreateParams::AnimationType
    \since 5.12.12

    Enumerates the animation types for window creation. See
    \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/arkts-apis-window-e#animationtype20}
    {Animation Type}.

    \value FADE_IN_OUT Fade-in/fade-out animation when the window is created and
    destroyed.
*/

namespace {

ElementName convertElementNameFromJsonObject(const QJsonObject &object)
{
    return ElementName{
        .deviceId = object.value(QLatin1String("deviceId")).toString(),
        .bundleName = object.value(QLatin1String("bundleName")).toString(),
        .abilityName = object.value(QLatin1String("abilityName")).toString(),
        .uri = object.value(QLatin1String("uri")).toString(),
        .shortName = object.value(QLatin1String("shortName")).toString(),
        .moduleName = object.value(QLatin1String("moduleName")).toString(),
    };
}

std::optional<QOhosStartOptionsData::SupportWindowMode> tryMapSupportWindowModeToQpaFunctions(
    StartOptions::SupportWindowMode supportWindowMode)
{
    switch (supportWindowMode) {
    case StartOptions::SupportWindowMode::FULL_SCREEN:
        return std::make_optional(QOhosStartOptionsData::SupportWindowMode::FULL_SCREEN);
    case StartOptions::SupportWindowMode::SPLIT:
        return std::make_optional(QOhosStartOptionsData::SupportWindowMode::SPLIT);
    case StartOptions::SupportWindowMode::FLOATING:
        return std::make_optional(QOhosStartOptionsData::SupportWindowMode::FLOATING);
    }
    return {};
}

QList<QOhosStartOptionsData::SupportWindowMode> mapSupportWindowModesToQpaFunctions(
    const QList<StartOptions::SupportWindowMode> &supportWindowModes)
{
    QList<QOhosStartOptionsData::SupportWindowMode> qpaFuncsSupportWindowModes;
    for (auto supportWindowMode : supportWindowModes) {
        auto optQpaFuncsSupportWindowMode = tryMapSupportWindowModeToQpaFunctions(supportWindowMode);
        if (optQpaFuncsSupportWindowMode.has_value()) {
            qpaFuncsSupportWindowModes.append(optQpaFuncsSupportWindowMode.value());
        } else {
            qCWarning(
                QtForOhos, "%s: got unsupported supportWindowMode (%d), ignoring",
                Q_FUNC_INFO, static_cast<int>(supportWindowMode));
        }
    }

    return qpaFuncsSupportWindowModes;
}

class QOhosWindowCreateParamsImpl : public WindowCreateParams
{
public:
    QOhosWindowCreateParamsImpl();

    void setAnimationType(AnimationType animationType) override;

    QOhosStartOptionsData::WindowCreateParamsPriv qpaWindowCreateParams() const;

private:
    QOhosStartOptionsData::WindowCreateParamsPriv m_qpaWindowCreateParams;
};

QOhosWindowCreateParamsImpl::QOhosWindowCreateParamsImpl() = default;

void QOhosWindowCreateParamsImpl::setAnimationType(AnimationType animationType)
{
    bool supportedAnimationType = false;
    switch (animationType) {
    case AnimationType::FADE_IN_OUT:
        m_qpaWindowCreateParams.setWindowFadeInOutAnimation = true;
        supportedAnimationType = true;
        break;
    }

    if (!supportedAnimationType) {
        qCWarning(
            QtForOhos, "%s: got unsupported AnimationType: %d",
            Q_FUNC_INFO, static_cast<int>(animationType));
    }
}

QOhosStartOptionsData::WindowCreateParamsPriv QOhosWindowCreateParamsImpl::qpaWindowCreateParams() const
{
    return m_qpaWindowCreateParams;
}

class QOhosStartOptionsImpl : public StartOptions
{
public:
    QOhosStartOptionsImpl()
        : StartOptions()
    {}

    /*!
        \fn QtOhosAppKit::StartOptions::setWindowMode(WindowMode windowMode)

        Sets \a windowMode when the ability is started. See
        \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-app-ability-startoptions-V5#properties}
        {Window Mode}.
    */
    void setWindowMode(StartOptions::WindowMode windowMode) override
    {
        std::optional<QOhosStartOptionsData::WindowMode> internalWindowMode;
        switch (windowMode) {
        case StartOptions::WindowMode::WINDOW_MODE_SPLIT_PRIMARY:
            internalWindowMode = QOhosStartOptionsData::WindowMode::WINDOW_MODE_SPLIT_PRIMARY;
            break;
        case StartOptions::WindowMode::WINDOW_MODE_SPLIT_SECONDARY:
            internalWindowMode = QOhosStartOptionsData::WindowMode::WINDOW_MODE_SPLIT_SECONDARY;
            break;
        case StartOptions::WindowMode::WINDOW_MODE_FULLSCREEN:
            internalWindowMode = QOhosStartOptionsData::WindowMode::WINDOW_MODE_FULLSCREEN;
            break;
        }

        if (internalWindowMode.has_value())
            m_startOptions.windowMode = internalWindowMode.value();
        else
            qCWarning(QtForOhos, "%s: unsupported windowMode: %d", Q_FUNC_INFO, static_cast<int>(windowMode));
    }

    /*!
        \fn QtOhosAppKit::StartOptions::setDisplayId(int displayId)

        Sets \a displayId. The default value is 0, indicating the current display. See
        \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-app-ability-startoptions-V5#properties}
        {Display Id}.
    */
    void setDisplayId(int displayId) override
    {
        m_startOptions.displayId = displayId;
    }

    /*!
        \fn QtOhosAppKit::StartOptions::setWithAnimation(bool withAnimation)

        Sets \a withAnimation whether the ability has the animation effect. See
        \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-app-ability-startoptions-V5}
        {With Animation}.
    */
    void setWithAnimation(bool withAnimation) override
    {
        m_startOptions.withAnimation = withAnimation;
    }

    /*!
        \fn QtOhosAppKit::StartOptions::setWindowLeft(int windowLeft)

        Sets \a windowLeft left position of the window. See
        \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-app-ability-startoptions-V5}
        {Window Left}.
    */
    void setWindowLeft(int windowLeft) override
    {
        m_startOptions.windowLeft = windowLeft;
    }

    /*!
        \fn QtOhosAppKit::StartOptions::setWindowTop(int windowTop)

        Sets \a windowTop top position of the window. See
        \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-app-ability-startoptions-V5}
        {Window Top}.
    */
    void setWindowTop(int windowTop) override
    {
        m_startOptions.windowTop = windowTop;
    }

    /*!
        \fn QtOhosAppKit::StartOptions::setWindowWidth(int windowWidth)

        Sets \a windowWidth width of of the window. See
        \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-app-ability-startoptions-V5}
        {Window Width}.
    */
    void setWindowWidth(int windowWidth) override
    {
        m_startOptions.windowWidth = windowWidth;
    }

    /*!
        \fn QtOhosAppKit::StartOptions::setWindowHeight(int windowHeight)

        Sets \a windowHeight height of of the window. See
        \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-app-ability-startoptions-V5}
        {Window Height}.
    */
    void setWindowHeight(int windowHeight) override
    {
        m_startOptions.windowHeight = windowHeight;
    }

    /*!
        \fn QtOhosAppKit::StartOptions::setProcessMode(ProcessMode processMode)

        Sets \a processMode. See
        \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-app-ability-startoptions-V5}
        {Process Mode}.
    */
    void setProcessMode(StartOptions::ProcessMode processMode) override
    {
        std::optional<QOhosStartOptionsData::ProcessMode> internalProcessMode;
        switch (processMode) {
        case StartOptions::ProcessMode::NEW_PROCESS_ATTACH_TO_PARENT:
            internalProcessMode = QOhosStartOptionsData::ProcessMode::NEW_PROCESS_ATTACH_TO_PARENT;
            break;
        case StartOptions::ProcessMode::NEW_PROCESS_ATTACH_TO_STATUS_BAR_ITEM:
            internalProcessMode = QOhosStartOptionsData::ProcessMode::NEW_PROCESS_ATTACH_TO_STATUS_BAR_ITEM;
            break;
        }

        if (internalProcessMode.has_value())
            m_startOptions.processMode = internalProcessMode.value();
        else
            qCWarning(QtForOhos, "%s: unsupported processMode: %d", Q_FUNC_INFO, static_cast<int>(processMode));
    }

    /*!
        \fn QtOhosAppKit::StartOptions::setStartupVisibility(StartupVisibility startupVisibility)

        Sets \a startupVisibility of the ability after it is started. See
        \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-app-ability-startoptions-V5}
        {Startup Visibility}.
    */
    void setStartupVisibility(StartOptions::StartupVisibility startupVisibility) override
    {
        std::optional<QOhosStartOptionsData::StartupVisibility> internalStartupVisibility;
        switch (startupVisibility) {
        case StartOptions::StartupVisibility::STARTUP_HIDE:
            internalStartupVisibility = QOhosStartOptionsData::StartupVisibility::STARTUP_HIDE;
            break;
        case StartOptions::StartupVisibility::STARTUP_SHOW:
            internalStartupVisibility = QOhosStartOptionsData::StartupVisibility::STARTUP_SHOW;
            break;
        }

        if (internalStartupVisibility.has_value())
            m_startOptions.startupVisibility = internalStartupVisibility.value();
        else
            qCWarning(QtForOhos, "%s: unsupported startupVisibility: %d", Q_FUNC_INFO, static_cast<int>(startupVisibility));
    }

    /*!
        \fn QtOhosAppKit::StartOptions::setStartWindowIcon(const QImage &startWindowIcon)

        Sets \a startWindowIcon for the start window.

        See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-app-ability-startoptions}
        {Start Window Icon}.
    */
    void setStartWindowIcon(const QImage &startWindowIcon) override
    {
        m_startOptions.windowIcon = startWindowIcon;
    }

    /*!
        \fn QtOhosAppKit::StartOptions::setStartWindowBackgroundColor(const QColor &startWindowBackgroundColor)

        Sets \a startWindowBackgroundColor for the start window.

        See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-app-ability-startoptions}
        {Start Window Background Color}.
    */
    void setStartWindowBackgroundColor(const QColor &startWindowBackgroundColor) override
    {
        if (startWindowBackgroundColor.isValid())
            m_startOptions.windowBackgroundColorHex = startWindowBackgroundColor.name(QColor::HexArgb);
    }

    /*!
        \fn QtOhosAppKit::StartOptions::setSupportWindowModes(const QList<SupportWindowMode> &supportWindowModes)

        Sets \a supportWindowModes when the ability is started. See
        \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-app-ability-startoptions}
        {Support Window Modes}.
    */
    void setSupportWindowModes(const QList<SupportWindowMode> &supportWindowModes) override
    {
        auto qpaFuncsSupportWindowModes = mapSupportWindowModesToQpaFunctions(supportWindowModes);
        if (!qpaFuncsSupportWindowModes.isEmpty())
            m_startOptions.supportWindowModes = qpaFuncsSupportWindowModes;
        else
            qCWarning(QtForOhos, "%s: empty supportWindowModes is unsupported, skipping", Q_FUNC_INFO);
    }

    /*!
        \fn QtOhosAppKit::StartOptions::setMinWindowWidth(int minWindowWidth)

        Sets \a minWindowWidth as the minimum width, in px.

        See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-app-ability-startoptions}
        {Minimum Window Width}.
    */
    void setMinWindowWidth(int minWindowWidth) override
    {
        m_startOptions.minWindowWidth = minWindowWidth;
    }

    /*!
        \fn QtOhosAppKit::StartOptions::setMinWindowHeight(int minWindowHeight)

        Sets \a minWindowHeight as the minimum height, in px.

        See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-app-ability-startoptions}
        {Minimum Window Height}.
    */
    void setMinWindowHeight(int minWindowHeight) override
    {
        m_startOptions.minWindowHeight = minWindowHeight;
    }

    /*!
        \fn QtOhosAppKit::StartOptions::setMaxWindowWidth(int maxWindowWidth)

        Sets \a maxWindowWidth as the maximum width, in px.

        See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-app-ability-startoptions}
        {Maximum Window Width}.
    */
    void setMaxWindowWidth(int maxWindowWidth) override
    {
        m_startOptions.maxWindowWidth = maxWindowWidth;
    }

    /*!
        \fn QtOhosAppKit::StartOptions::setMaxWindowHeight(int maxWindowHeight)

        Sets \a maxWindowHeight as the maximum height, in px.

        See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-app-ability-startoptions}
        {Maximum Window Height}.
    */
    void setMaxWindowHeight(int maxWindowHeight) override
    {
        m_startOptions.maxWindowHeight = maxWindowHeight;
    }

    /*!
        \fn QtOhosAppKit::StartOptions::setHideStartWindow(bool hideStartWindow)

        Controls whether to hide the start window when launching the current application's UIAbility.

        See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-app-ability-startoptions}
        {Hide Start Window}.
    */
    void setHideStartWindow(bool hideStartWindow) override
    {
        m_startOptions.hideStartWindow = hideStartWindow;
    }

    /*!
        \fn QtOhosAppKit::StartOptions::setWindowCreateParams(const WindowCreateParams &windowCreateParams)

        Sets \a windowCreateParams used when creating the window.

        See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/arkts-apis-window-i#windowcreateparams20}
        {Window Create Params}.
    */
    void setWindowCreateParams(const WindowCreateParams &windowCreateParams) override
    {
        const auto *windowCreateParamsImpl = dynamic_cast<const QOhosWindowCreateParamsImpl *>(&windowCreateParams);
        if (windowCreateParamsImpl != nullptr)
            m_startOptions.windowCreateParams = windowCreateParamsImpl->qpaWindowCreateParams();
    }

    /*!
        \fn QtOhosAppKit::StartOptions::setCompletionHandler(QObject *context, std::function<void(bool, ElementName, QString)> callback)

        Sets the completion \a callback invoked when the corresponding start request completes. It is
        invoked on the thread of \a context; if \a context is destroyed before completion, \a callback
        is not invoked. Its arguments report whether the ability was started, the launched element
        name, and a message with the outcome details.

        See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-app-ability-startoptions}
        {Completion Handler}.
    */
    void setCompletionHandler(
        QObject *context,
        std::function<void(bool, ElementName, QString)> callback) override
    {
        m_startOptions.optCompletionHandler =
            std::make_shared<QOhosConsumer<bool, QJsonObject, QString>>(
                [contextPtr = QPointer<QObject>(context), callback = std::move(callback)](
                    bool succeeded, const QJsonObject &elementName, const QString &message) {
                    if (!contextPtr.isNull() && callback)
                        callback(succeeded, convertElementNameFromJsonObject(elementName), message);
                });
    }

    QOhosStartOptionsData getStartOptions() const
    {
        return m_startOptions;
    }

private:
    QOhosStartOptionsData m_startOptions;
};

}

WindowCreateParams::WindowCreateParams() = default;

WindowCreateParams::~WindowCreateParams() = default;

StartOptions::StartOptions() = default;
StartOptions::~StartOptions() = default;

/*!
    \fn QSharedPointer<QtOhosAppKit::WindowCreateParams> QtOhosAppKit::createWindowCreateParams()

    Creates WindowCreateParams instance.
*/
QSharedPointer<WindowCreateParams> createWindowCreateParams()
{
    return QSharedPointer<QOhosWindowCreateParamsImpl>::create();
}

/*!
    \fn QSharedPointer<QtOhosAppKit::StartOptions> QtOhosAppKit::createStartOptions()

    Creates StartOptions instance.
*/
QSharedPointer<StartOptions> createStartOptions()
{
    return QSharedPointer<QOhosStartOptionsImpl>::create();
}

std::optional<QOhosStartOptionsData> tryConvertStartOptionsToQpaFunctionsStruct(
    const StartOptions &options)
{
    const auto *startOptionsImpl = dynamic_cast<const QOhosStartOptionsImpl *>(&options);
    return startOptionsImpl != nullptr
        ? std::make_optional(startOptionsImpl->getStartOptions())
        : std::nullopt;
}

}


QT_END_NAMESPACE
