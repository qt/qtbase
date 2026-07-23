// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSABILITYCONTEXT_H
#define QOHOSABILITYCONTEXT_H

#include <QtCore/qbytearray.h>
#include <QtCore/qlist.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qstring.h>
#include <QtCore/qjsonobject.h>
#include <QtGui/qwindow.h>
#include <QtOhosAppKit/qohosoperationstatus.h>
#include <QtOhosAppKit/qohoswant.h>
#include <QtOhosAppKit/qohosstartoptions.h>
#include <QtOhosAppKit/qtohosappkitglobal.h>
#include <QtOhosAppKit/qohossharekit.h>
#include <QtWidgets/qwidget.h>
#include <functional>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

class Q_OHOSAPPKIT_EXPORT OpenLinkOptions
{
public:
    virtual ~OpenLinkOptions();

    virtual void setAppLinkingOnly(bool appLinkingOnly) = 0;

protected:
    OpenLinkOptions();

private:
    Q_DISABLE_COPY(OpenLinkOptions)
};

Q_OHOSAPPKIT_EXPORT QSharedPointer<OpenLinkOptions> createOpenLinkOptions();

class Q_OHOSAPPKIT_EXPORT OnContinueContext
{
public:
    virtual ~OnContinueContext();

    virtual void setAgreeResponse(const QByteArray &responseData) = 0;
    virtual void setRejectResponse() = 0;
    virtual void setMismatchResponse() = 0;

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

class Q_OHOSAPPKIT_EXPORT AbilityContext : public QObject
{
    Q_OBJECT

public:
    static QSharedPointer<AbilityContext> defaultInstance();
    static QSharedPointer<AbilityContext> instanceForMainWindow(QWindow *instanceMainWindow);

    virtual void setDestroyFromSystemEnabled(bool destroyEnabled) = 0;

    virtual void startAbilityForResult(
        const Want &want, QObject *context,
        std::function<void(std::optional<StartAbilityResult>)> callback) = 0;
    virtual void startAbilityForResult(
        const Want &want, const StartOptions &options, QObject *context,
        std::function<void(std::optional<StartAbilityResult>)> callback) = 0;

    virtual void shareDataWithShareKit(
        const QList<QSharedPointer<ShareKit::SharedRecord>> &records,
        QSharedPointer<ShareKit::ShareControllerOptions> controllerOptions,
        QObject *context,
        std::function<void(QSharedPointer<ShareKit::ShareOperationResult>)> onShareCompleted,
        std::function<void()> onPanelClosed) = 0;

    virtual bool tryOpenLink(const QString &link) = 0;
    virtual bool tryOpenLink(const QString &link, const OpenLinkOptions &options) = 0;

    virtual void setContinuationActive(bool continuationActive) = 0;

Q_SIGNALS:
    void newWantInfoReceived(QSharedPointer<WantInfo> wantInfo);
    void continueRequestReceived(QSharedPointer<OnContinueContext> onContinueContext);

protected:
    AbilityContext();

    Q_DISABLE_COPY(AbilityContext)
};

Q_OHOSAPPKIT_EXPORT QSharedPointer<OperationStatus> startAbility(const Want &want);
Q_OHOSAPPKIT_EXPORT QSharedPointer<OperationStatus> startAbility(const Want &want, const StartOptions &options);

Q_OHOSAPPKIT_EXPORT QSharedPointer<OperationStatus> startAbilityByType(const QString &appType, const QJsonObject &wantParameters);

Q_OHOSAPPKIT_EXPORT void startNewAbilityInstance(QWidget *instanceWidget);

Q_OHOSAPPKIT_EXPORT void startAppProcess(const QString &processId, const Want &requestWant);
Q_OHOSAPPKIT_EXPORT void startAppProcess(const QString &processId, const Want &requestWant, const StartOptions &options);

Q_OHOSAPPKIT_EXPORT void setAbilityInstanceDestroyEnabled(QWindow *instanceWindow, bool destroyEnabled);

Q_OHOSAPPKIT_EXPORT std::optional<QByteArray> tryGetOnContinueData(const Want &want);

}

QT_END_NAMESPACE

#endif // QOHOSABILITYCONTEXT_H
