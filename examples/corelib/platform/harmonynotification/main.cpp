// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QFormLayout>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QString>
#include <QWidget>
#include <QtCore/qharmonyappjscontext.h>
#include <QtCore/qharmonyjscontext.h>
#include <QtCore/qharmonyjsonconversions.h>
#include <napi.h>
#include <string>

using QHarmonyApplication = QNativeInterface::QHarmonyApplication;

namespace {

const std::string notificationManagerModuleName = "@ohos.notificationManager";

//! [3]
void showStatusInLabel(QPointer<QLabel> statusLabel, QString text)
{
    QHarmonyApplication::runOnQtThread(
        [statusLabel, text]() {
            if (statusLabel)
                statusLabel->setText(text);
        });
}

void showFailureInLabel(QPointer<QLabel> statusLabel, const QString &asyncCallName, const QString &errorText)
{
    showStatusInLabel(statusLabel, QStringLiteral("%1 failed: %2").arg(asyncCallName, errorText));
}

void attachPromiseHandlersReportingStatusInLabel(Napi::Object asyncCallPromise, const QString &asyncCallName, QPointer<QLabel> statusLabel)
{
    asyncCallPromise.Get("then").As<Napi::Function>().Call(
        asyncCallPromise,
        {
            Napi::Function::New(
                asyncCallPromise.Env(),
                [statusLabel, asyncCallName](const Napi::CallbackInfo &) {
                    showStatusInLabel(statusLabel, QStringLiteral("%1 succeeded").arg(asyncCallName));
                }),
            Napi::Function::New(
                asyncCallPromise.Env(),
                [statusLabel, asyncCallName](const Napi::CallbackInfo &info) {
                    showFailureInLabel(statusLabel, asyncCallName, QString::fromStdString(info[0].ToString()));
                }),
        });
}
//! [3]

//! [1]
void askUserToAllowNotifications(QHarmonyAppJsContext &appJsContext, QPointer<QLabel> statusLabel)
{
    const QString asyncCallName = QStringLiteral("requestEnableNotification()");
    Napi::Env env(appJsContext.env());
    Napi::Object notificationManagerModule(env, appJsContext.tryImportModule(notificationManagerModuleName));
    if (notificationManagerModule.IsEmpty()) {
        showFailureInLabel(statusLabel, asyncCallName, QStringLiteral("no notification manager module"));
        return;
    }

    Napi::Object ability(env, appJsContext.defaultAbility());
    if (ability.IsEmpty()) {
        showFailureInLabel(statusLabel, asyncCallName, QStringLiteral("no ability to request from"));
        return;
    }

    try {
        auto abilityContext = ability.Get("context").As<Napi::Object>();
        auto requestEnableNotificationFunc = notificationManagerModule.Get("requestEnableNotification").As<Napi::Function>();
        auto requestEnableNotificationPromise = requestEnableNotificationFunc.Call(notificationManagerModule, {abilityContext}).As<Napi::Object>();
        attachPromiseHandlersReportingStatusInLabel(requestEnableNotificationPromise, asyncCallName, statusLabel);
    } catch (const Napi::Error &error) {
        showFailureInLabel(statusLabel, asyncCallName, QString::fromUtf8(error.what()));
    }
}
//! [1]

//! [2]
void postNotification(QHarmonyJsContext &jsContext, const QJsonObject &titleAndText, QPointer<QLabel> statusLabel)
{
    const QString asyncCallName = QStringLiteral("publish()");
    Napi::Env env(jsContext.env());
    Napi::Object notificationManagerModule(env, jsContext.tryImportModule(notificationManagerModuleName));
    if (notificationManagerModule.IsEmpty()) {
        showFailureInLabel(statusLabel, asyncCallName, QStringLiteral("no notification manager module"));
        return;
    }

    try {
        auto basicTextContentType = notificationManagerModule.Get("ContentType").As<Napi::Object>().Get("NOTIFICATION_CONTENT_BASIC_TEXT");
        Napi::Object jsTitleAndText(env, QtHarmony::mapQJsonObjectToJsObject(env, titleAndText));

        auto notificationContent = Napi::Object::New(env);
        notificationContent.Set("notificationContentType", basicTextContentType);
        notificationContent.Set("normal", jsTitleAndText);
        auto notificationRequest = Napi::Object::New(env);
        notificationRequest.Set("content", notificationContent);

        auto publishFunc = notificationManagerModule.Get("publish").As<Napi::Function>();
        auto publishPromise = publishFunc.Call(notificationManagerModule, {notificationRequest}).As<Napi::Object>();
        attachPromiseHandlersReportingStatusInLabel(publishPromise, asyncCallName, statusLabel);
    } catch (const Napi::Error &error) {
        showFailureInLabel(statusLabel, asyncCallName, QString::fromUtf8(error.what()));
    }
}
//! [2]

}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle(QStringLiteral("Notification example"));
    // the platform shows the requestEnableNotification() dialog inside this window, so it has to fit
    window.resize(800, 600);

    auto *notificationTitleEdit = new QLineEdit(QStringLiteral("Hello from Qt"), &window);
    auto *notificationTextEdit = new QLineEdit(QStringLiteral("This notification was posted by a Qt application."), &window);
    auto *allowNotificationsButton = new QPushButton(QStringLiteral("Allow notifications"), &window);
    auto *postNotificationButton = new QPushButton(QStringLiteral("Post notification"), &window);
    auto *statusLabel = new QLabel(&window);
    statusLabel->setWordWrap(true);

    auto *layout = new QFormLayout(&window);
    layout->addRow(QStringLiteral("Title"), notificationTitleEdit);
    layout->addRow(QStringLiteral("Text"), notificationTextEdit);
    layout->addRow(allowNotificationsButton);
    layout->addRow(postNotificationButton);
    layout->addRow(statusLabel);

    //! [0]
    QObject::connect(
        allowNotificationsButton, &QPushButton::clicked, &window,
        [statusLabel]() {
            statusLabel->clear();
            QHarmonyApplication::runOnJsThread(
                [statusLabel = QPointer<QLabel>(statusLabel)](QHarmonyJsContext &jsContext) {
                    askUserToAllowNotifications(QHarmonyAppJsContext::fromJsContext(jsContext), statusLabel);
                });
        });
    QObject::connect(
        postNotificationButton, &QPushButton::clicked, &window,
        [notificationTitleEdit, notificationTextEdit, statusLabel]() {
            const QJsonObject titleAndText{
                {QStringLiteral("title"), notificationTitleEdit->text()},
                {QStringLiteral("text"), notificationTextEdit->text()},
            };
            statusLabel->clear();
            QHarmonyApplication::runOnJsThread(
                [titleAndText, statusLabel = QPointer<QLabel>(statusLabel)](QHarmonyJsContext &jsContext) {
                    postNotification(jsContext, titleAndText, statusLabel);
                });
        });
    //! [0]

    window.show();
    return app.exec();
}
