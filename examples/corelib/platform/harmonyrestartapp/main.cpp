// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QJsonObject>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qharmonyappjscontext.h>
#include <QtCore/qharmonyjscontext.h>
#include <QtCore/qharmonyjsonconversions.h>
#include <functional>
#include <napi.h>

using QHarmonyApplication = QNativeInterface::QHarmonyApplication;

namespace {

const QString wantParametersPropName = QStringLiteral("parameters");
const QString restartCountWantParamName = QStringLiteral("org.qtproject.example.restartCount");

//! [2]
void showStatusInLabel(QPointer<QLabel> statusLabel, QString text)
{
    QHarmonyApplication::runOnQtThread(
        [statusLabel, text]() {
            if (statusLabel)
                statusLabel->setText(text);
        });
}

void showFailureInLabel(QPointer<QLabel> statusLabel, const QString &errorText)
{
    showStatusInLabel(statusLabel, QStringLiteral("Failed to restart application: %1").arg(errorText));
}
//! [2]

//! [0]
void restartApplicationWithWant(QHarmonyAppJsContext &appJsContext, Napi::Object want, QPointer<QLabel> statusLabel)
{
    Napi::Env env(appJsContext.env());
    Napi::Object ability(env, appJsContext.defaultAbility());
    if (ability.IsEmpty()) {
        showFailureInLabel(statusLabel, QStringLiteral("no ability to restart from"));
        return;
    }

    try {
        auto abilityContext = ability.Get("context").As<Napi::Object>();
        auto getApplicationContext = abilityContext.Get("getApplicationContext").As<Napi::Function>();
        auto applicationContext = getApplicationContext.Call(abilityContext, {}).As<Napi::Object>();
        auto restartAppFunc = applicationContext.Get("restartApp").As<Napi::Function>();
        restartAppFunc.Call(applicationContext, {want});
    } catch (const Napi::Error &error) {
        showFailureInLabel(statusLabel, QString::fromUtf8(error.what()));
    }
}
//! [0]

Napi::Object appLaunchWant(QHarmonyAppJsContext &appJsContext)
{
    Napi::Env env(appJsContext.env());
    Napi::Object ability(env, appJsContext.defaultAbility());
    if (ability.IsEmpty())
        return Napi::Object();
    return ability.Get("launchWant").As<Napi::Object>();
}

QJsonObject readLaunchWant()
{
    return QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            auto &appJsContext = QHarmonyAppJsContext::fromJsContext(jsContext);
            return QtHarmony::mapJsObjectToQJsonObject(appJsContext.env(), appLaunchWant(appJsContext));
        });
}

QWidget *createActionButton(const QString &label, QWidget *parent, std::function<void()> action)
{
    auto *button = new QPushButton(label, parent);
    QObject::connect(button, &QPushButton::clicked, parent, std::move(action));
    return button;
}

int restartCountFromWant(const QJsonObject &want)
{
    return want.value(wantParametersPropName).toObject().value(restartCountWantParamName).toInt();
}

QJsonObject makeWantWithRestartCount(const QJsonObject &launchWant, int restartCount)
{
    QJsonObject parameters = launchWant.value(wantParametersPropName).toObject();
    parameters.insert(restartCountWantParamName, restartCount);
    QJsonObject want = launchWant;
    want.insert(wantParametersPropName, parameters);
    return want;
}

}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    const QJsonObject launchWant = readLaunchWant();
    const int restartCount = restartCountFromWant(launchWant);

    QWidget window;
    window.setWindowTitle(QStringLiteral("Restart application example"));

    auto *statusLabel = new QLabel(&window);
    statusLabel->setWordWrap(true);

    auto *layout = new QVBoxLayout(&window);
    //! [1]
    layout->addWidget(
        createActionButton(
            QStringLiteral("Restart with the launch Want\n(the restart count stays the same)"),
            &window,
            [statusLabel]() {
                statusLabel->clear();
                QHarmonyApplication::runOnJsThread(
                    [statusLabel = QPointer<QLabel>(statusLabel)](QHarmonyJsContext &jsContext) {
                        auto &appJsContext = QHarmonyAppJsContext::fromJsContext(jsContext);
                        restartApplicationWithWant(appJsContext, appLaunchWant(appJsContext), statusLabel);
                    });
            }));
    layout->addWidget(
        createActionButton(
            QStringLiteral("Restart with the restart count increased"),
            &window,
            [statusLabel, want = makeWantWithRestartCount(launchWant, restartCount + 1)]() {
                statusLabel->clear();
                QHarmonyApplication::runOnJsThread(
                    [want, statusLabel = QPointer<QLabel>(statusLabel)](QHarmonyJsContext &jsContext) {
                        auto &appJsContext = QHarmonyAppJsContext::fromJsContext(jsContext);
                        Napi::Object jsWant(appJsContext.env(), QtHarmony::mapQJsonObjectToJsObject(appJsContext.env(), want));
                        restartApplicationWithWant(appJsContext, jsWant, statusLabel);
                    });
            }));
    //! [1]
    layout->addWidget(new QLabel(QStringLiteral("Restart count: %1").arg(restartCount), &window));
    layout->addWidget(statusLabel);

    window.show();
    return app.exec();
}
