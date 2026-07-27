// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSABILITYCONTEXT_P_H
#define QOHOSABILITYCONTEXT_P_H

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

#include <QtHarmonyExtras/private/qohosoperationstatus_p.h>
#include <QtHarmonyExtras/private/qohossharekit_p.h>
#include <QtHarmonyExtras/private/qohosstartoptions_p.h>
#include <QtHarmonyExtras/private/qohoswant_p.h>
#include <QtHarmonyExtras/private/qtharmonyextrasglobal_p.h>

#include <QtGui/qwindow.h>
#include <QtWidgets/qwidget.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qlist.h>
#include <QtCore/qstring.h>

#include <functional>
#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QtHarmonyExtras {

class Q_HARMONYEXTRAS_EXPORT OpenLinkOptions
{
public:
    virtual ~OpenLinkOptions();

    virtual void setAppLinkingOnly(bool appLinkingOnly) = 0;

protected:
    OpenLinkOptions();

private:
    Q_DISABLE_COPY(OpenLinkOptions)
};

Q_HARMONYEXTRAS_EXPORT std::shared_ptr<OpenLinkOptions> createOpenLinkOptions();

class Q_HARMONYEXTRAS_EXPORT OnContinueContext
{
public:
    virtual ~OnContinueContext();

    virtual void sendAgreeResponse(const QByteArray &responseData) = 0;
    virtual void sendRejectResponse() = 0;
    virtual void sendMismatchResponse() = 0;

    virtual void setExitAppOnSourceDeviceAfterMigration(bool exitAfterMigration) = 0;

    virtual int sourceApplicationVersionCode() const = 0;

protected:
    OnContinueContext();

private:
    Q_DISABLE_COPY(OnContinueContext)
};

struct StartAbilityResult
{
    int resultCode = 0;
    std::optional<Want> want;
};

class Q_HARMONYEXTRAS_EXPORT AbilityContext : public QObject
{
    Q_OBJECT

public:
    static AbilityContext *defaultInstance();
    static AbilityContext *instanceForMainWindow(QWindow *instanceMainWindow);

    virtual void setDestroyFromSystemEnabled(bool destroyEnabled) = 0;

    virtual void startAbilityForResult(
        const Want &want, std::shared_ptr<StartOptions> options, QObject *context,
        std::function<void(std::optional<StartAbilityResult>)> callback) = 0;

    virtual void shareDataWithShareKit(
        const QList<std::shared_ptr<ShareKit::SharedRecord>> &records,
        std::shared_ptr<ShareKit::ShareControllerOptions> controllerOptions,
        QObject *context,
        std::function<void(std::shared_ptr<ShareKit::ShareOperationResult>)> onShareCompleted,
        std::function<void()> onPanelClosed) = 0;

    virtual bool tryOpenLink(const QString &link, std::shared_ptr<OpenLinkOptions> options) = 0;

    virtual void setContinuationActive(bool continuationActive) = 0;

Q_SIGNALS:
    void newWantInfoReceived(std::shared_ptr<WantInfo> wantInfo);
    void continueRequestReceived(std::shared_ptr<OnContinueContext> onContinueContext);

protected:
    AbilityContext();
    ~AbilityContext() override;

    Q_DISABLE_COPY(AbilityContext)
};

Q_HARMONYEXTRAS_EXPORT std::shared_ptr<OperationStatus> startAbility(const Want &want, std::shared_ptr<StartOptions> options = nullptr);

Q_HARMONYEXTRAS_EXPORT std::shared_ptr<OperationStatus> startAbilityByType(const QString &appType, const QJsonObject &wantParameters);

Q_HARMONYEXTRAS_EXPORT void startNewAbilityInstance(QWidget *instanceWidget);

Q_HARMONYEXTRAS_EXPORT void startAppProcess(const QString &processId, const Want &requestWant, std::shared_ptr<StartOptions> options = nullptr);

Q_HARMONYEXTRAS_EXPORT void setAbilityInstanceDestroyEnabled(QWindow *instanceWindow, bool destroyEnabled);

Q_HARMONYEXTRAS_EXPORT std::optional<QByteArray> tryGetOnContinueData(const Want &want);

}

QT_END_NAMESPACE

#endif // QOHOSABILITYCONTEXT_P_H
