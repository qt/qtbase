// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QWindow>
#include <QtCore/qharmonyjscontext.h>
#include <QtCore/qharmonyjsobjectref.h>
#include <cstdint>
#include <napi.h>
#include <optional>

using QHarmonyApplication = QNativeInterface::QHarmonyApplication;
using QHarmonyWindow = QNativeInterface::QHarmonyWindow;

namespace {

//! [1]
struct WindowIds
{
    std::int64_t windowId = 0;
    std::optional<std::int64_t> optDisplayId;
    QString errorText;
};

WindowIds readWindowIds(QHarmonyJsContext &jsContext, const QHarmonyJsObjectRef &jsWindowRef)
{
    Napi::Env env(jsContext.env());
    Napi::Object jsWindow(env, jsWindowRef.value(jsContext));
    if (jsWindow.IsEmpty()) {
        return WindowIds{
            .errorText = QStringLiteral("the window is not shown in a window.Window"),
        };
    }

    try {
        auto getWindowPropertiesFunc = jsWindow.Get("getWindowProperties").As<Napi::Function>();
        auto windowProperties = getWindowPropertiesFunc.Call(jsWindow, {}).As<Napi::Object>();
        auto displayIdValue = windowProperties.Get("displayId");
        return WindowIds{
            .windowId = windowProperties.Get("id").As<Napi::Number>().Int64Value(),
            .optDisplayId = displayIdValue.IsNumber() ? std::optional(displayIdValue.As<Napi::Number>().Int64Value()) : std::nullopt,
        };
    } catch (const Napi::Error &error) {
        return WindowIds{
            .errorText = QString::fromUtf8(error.what()),
        };
    }
}
//! [1]

QString formatDisplayId(std::optional<std::int64_t> optDisplayId)
{
    return optDisplayId ? QString::number(*optDisplayId) : QStringLiteral("<unavailable>");
}

class WindowIdWindow : public QWidget
{
public:
    explicit WindowIdWindow(const QString &title, QWidget *parentWindow = nullptr);

    void showWindowIds();

private:
    static void openWindow(const QString &title, QWidget *parentWindow = nullptr);

    QLabel *m_idsLabel;
};

WindowIdWindow::WindowIdWindow(const QString &title, QWidget *parentWindow)
    : QWidget(parentWindow, Qt::Window)
    , m_idsLabel(new QLabel(this))
{
    setWindowTitle(title);
    m_idsLabel->setWordWrap(true);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_idsLabel);

    if (parentWindow) {
        resize(400, 300);
    } else {
        auto *openSubWindowButton = new QPushButton(QStringLiteral("New sub window"), this);
        connect(
            openSubWindowButton, &QPushButton::clicked, this,
            [this] {
                openWindow(QStringLiteral("Sub window"), this);
            });
        auto *openTopLevelWindowButton = new QPushButton(QStringLiteral("New top-level window"), this);
        connect(
            openTopLevelWindowButton, &QPushButton::clicked, this,
            [] {
                openWindow(QStringLiteral("Top-level window"));
            });
        layout->addWidget(openSubWindowButton);
        layout->addWidget(openTopLevelWindowButton);
    }

    //! [2]
    create();
    connect(windowHandle(), &QWindow::screenChanged, this, &WindowIdWindow::showWindowIds);
    //! [2]
}

//! [0]
void WindowIdWindow::showWindowIds()
{
    auto *harmonyWindow = windowHandle()->nativeInterface<QHarmonyWindow>();
    const auto jsWindowRef = harmonyWindow ? harmonyWindow->jsWindow() : QHarmonyJsObjectRef();
    const WindowIds windowIds = QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &jsContext) {
            return readWindowIds(jsContext, jsWindowRef);
        });

    if (!windowIds.errorText.isEmpty()) {
        m_idsLabel->setText(QStringLiteral("Reading the IDs failed: %1").arg(windowIds.errorText));
        return;
    }
    m_idsLabel->setText(
        QStringLiteral("Window ID: %1\nDisplay ID: %2")
            .arg(QString::number(windowIds.windowId), formatDisplayId(windowIds.optDisplayId)));
}
//! [0]

//! [3]
void WindowIdWindow::openWindow(const QString &title, QWidget *parentWindow)
{
    auto *newWindow = new WindowIdWindow(title, parentWindow);
    newWindow->setAttribute(Qt::WA_DeleteOnClose);
    newWindow->show();
    newWindow->showWindowIds();
}
//! [3]

}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    WindowIdWindow window(QStringLiteral("Native window ID example"));
    window.show();
    window.showWindowIds();

    return app.exec();
}
