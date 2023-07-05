// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QtWidgets/QtWidgets>

#include <iostream>
#include <sstream>

#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <emscripten.h>

#include <QtGui/qpa/qplatformscreen.h>

namespace {
constexpr int ExitValueImmediateReturn = 42;
constexpr int ExitValueFromExitApp = 22;

std::string screenInformation()
{
    auto screens = qGuiApp->screens();
    std::ostringstream out;
    out << "[";
    const char *separator = "";
    for (const auto &screen : screens) {
        out << separator;
        out << "[" << std::to_string(screen->geometry().x()) << ","
            << std::to_string(screen->geometry().y()) << ","
            << std::to_string(screen->geometry().width()) << ","
            << std::to_string(screen->geometry().height()) << "]";
        separator = ",";
    }
    out << "]";
    return out.str();
}

std::string logicalDpi()
{
    auto screens = qGuiApp->screens();
    std::ostringstream out;
    out << "[";
    const char *separator = "";
    for (const auto &screen : screens) {
        out << separator;
        out << "[" << std::to_string(screen->handle()->logicalDpi().first) << ", "
            << std::to_string(screen->handle()->logicalDpi().second) << "]";
        separator = ",";
    }
    out << "]";
    return out.str();
}

std::string preloadedFiles()
{
    QStringList files = QDir("/preload").entryList(QDir::Files);
    std::ostringstream out;
    out << "[";
    const char *separator = "";
    for (const auto &file : files) {
        out << separator;
        out << file.toStdString();
        separator = ",";
    }
    out << "]";
    return out.str();
}

void crash()
{
    std::abort();
}

void exitApp()
{
    emscripten_force_exit(ExitValueFromExitApp);
}

void produceOutput()
{
    fprintf(stdout, "Sample output!\n");
}

std::string retrieveArguments()
{
    auto arguments = QApplication::arguments();
    std::ostringstream out;
    out << "[";
    const char *separator = "";
    for (const auto &argument : arguments) {
        out << separator;
        out << "'" << argument.toStdString() << "'";
        separator = ",";
    }
    out << "]";
    return out.str();
}

std::string getEnvironmentVariable(std::string name) {
    return QString::fromLatin1(qgetenv(name.c_str())).toStdString();
}
} // namespace

class AppWindow : public QObject
{
    Q_OBJECT
public:
    AppWindow() : m_layout(new QVBoxLayout(&m_ui))
    {
        addWidget<QLabel>("Qt Loader integration tests");

        m_ui.setLayout(m_layout);
    }

    void show() { m_ui.show(); }

    ~AppWindow() = default;

private:
    template<class T, class... Args>
    T *addWidget(Args... args)
    {
        T *widget = new T(std::forward<Args>(args)..., &m_ui);
        m_layout->addWidget(widget);
        return widget;
    }

    QWidget m_ui;
    QVBoxLayout *m_layout;
};

int main(int argc, char **argv)
{
    QApplication application(argc, argv);
    const auto arguments = application.arguments();
    const bool exitImmediately =
            std::find(arguments.begin(), arguments.end(), QStringLiteral("--exit-immediately"))
            != arguments.end();
    if (exitImmediately)
        emscripten_force_exit(ExitValueImmediateReturn);

    const bool crashImmediately =
            std::find(arguments.begin(), arguments.end(), QStringLiteral("--crash-immediately"))
            != arguments.end();
    if (crashImmediately)
        crash();

    const bool noGui = std::find(arguments.begin(), arguments.end(), QStringLiteral("--no-gui"))
            != arguments.end();
    if (!noGui) {
        AppWindow window;
        window.show();
        return application.exec();
    }
    return application.exec();
}

EMSCRIPTEN_BINDINGS(qtLoaderIntegrationTest)
{
    emscripten::constant("EXIT_VALUE_IMMEDIATE_RETURN", ExitValueImmediateReturn);
    emscripten::constant("EXIT_VALUE_FROM_EXIT_APP", ExitValueFromExitApp);

    emscripten::function("screenInformation", &screenInformation);
    emscripten::function("logicalDpi", &logicalDpi);
    emscripten::function("preloadedFiles", &preloadedFiles);
    emscripten::function("crash", &crash);
    emscripten::function("exitApp", &exitApp);
    emscripten::function("produceOutput", &produceOutput);
    emscripten::function("retrieveArguments", &retrieveArguments);
    emscripten::function("getEnvironmentVariable", &getEnvironmentVariable);
}

#include "main.moc"
