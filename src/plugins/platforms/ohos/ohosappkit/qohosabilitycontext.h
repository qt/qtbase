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
#include <QtOhosAppKit/qohosstartrequest.h>
#include <QtOhosAppKit/qtohosappkitglobal.h>
#include <QtOhosAppKit/qohossharekit.h>
#include <QtWidgets/qwidget.h>
#include <functional>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

class Q_OHOSAPPKIT_EXPORT QOhosOpenLinkOptions
{
public:
    virtual ~QOhosOpenLinkOptions();

    virtual void setAppLinkingOnly(bool appLinkingOnly) = 0;

protected:
    QOhosOpenLinkOptions();

private:
    Q_DISABLE_COPY(QOhosOpenLinkOptions)
};

Q_OHOSAPPKIT_EXPORT QSharedPointer<QOhosOpenLinkOptions> createOpenLinkOptions();

class Q_OHOSAPPKIT_EXPORT QOhosOnContinueContext
{
public:
    virtual ~QOhosOnContinueContext();

    virtual void setAgreeResponse(const QByteArray &responseData) = 0;
    virtual void setRejectResponse() = 0;
    virtual void setMismatchResponse() = 0;

    virtual void setExitAppOnSourceDeviceAfterMigration(bool exitAfterMigration) = 0;

    virtual int sourceApplicationVersionCode() const = 0;

protected:
    QOhosOnContinueContext();

private:
    Q_DISABLE_COPY(QOhosOnContinueContext)
};

struct QOhosStartAbilityResult
{
    int resultCode = 0;
    QSharedPointer<QOhosWant> want;
};

class Q_OHOSAPPKIT_EXPORT QOhosAbilityContext : public QObject
{
    Q_OBJECT

public:
    static QSharedPointer<QOhosAbilityContext> getDefaultInstance();
    static QSharedPointer<QOhosAbilityContext> getInstanceForMainWindow(QWindow *instanceMainWindow);

    virtual void setDestroyFromSystemEnabled(bool destroyEnabled) = 0;

    virtual void startAbilityForResult(
        const QOhosWant &want, QObject *context,
        std::function<void(std::optional<QOhosStartAbilityResult>)> callback) = 0;
    virtual void startAbilityForResult(
        const QOhosWant &want, const QOhosStartOptions &options, QObject *context,
        std::function<void(std::optional<QOhosStartAbilityResult>)> callback) = 0;
    virtual void startAbilityForResult(
        const QOhosWant &want, const QOhosStartRequest &startRequest, QObject *context,
        std::function<void(std::optional<QOhosStartAbilityResult>)> callback) = 0;

    virtual void shareDataWithShareKit(
        const QList<QSharedPointer<ShareKit::QOhosSharedRecord>> &records,
        QSharedPointer<ShareKit::QOhosShareControllerOptions> controllerOptions,
        QObject *context,
        std::function<void(QSharedPointer<ShareKit::QOhosShareOperationResult>)> onShareCompleted,
        std::function<void()> onPanelClosed) = 0;

    virtual bool tryOpenLink(const QString &link) = 0;
    virtual bool tryOpenLink(const QString &link, const QOhosOpenLinkOptions &options) = 0;

    virtual void setContinuationActive(bool continuationActive) = 0;

Q_SIGNALS:
    void newWantInfoReceived(QSharedPointer<QOhosWantInfo> wantInfo);
    void continueRequestReceived(QSharedPointer<QOhosOnContinueContext> onContinueContext);

protected:
    QOhosAbilityContext();

    Q_DISABLE_COPY(QOhosAbilityContext)
};

Q_OHOSAPPKIT_EXPORT QSharedPointer<QOhosOperationStatus> startAbility(const QOhosWant &want);
Q_OHOSAPPKIT_EXPORT QSharedPointer<QOhosOperationStatus> startAbility(const QOhosWant &want, const QOhosStartOptions &options);
Q_OHOSAPPKIT_EXPORT QSharedPointer<QOhosOperationStatus> startAbility(const QOhosWant &want, const QOhosStartRequest &startRequest);

Q_OHOSAPPKIT_EXPORT QSharedPointer<QOhosOperationStatus> startAbilityByType(const QString &appType, const QJsonObject &wantParameters);

Q_OHOSAPPKIT_EXPORT void startNewAbilityInstance(QWidget *instanceWidget);

Q_OHOSAPPKIT_EXPORT void startAppProcess(const QString &processId, const QOhosWant &requestWant);
Q_OHOSAPPKIT_EXPORT void startAppProcess(const QString &processId, const QOhosWant &requestWant, const QOhosStartOptions &options);
Q_OHOSAPPKIT_EXPORT void startAppProcess(const QString &processId, const QOhosWant &requestWant, const QOhosStartRequest &startRequest);

Q_OHOSAPPKIT_EXPORT void setAbilityInstanceDestroyEnabled(QWindow *instanceWindow, bool destroyEnabled);

Q_OHOSAPPKIT_EXPORT QSharedPointer<QByteArray> tryGetOnContinueData(const QOhosWant &want);

}

QT_END_NAMESPACE

#endif // QOHOSABILITYCONTEXT_H
