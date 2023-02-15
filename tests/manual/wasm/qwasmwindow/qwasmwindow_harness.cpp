// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QEvent>
#include <QtCore/qobject.h>
#include <QtCore/qregularexpression.h>
#include <QtGui/qscreen.h>
#include <QtGui/qwindow.h>
#include <QtGui/qguiapplication.h>

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <memory>
#include <sstream>
#include <vector>

class DeleteOnCloseWindow : public QWindow
{
    Q_OBJECT

private:
    void closeEvent(QCloseEvent *ev) override
    {
        Q_UNUSED(ev);
        delete this;
    }
};

using namespace emscripten;

std::string toJSArray(const std::vector<std::string> &elements)
{
    std::ostringstream out;
    out << "[";
    bool comma = false;
    for (const auto &element : elements) {
        out << (comma ? "," : "");
        out << element;
        comma = true;
    }
    out << "]";
    return out.str();
}

std::string toJSString(const QString &qstring)
{
    Q_ASSERT_X(([qstring]() {
                   static QRegularExpression unescapedQuoteRegex(R"re((?:^|[^\\])')re");
                   return qstring.indexOf(unescapedQuoteRegex) == -1;
               })(),
               Q_FUNC_INFO, "Unescaped single quotes found");
    return "'" + qstring.toStdString() + "'";
}

std::string rectToJSObject(const QRect &rect)
{
    std::ostringstream out;
    out << "{"
        << "  x: " << std::to_string(rect.x()) << ","
        << "  y: " << std::to_string(rect.y()) << ","
        << "  width: " << std::to_string(rect.width()) << ","
        << "  height: " << std::to_string(rect.height()) << "}";
    return out.str();
}

std::string screenToJSObject(const QScreen &screen)
{
    std::ostringstream out;
    out << "{"
        << "  name: " << toJSString(screen.name()) << ","
        << "  geometry: " << rectToJSObject(screen.geometry()) << "}";
    return out.str();
}

std::string windowToJSObject(const QWindow &window)
{
    std::ostringstream out;
    out << "{"
        << "  id: " << std::to_string(window.winId()) << ","
        << "  geometry: " << rectToJSObject(window.geometry()) << ","
        << "  frameGeometry: " << rectToJSObject(window.frameGeometry()) << ","
        << "  title: '" << window.title().toStdString() << "' }";
    return out.str();
}

void windowInformation()
{
    auto windows = qGuiApp->allWindows();

    std::vector<std::string> windowsAsJsObjects;
    windowsAsJsObjects.reserve(windows.size());
    std::transform(windows.begin(), windows.end(), std::back_inserter(windowsAsJsObjects),
                   [](const QWindow *window) { return windowToJSObject(*window); });

    emscripten::val::global("window").call<void>("windowInformationCallback",
                                                 emscripten::val(toJSArray(windowsAsJsObjects)));
}

void screenInformation()
{
    auto screens = qGuiApp->screens();

    std::vector<std::string> screensAsJsObjects;
    screensAsJsObjects.reserve(screens.size());
    std::transform(screens.begin(), screens.end(), std::back_inserter(screensAsJsObjects),
                   [](const QScreen *screen) { return screenToJSObject(*screen); });
    emscripten::val::global("window").call<void>("screenInformationCallback",
                                                 emscripten::val(toJSArray(screensAsJsObjects)));
}

void createWindow(int x, int y, int w, int h, std::string screenId, std::string title)
{
    auto screens = qGuiApp->screens();
    auto screen_it = std::find_if(screens.begin(), screens.end(), [&screenId](QScreen *screen) {
        return screen->name() == QString::fromLatin1(screenId);
    });
    if (screen_it == screens.end()) {
        qWarning() << "No such screen: " << screenId;
        return;
    }

    auto *window = new DeleteOnCloseWindow;

    window->setFlag(Qt::WindowTitleHint);
    window->setFlag(Qt::WindowMaximizeButtonHint);
    window->setTitle(QString::fromLatin1(title));
    window->setGeometry(x, y, w, h);
    window->setScreen(*screen_it);
    window->showNormal();
}

EMSCRIPTEN_BINDINGS(qwasmwindow)
{
    emscripten::function("screenInformation", &screenInformation);
    emscripten::function("windowInformation", &windowInformation);
    emscripten::function("createWindow", &createWindow);
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    app.exec();
    return 0;
}

#include "qwasmwindow_harness.moc"
