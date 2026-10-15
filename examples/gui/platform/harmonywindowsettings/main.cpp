// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFormLayout>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QSignalBlocker>
#include <QString>
#include <QTextEdit>
#include <QWidget>
#include <QWindow>
#include <QtCore/qharmonyjscontext.h>
#include <QtCore/qharmonyjsobjectref.h>
#include <napi.h>
#include <optional>
#include <string>

using QHarmonyApplication = QNativeInterface::QHarmonyApplication;

namespace {

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

void attachPromiseHandlersReportingStatusInLog(Napi::Object asyncCallPromise, const QString &asyncCallName, QPointer<QTextEdit> log)
{
    asyncCallPromise.Get("then").As<Napi::Function>().Call(
        asyncCallPromise,
        {
            Napi::Function::New(
                asyncCallPromise.Env(),
                [log, asyncCallName](const Napi::CallbackInfo &) {
                    showStatusInLog(log, QStringLiteral("%1 succeeded").arg(asyncCallName));
                }),
            Napi::Function::New(
                asyncCallPromise.Env(),
                [log, asyncCallName](const Napi::CallbackInfo &info) {
                    showFailureInLog(log, asyncCallName, QString::fromStdString(info[0].ToString()));
                }),
        });
}

void callJsWindowSetter(
    QHarmonyJsContext &jsContext, const QHarmonyJsObjectRef &jsWindowRef, const std::string &setterName,
    Napi::Value argument, QPointer<QTextEdit> log)
{
    const QString asyncCallName = QStringLiteral("%1()").arg(QString::fromStdString(setterName));
    Napi::Env env(jsContext.env());
    Napi::Object jsWindow(env, jsWindowRef.value(jsContext));
    if (jsWindow.IsEmpty()) {
        showFailureInLog(log, asyncCallName, QStringLiteral("the window is not shown in a window.Window"));
        return;
    }

    try {
        auto setterFunc = jsWindow.Get(setterName).As<Napi::Function>();
        auto setterPromise = setterFunc.Call(jsWindow, {argument}).As<Napi::Object>();
        attachPromiseHandlersReportingStatusInLog(setterPromise, asyncCallName, log);
    } catch (const Napi::Error &error) {
        showFailureInLog(log, asyncCallName, QString::fromUtf8(error.what()));
    }
}
//! [1]

//! [2]
struct PlatformWindowSettings
{
    std::optional<bool> optKeepScreenOn;
    std::optional<bool> optPrivacyMode;
    std::optional<double> optCornerRadius;
    QString errorText;
};

PlatformWindowSettings readPlatformWindowSettings(QHarmonyJsContext &jsContext, const QHarmonyJsObjectRef &jsWindowRef, bool subWindow)
{
    Napi::Env env(jsContext.env());
    Napi::Object jsWindow(env, jsWindowRef.value(jsContext));
    if (jsWindow.IsEmpty()) {
        return PlatformWindowSettings{
            .errorText = QStringLiteral("the window is not shown in a window.Window"),
        };
    }

    PlatformWindowSettings settings;
    try {
        auto getWindowPropertiesFunc = jsWindow.Get("getWindowProperties").As<Napi::Function>();
        auto windowProperties = getWindowPropertiesFunc.Call(jsWindow, {}).As<Napi::Object>();
        settings.optKeepScreenOn = windowProperties.Get("isKeepScreenOn").As<Napi::Boolean>().Value();
        settings.optPrivacyMode = windowProperties.Get("isPrivacyMode").As<Napi::Boolean>().Value();
        if (subWindow) {
            auto getWindowCornerRadiusFunc = jsWindow.Get("getWindowCornerRadius").As<Napi::Function>();
            settings.optCornerRadius = getWindowCornerRadiusFunc.Call(jsWindow, {}).As<Napi::Number>().DoubleValue();
        }
    } catch (const Napi::Error &error) {
        settings.errorText = QString::fromUtf8(error.what());
    }
    return settings;
}
//! [2]

class WindowSettingsWindow : public QWidget
{
public:
    explicit WindowSettingsWindow(QWidget *mainWindow = nullptr);

protected:
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    bool isSubWindow() const;
    QHarmonyJsObjectRef jsWindow() const;
    void syncWithPlatformWindow();
    void setKeepScreenOn(bool keepScreenOn);
    void setPrivacyMode(bool privacyMode);
    void setCornerRadius(double cornerRadius);
    void setDragResizable(bool dragResizable);
    void openSubWindow();

    QCheckBox *m_keepScreenOnCheckBox;
    QCheckBox *m_privacyModeCheckBox;
    QDoubleSpinBox *m_cornerRadiusSpinBox;
    QCheckBox *m_dragResizableCheckBox;
    QTextEdit *m_log;
};

WindowSettingsWindow::WindowSettingsWindow(QWidget *mainWindow)
    : QWidget(mainWindow)
    , m_keepScreenOnCheckBox(new QCheckBox(QStringLiteral("Keep the screen on"), this))
    , m_privacyModeCheckBox(new QCheckBox(QStringLiteral("Hide the content in screenshots and recordings"), this))
    , m_cornerRadiusSpinBox(new QDoubleSpinBox(this))
    , m_dragResizableCheckBox(new QCheckBox(QStringLiteral("Resizable by dragging its edges"), this))
    , m_log(new QTextEdit(this))
{
    m_cornerRadiusSpinBox->setRange(0.0, 1000.0);
    m_cornerRadiusSpinBox->setDecimals(1);
    m_cornerRadiusSpinBox->setSuffix(QStringLiteral(" vp"));
    m_cornerRadiusSpinBox->setKeyboardTracking(false);
    m_dragResizableCheckBox->setChecked(true);
    m_log->setReadOnly(true);

    auto *layout = new QFormLayout(this);
    layout->addRow(m_keepScreenOnCheckBox);
    layout->addRow(m_privacyModeCheckBox);
    layout->addRow(QStringLiteral("Corner radius"), m_cornerRadiusSpinBox);
    layout->addRow(m_dragResizableCheckBox);

    if (isSubWindow()) {
        setWindowFlags(Qt::Window);
        setAttribute(Qt::WA_DeleteOnClose);
        setWindowTitle(QStringLiteral("Sub window"));
    } else {
        setWindowTitle(QStringLiteral("Window settings example"));
        m_cornerRadiusSpinBox->setEnabled(false);
        m_dragResizableCheckBox->setEnabled(false);

        auto *hintLabel = new QLabel(QStringLiteral("The corner radius and resizing by dragging apply only to sub windows."), this);
        hintLabel->setWordWrap(true);
        auto *openSubWindowButton = new QPushButton(QStringLiteral("New sub window"), this);
        connect(openSubWindowButton, &QPushButton::clicked, this, &WindowSettingsWindow::openSubWindow);
        layout->addRow(hintLabel);
        layout->addRow(openSubWindowButton);
    }
    layout->addRow(m_log);

    connect(m_keepScreenOnCheckBox, &QCheckBox::toggled, this, &WindowSettingsWindow::setKeepScreenOn);
    connect(m_privacyModeCheckBox, &QCheckBox::toggled, this, &WindowSettingsWindow::setPrivacyMode);
    connect(m_cornerRadiusSpinBox, &QDoubleSpinBox::valueChanged, this, &WindowSettingsWindow::setCornerRadius);
    connect(m_dragResizableCheckBox, &QCheckBox::toggled, this, &WindowSettingsWindow::setDragResizable);
}

bool WindowSettingsWindow::isSubWindow() const
{
    return parentWidget();
}

//! [0]
QHarmonyJsObjectRef WindowSettingsWindow::jsWindow() const
{
    auto *harmonyWindow = windowHandle() ? windowHandle()->nativeInterface<QNativeInterface::QHarmonyWindow>() : nullptr;
    return harmonyWindow ? harmonyWindow->jsWindow() : QHarmonyJsObjectRef();
}

void WindowSettingsWindow::setKeepScreenOn(bool keepScreenOn)
{
    QHarmonyApplication::runOnJsThread(
        [jsWindowRef = jsWindow(), keepScreenOn, log = QPointer<QTextEdit>(m_log)](QHarmonyJsContext &jsContext) {
            callJsWindowSetter(jsContext, jsWindowRef, "setWindowKeepScreenOn", Napi::Boolean::New(jsContext.env(), keepScreenOn), log);
        });
}

void WindowSettingsWindow::setPrivacyMode(bool privacyMode)
{
    QHarmonyApplication::runOnJsThread(
        [jsWindowRef = jsWindow(), privacyMode, log = QPointer<QTextEdit>(m_log)](QHarmonyJsContext &jsContext) {
            callJsWindowSetter(jsContext, jsWindowRef, "setWindowPrivacyMode", Napi::Boolean::New(jsContext.env(), privacyMode), log);
        });
}

void WindowSettingsWindow::setCornerRadius(double cornerRadius)
{
    QHarmonyApplication::runOnJsThread(
        [jsWindowRef = jsWindow(), cornerRadius, log = QPointer<QTextEdit>(m_log)](QHarmonyJsContext &jsContext) {
            callJsWindowSetter(jsContext, jsWindowRef, "setWindowCornerRadius", Napi::Number::New(jsContext.env(), cornerRadius), log);
        });
}

void WindowSettingsWindow::setDragResizable(bool dragResizable)
{
    QHarmonyApplication::runOnJsThread(
        [jsWindowRef = jsWindow(), dragResizable, log = QPointer<QTextEdit>(m_log)](QHarmonyJsContext &jsContext) {
            callJsWindowSetter(jsContext, jsWindowRef, "enableDrag", Napi::Boolean::New(jsContext.env(), dragResizable), log);
        });
}
//! [0]

//! [3]
void WindowSettingsWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    windowHandle()->installEventFilter(this);
}

bool WindowSettingsWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == windowHandle() && event->type() == QEvent::Expose && windowHandle()->isExposed()) {
        windowHandle()->removeEventFilter(this);
        syncWithPlatformWindow();
    }
    return QWidget::eventFilter(watched, event);
}

void WindowSettingsWindow::syncWithPlatformWindow()
{
    const QHarmonyJsObjectRef jsWindowRef = jsWindow();
    const bool subWindow = isSubWindow();
    const PlatformWindowSettings settings = QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &jsContext) {
            return readPlatformWindowSettings(jsContext, jsWindowRef, subWindow);
        });

    if (settings.optKeepScreenOn) {
        const QSignalBlocker blocker(m_keepScreenOnCheckBox);
        m_keepScreenOnCheckBox->setChecked(*settings.optKeepScreenOn);
    }
    if (settings.optPrivacyMode) {
        const QSignalBlocker blocker(m_privacyModeCheckBox);
        m_privacyModeCheckBox->setChecked(*settings.optPrivacyMode);
    }
    if (settings.optCornerRadius) {
        const QSignalBlocker blocker(m_cornerRadiusSpinBox);
        m_cornerRadiusSpinBox->setValue(*settings.optCornerRadius);
    }
    if (!settings.errorText.isEmpty())
        m_log->append(QStringLiteral("reading the window settings failed: %1").arg(settings.errorText));

    if (subWindow)
        setDragResizable(m_dragResizableCheckBox->isChecked());
}
//! [3]

void WindowSettingsWindow::openSubWindow()
{
    auto *subWindow = new WindowSettingsWindow(this);
    subWindow->resize(500, 400);
    subWindow->show();
}

}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    WindowSettingsWindow window;
    window.show();

    return app.exec();
}
