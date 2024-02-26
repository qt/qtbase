// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QEvent>
#include <QtWidgets/qwidget.h>

#include <QtGui/qevent.h>
#include <QtCore/qobject.h>
#include <QtCore/qregularexpression.h>
#include <QtGui/qpainter.h>
#include <QtGui/qrasterwindow.h>
#include <QtGui/qscreen.h>
#include <QtGui/qwindow.h>
#include <QtGui/qguiapplication.h>
#include <QtWidgets/qlineedit.h>
#include <QApplication>
#include <QDialog>
#include <QSysInfo>

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <memory>
#include <sstream>
#include <vector>

class TestWidget : public QDialog
{
    Q_OBJECT
};


class TestWindow : public QRasterWindow
{
    Q_OBJECT

public:
    void setBackgroundColor(int r, int g, int b)
    {
        m_backgroundColor = QColor::fromRgb(r, g, b);
        update();
    }

private:
    void closeEvent(QCloseEvent *ev) override
    {
        Q_UNUSED(ev);
        delete this;
    }

    void keyPressEvent(QKeyEvent *event) final
    {
        auto data = emscripten::val::object();
        data.set("type", emscripten::val("keyPress"));
        data.set("windowId", emscripten::val(winId()));
        data.set("windowTitle", emscripten::val(title().toStdString()));
        data.set("key", emscripten::val(event->text().toStdString()));
        emscripten::val::global("window")["testSupport"].call<void>("reportEvent", std::move(data));
    }

    void keyReleaseEvent(QKeyEvent *event) final
    {
        auto data = emscripten::val::object();
        data.set("type", emscripten::val("keyRelease"));
        data.set("windowId", emscripten::val(winId()));
        data.set("windowTitle", emscripten::val(title().toStdString()));
        data.set("key", emscripten::val(event->text().toStdString()));
        emscripten::val::global("window")["testSupport"].call<void>("reportEvent", std::move(data));
    }

    void paintEvent(QPaintEvent *e) final
    {
        QPainter painter(this);
        painter.fillRect(e->rect(), m_backgroundColor);
    }

    QColor m_backgroundColor = Qt::white;
};

namespace {
TestWindow *findWindowByTitle(const std::string &title)
{
    auto windows = qGuiApp->allWindows();
    auto window_it = std::find_if(windows.begin(), windows.end(), [&title](QWindow *window) {
        return window->title() == QString::fromLatin1(title);
    });
    return window_it == windows.end() ? nullptr : static_cast<TestWindow *>(*window_it);
}

class WidgetStorage
{
private:
    ~WidgetStorage()
    {
    }
public:
    static WidgetStorage *getInstance()
    {
        if (!s_instance)
        {
            s_instance = new WidgetStorage();
        }
        return s_instance;
    }
    static void clearInstance()
    {
        delete s_instance;
        s_instance = nullptr;
    }

    TestWidget *findWidget(const std::string &name)
    {
        auto it = m_widgets.find(name);
        if (it != m_widgets.end())
            return it->second.get();
        return nullptr;
    }

    QLineEdit *findEdit(const std::string &name)
    {
        auto it = m_lineEdits.find(name);
        if (it != m_lineEdits.end())
            return it->second;
        return nullptr;
    }

    void make(const std::string &name)
    {
        auto widget = std::make_shared<TestWidget>();
        auto *lineEdit = new QLineEdit(widget.get());

        widget->setMinimumSize(200, 200);
        widget->setMaximumSize(200, 200);
        widget->setGeometry(0, m_widgetY, 200, 200);
        m_widgetY += 200;

        lineEdit->setText("Hello world");

        m_widgets[name] = widget;
        m_lineEdits[name] = lineEdit;
    }

private:
    using TestWidgetPtr = std::shared_ptr<TestWidget>;

    static WidgetStorage               * s_instance;
    std::map<std::string, TestWidgetPtr> m_widgets;
    std::map<std::string, QLineEdit  *>  m_lineEdits;
    int                                  m_widgetY = 0;
};

WidgetStorage *WidgetStorage::s_instance = nullptr;

} // namespace

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
        << "  screen: " << screenToJSObject(*window.screen()) << ","
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

void createWidget(const std::string &name)
{
    WidgetStorage::getInstance()->make(name);
}

void setWidgetNoFocusShow(const std::string &name)
{
    auto w = WidgetStorage::getInstance()->findWidget(name);
    if (w)
        w->setAttribute(Qt::WA_ShowWithoutActivating);
}

void showWidget(const std::string &name)
{
    auto w = WidgetStorage::getInstance()->findWidget(name);
    if (w)
        w->show();
}

void hasWidgetFocus(const std::string &name)
{
    bool focus = false;
    auto le = WidgetStorage::getInstance()->findEdit(name);
    if (le)
        focus = le->hasFocus();

    emscripten::val::global("window").call<void>("hasWidgetFocusCallback",
                                                 emscripten::val(focus));
}

void activateWidget(const std::string &name)
{
    auto w = WidgetStorage::getInstance()->findWidget(name);
    if (w)
        w->activateWindow();
}

void clearWidgets()
{
    WidgetStorage::clearInstance();
}

void createWindow(int x, int y, int w, int h, const std::string &parentType, const std::string &parentId,
                  const std::string &title)
{
    QScreen *parentScreen = nullptr;
    QWindow *parentWindow = nullptr;
    if (parentType == "screen") {
        auto screens = qGuiApp->screens();
        auto screen_it = std::find_if(screens.begin(), screens.end(), [&parentId](QScreen *screen) {
            return screen->name() == QString::fromLatin1(parentId);
        });
        if (screen_it == screens.end()) {
            qWarning() << "No such screen: " << parentId;
            return;
        }
        parentScreen = *screen_it;
    } else if (parentType == "window") {
        auto windows = qGuiApp->allWindows();
        auto window_it = std::find_if(windows.begin(), windows.end(), [&parentId](QWindow *window) {
            return window->title() == QString::fromLatin1(parentId);
        });
        if (window_it == windows.end()) {
            qWarning() << "No such window: " << parentId;
            return;
        }
        parentWindow = *window_it;
        parentScreen = parentWindow->screen();
    } else {
        qWarning() << "Wrong parent type " << parentType;
        return;
    }

    auto *window = new TestWindow;

    window->setFlag(Qt::WindowTitleHint);
    window->setFlag(Qt::WindowMaximizeButtonHint);
    window->setTitle(QString::fromLatin1(title));
    window->setGeometry(x, y, w, h);
    window->setScreen(parentScreen);
    window->setParent(parentWindow);
}

void setWindowBackgroundColor(const std::string &title, int r, int g, int b)
{
    auto *window = findWindowByTitle(title);
    if (!window) {
        qWarning() << "No such window: " << title;
        return;
    }
    window->setBackgroundColor(r, g, b);
}

void setWindowVisible(int windowId, bool visible) {
    auto windows = qGuiApp->allWindows();
    auto window_it = std::find_if(windows.begin(), windows.end(), [windowId](QWindow *window) {
        return window->winId() == WId(windowId);
    });
    if (window_it == windows.end()) {
        qWarning() << "No such window: " << windowId;
        return;
    }

    (*window_it)->setVisible(visible);
}

void setWindowParent(const std::string &windowTitle, const std::string &parentTitle)
{
    QWindow *window = findWindowByTitle(windowTitle);
    if (!window) {
        qWarning() << "Window could not be found " << parentTitle;
        return;
    }
    QWindow *parent = nullptr;
    if (parentTitle != "none") {
        if ((parent = findWindowByTitle(parentTitle)) == nullptr) {
            qWarning() << "Parent window could not be found " << parentTitle;
            return;
        }
    }
    window->setParent(parent);
}

bool closeWindow(const std::string &title)
{
    QWindow *window = findWindowByTitle(title);
    return window ? window->close() : false;
}

EMSCRIPTEN_BINDINGS(qwasmwindow)
{
    emscripten::function("screenInformation", &screenInformation);
    emscripten::function("windowInformation", &windowInformation);

    emscripten::function("createWindow", &createWindow);
    emscripten::function("setWindowVisible", &setWindowVisible);
    emscripten::function("setWindowParent", &setWindowParent);
    emscripten::function("closeWindow", &closeWindow);
    emscripten::function("setWindowBackgroundColor", &setWindowBackgroundColor);

    emscripten::function("createWidget", &createWidget);
    emscripten::function("setWidgetNoFocusShow", &setWidgetNoFocusShow);
    emscripten::function("showWidget", &showWidget);
    emscripten::function("activateWidget", &activateWidget);
    emscripten::function("hasWidgetFocus", &hasWidgetFocus);
    emscripten::function("clearWidgets", &clearWidgets);
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    app.exec();
    return 0;
}

#include "tst_qwasmwindow_harness.moc"
