// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QCheckBox>
#include <QFormLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QList>
#include <QPointer>
#include <QPushButton>
#include <QSpinBox>
#include <QString>
#include <QTextEdit>
#include <QWidget>
#include <QWindow>
#include <QtCore/qharmonyjscontext.h>
#include <QtCore/qharmonyjsobjectref.h>
#include <QtCore/qharmonyjsonconversions.h>
#include <napi.h>

using QHarmonyApplication = QNativeInterface::QHarmonyApplication;

namespace {

struct PolicyParameter
{
    QString key;
    QCheckBox *setCheckBox;
    QSpinBox *valueSpinBox;
    int defaultValue;
};

QString toCompactJson(const QJsonObject &jsonObject)
{
    return QString::fromUtf8(QJsonDocument(jsonObject).toJson(QJsonDocument::Compact));
}

//! [1]
void showStatusInLog(QPointer<QTextEdit> log, QString text)
{
    QHarmonyApplication::runOnQtThread(
        [log, text]() {
            if (log)
                log->append(text);
        });
}

void showFailureInLog(QPointer<QTextEdit> log, const QString &asyncCallName, const QString &errorText)
{
    showStatusInLog(log, QStringLiteral("%1 failed: %2").arg(asyncCallName, errorText));
}

void applyDragKeyFramePolicy(
    QHarmonyJsContext &jsContext, const QHarmonyJsObjectRef &jsWindowRef, const QJsonObject &policy,
    QPointer<QTextEdit> log)
{
    const QString asyncCallName = QStringLiteral("setDragKeyFramePolicy()");
    Napi::Env env(jsContext.env());
    Napi::Object jsWindow(env, jsWindowRef.value(jsContext));
    if (jsWindow.IsEmpty()) {
        showFailureInLog(log, asyncCallName, QStringLiteral("the window is not shown in a window.Window"));
        return;
    }

    try {
        Napi::Object jsPolicy(env, QtHarmony::mapQJsonObjectToJsObject(env, policy));
        auto setDragKeyFramePolicyFunc = jsWindow.Get("setDragKeyFramePolicy").As<Napi::Function>();
        auto setDragKeyFramePolicyPromise = setDragKeyFramePolicyFunc.Call(jsWindow, {jsPolicy}).As<Napi::Object>();
        setDragKeyFramePolicyPromise.Get("then").As<Napi::Function>().Call(
            setDragKeyFramePolicyPromise,
            {
                Napi::Function::New(
                    env,
                    [log, asyncCallName](const Napi::CallbackInfo &info) {
                        const QJsonObject effectivePolicy = QtHarmony::mapJsObjectToQJsonObject(info.Env(), info[0]);
                        showStatusInLog(log, QStringLiteral("%1 succeeded, effective policy: %2").arg(asyncCallName, toCompactJson(effectivePolicy)));
                    }),
                Napi::Function::New(
                    env,
                    [log, asyncCallName](const Napi::CallbackInfo &info) {
                        showFailureInLog(log, asyncCallName, QString::fromStdString(info[0].ToString()));
                    }),
            });
    } catch (const Napi::Error &error) {
        showFailureInLog(log, asyncCallName, QString::fromUtf8(error.what()));
    }
}
//! [1]

class KeyFramePolicyWindow : public QWidget
{
public:
    KeyFramePolicyWindow();

private:
    void addPolicyParameter(QFormLayout *layout, const QString &key, const QString &label, int minimum, int defaultValue, const QString &unit);
    void updatePolicyParameterControls();
    QJsonObject makePolicy() const;
    void applyPolicy();
    void resetPolicy();

    QCheckBox *m_enableCheckBox;
    QList<PolicyParameter> m_policyParameters;
    QTextEdit *m_log;
};

KeyFramePolicyWindow::KeyFramePolicyWindow()
    : m_enableCheckBox(new QCheckBox(QStringLiteral("Key frame mode enabled"), this))
    , m_log(new QTextEdit(this))
{
    setWindowTitle(QStringLiteral("Window drag key frame example"));
    m_log->setReadOnly(true);

    auto *hintLabel = new QLabel(QStringLiteral("Enable the key frame mode, press Apply and resize the window by dragging its edge."), this);
    hintLabel->setWordWrap(true);

    auto *layout = new QFormLayout(this);
    layout->addRow(hintLabel);
    layout->addRow(m_enableCheckBox);
    addPolicyParameter(layout, QStringLiteral("interval"), QStringLiteral("Interval"), 1, 1000, QStringLiteral("ms"));
    addPolicyParameter(layout, QStringLiteral("distance"), QStringLiteral("Distance"), 0, 1000, QStringLiteral("px"));
    addPolicyParameter(layout, QStringLiteral("animationDuration"), QStringLiteral("Animation duration"), 0, 100, QStringLiteral("ms"));
    addPolicyParameter(layout, QStringLiteral("animationDelay"), QStringLiteral("Animation delay"), 0, 100, QStringLiteral("ms"));

    auto *applyButton = new QPushButton(QStringLiteral("Apply"), this);
    connect(applyButton, &QPushButton::clicked, this, &KeyFramePolicyWindow::applyPolicy);
    auto *resetButton = new QPushButton(QStringLiteral("Reset to defaults"), this);
    connect(resetButton, &QPushButton::clicked, this, &KeyFramePolicyWindow::resetPolicy);
    layout->addRow(applyButton);
    layout->addRow(resetButton);
    layout->addRow(m_log);

    connect(m_enableCheckBox, &QCheckBox::toggled, this, &KeyFramePolicyWindow::updatePolicyParameterControls);
    updatePolicyParameterControls();
}

void KeyFramePolicyWindow::addPolicyParameter(QFormLayout *layout, const QString &key, const QString &label, int minimum, int defaultValue, const QString &unit)
{
    auto *setCheckBox = new QCheckBox(label, this);
    auto *valueSpinBox = new QSpinBox(this);
    valueSpinBox->setRange(minimum, 2000);
    valueSpinBox->setValue(defaultValue);
    valueSpinBox->setSuffix(QStringLiteral(" %1").arg(unit));
    connect(setCheckBox, &QCheckBox::toggled, this, &KeyFramePolicyWindow::updatePolicyParameterControls);

    layout->addRow(setCheckBox, valueSpinBox);
    m_policyParameters.append(PolicyParameter{
        .key = key,
        .setCheckBox = setCheckBox,
        .valueSpinBox = valueSpinBox,
        .defaultValue = defaultValue,
    });
}

void KeyFramePolicyWindow::updatePolicyParameterControls()
{
    const bool enabled = m_enableCheckBox->isChecked();
    for (const PolicyParameter &parameter : m_policyParameters) {
        parameter.setCheckBox->setEnabled(enabled);
        parameter.valueSpinBox->setEnabled(enabled && parameter.setCheckBox->isChecked());
    }
}

//! [0]
QJsonObject KeyFramePolicyWindow::makePolicy() const
{
    QJsonObject policy{{QStringLiteral("enable"), m_enableCheckBox->isChecked()}};
    if (!m_enableCheckBox->isChecked())
        return policy;

    for (const PolicyParameter &parameter : m_policyParameters) {
        if (parameter.setCheckBox->isChecked())
            policy.insert(parameter.key, parameter.valueSpinBox->value());
    }
    return policy;
}

void KeyFramePolicyWindow::applyPolicy()
{
    auto *harmonyWindow = windowHandle() ? windowHandle()->nativeInterface<QNativeInterface::QHarmonyWindow>() : nullptr;
    const auto jsWindowRef = harmonyWindow ? harmonyWindow->jsWindow() : QHarmonyJsObjectRef();
    const QJsonObject policy = makePolicy();
    m_log->append(QStringLiteral("requested policy: %1").arg(toCompactJson(policy)));

    QHarmonyApplication::runOnJsThread(
        [jsWindowRef, policy, log = QPointer<QTextEdit>(m_log)](QHarmonyJsContext &jsContext) {
            applyDragKeyFramePolicy(jsContext, jsWindowRef, policy, log);
        });
}
//! [0]

void KeyFramePolicyWindow::resetPolicy()
{
    m_enableCheckBox->setChecked(false);
    for (const PolicyParameter &parameter : m_policyParameters) {
        parameter.setCheckBox->setChecked(false);
        parameter.valueSpinBox->setValue(parameter.defaultValue);
    }
    applyPolicy();
}

}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    KeyFramePolicyWindow window;
    window.show();

    return app.exec();
}
