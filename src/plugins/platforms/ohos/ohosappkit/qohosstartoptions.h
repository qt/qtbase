// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSSTARTOPTIONS_H
#define QOHOSSTARTOPTIONS_H

#include <QtCore/qlist.h>
#include <QtCore/qsharedpointer.h>
#include <QtGui/qcolor.h>
#include <QtGui/qimage.h>
#include <QtOhosAppKit/qtohosappkitglobal.h>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

class Q_OHOSAPPKIT_EXPORT QOhosWindowCreateParams
{
    Q_GADGET

public:
    enum class AnimationType {
        FADE_IN_OUT = 0,
    };
    Q_ENUM(AnimationType)

    virtual ~QOhosWindowCreateParams();

    virtual void setAnimationType(AnimationType animationType) = 0;

protected:
    QOhosWindowCreateParams();

private:
    Q_DISABLE_COPY(QOhosWindowCreateParams)
};

class Q_OHOSAPPKIT_EXPORT QOhosStartOptions
{
    Q_GADGET

public:
    enum class ProcessMode {
        NEW_PROCESS_ATTACH_TO_PARENT,
        NEW_PROCESS_ATTACH_TO_STATUS_BAR_ITEM,
    };
    Q_ENUM(ProcessMode)

    enum class StartupVisibility {
        STARTUP_HIDE,
        STARTUP_SHOW,
    };
    Q_ENUM(StartupVisibility)

    enum class WindowMode {
        WINDOW_MODE_SPLIT_PRIMARY,
        WINDOW_MODE_SPLIT_SECONDARY,
        WINDOW_MODE_FULLSCREEN,
    };
    Q_ENUM(WindowMode)

    enum class SupportWindowMode {
        FULL_SCREEN,
        SPLIT,
        FLOATING,
    };
    Q_ENUM(SupportWindowMode)

    virtual ~QOhosStartOptions();

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
    virtual void setWindowCreateParams(const QOhosWindowCreateParams &windowCreateParams) = 0;

protected:
    QOhosStartOptions();

private:
    Q_DISABLE_COPY(QOhosStartOptions)
};

Q_OHOSAPPKIT_EXPORT QSharedPointer<QOhosWindowCreateParams> createWindowCreateParams();

Q_OHOSAPPKIT_EXPORT QSharedPointer<QOhosStartOptions> createStartOptions();

}

QT_END_NAMESPACE

#endif
