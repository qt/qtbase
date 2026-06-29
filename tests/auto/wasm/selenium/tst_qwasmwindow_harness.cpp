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
#include <QTreeView>
#include <QFileSystemModel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QToolTip>
#include <QCheckBox>
#include <QRadioButton>
#include <QPushButton>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QLabel>

#include <QOpenGLWindow>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLContext>
#include <QOffscreenSurface>

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <memory>
#include <sstream>
#include <vector>

void QWasmAccessibilityEnable();

// Our dialog to test two things
// 1) Focus logic
// 2) spinbox context menu
class TestWidget : public QDialog
{
    Q_OBJECT
};

// We override to be able to test that the contextMenu
// calls popup and not exec. Calling exec locks the
// test.
class TestSpinBox : public QSpinBox
{
    Q_OBJECT

public:
    TestSpinBox(QWidget *parent = nullptr) : QSpinBox(parent) { }

    void ShowContextMenu()
    {
        QContextMenuEvent event(QContextMenuEvent::Reason::Mouse, QPoint(0, geometry().bottom()),
                                mapToGlobal(QPoint(0, geometry().bottom())), Qt::NoModifier);

        contextMenuEvent(&event);
    }
};

// Baseclass for our windows, openglwindow and raster window
class TestWindowBase
{
public:
    virtual ~TestWindowBase() {}
    virtual void setBackgroundColor(int r, int g, int b) = 0;
    virtual void setVisible(bool visible) = 0;
    virtual void setParent(QWindow *parent) = 0;
    virtual bool close() = 0;
    virtual QWindow *qWindow() = 0;
    virtual void opengl_color_at_0_0(int *r, int *g, int *b) = 0;
};

class TestWindow : public QRasterWindow, public TestWindowBase
{
    Q_OBJECT

public:
    virtual void setBackgroundColor(int r, int g, int b) override final
    {
        m_backgroundColor = QColor::fromRgb(r, g, b);
        update();
    }
    virtual void setVisible(bool visible) override final
    {
        QRasterWindow::setVisible(visible);
    }
    virtual void setParent(QWindow *parent) override final
    {
        QRasterWindow::setParent(parent);
    }
    virtual bool close() override final
    {
        return QRasterWindow::close();
    }
    virtual QWindow *qWindow() override final
    {
        return static_cast<QRasterWindow *>(this);
    }
    virtual void opengl_color_at_0_0(int *r, int *g, int *b) override final
    {
        *r = 0;
        *g = 0;
        *b = 0;
    }

private:
    void closeEvent(QCloseEvent *ev) override final
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

class ContextGuard
{
public:
    ContextGuard(QOpenGLContext *context, QSurface *surface) : m_context(context)
    {
        m_contextMutex.lock();
        m_context->makeCurrent(surface);
    }

    ~ContextGuard()
    {
        m_context->doneCurrent();
        m_contextMutex.unlock();
    }

private:
    QOpenGLContext *m_context = nullptr;
    static std::mutex m_contextMutex;
};

std::mutex ContextGuard::m_contextMutex;

class TestOpenGLWindow : public QWindow, QOpenGLFunctions, public TestWindowBase
{
    Q_OBJECT

public:
    TestOpenGLWindow()
    {
        setSurfaceType(OpenGLSurface);
        create();

        //
        // Create the texture in the share context
        //
        m_shareContext = std::make_shared<QOpenGLContext>();
        m_shareContext->create();

        {
            ContextGuard guard(m_shareContext.get(), this);
            initializeOpenGLFunctions();

            m_shaderProgram = std::make_shared<QOpenGLShaderProgram>();

            if (!m_shaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/vshader.glsl")
                || !m_shaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                                             ":/fshader.glsl")
                || !m_shaderProgram->link() || !m_shaderProgram->bind()) {

                qDebug() << " Build problem";
                qDebug() << "Log " << m_shaderProgram->log();

                m_shaderProgram = nullptr;
            } else {
                m_shaderProgram->setUniformValue("texture", 0);
            }

            //
            // Texture
            //
            glGenTextures(1, &m_TextureId);
            glBindTexture(GL_TEXTURE_2D, m_TextureId);

            uint8_t pixel[4] = { 255, 255, 255, 128 };
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            const GLfloat triangleData[] = { -1.0, -1.0, 0.0,  0.5, 0.5, 1.0, -1.0, 0.0,
                                             0.5,  0.5,  -1.0, 1.0, 0.0, 0.5, 0.5 };
            const GLushort indices[] = { 0, 1, 2 };

            glGenBuffers(1, &m_vertexBufferId);
            glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferId);
            glBufferData(GL_ARRAY_BUFFER, sizeof(float[5]) * 3, &triangleData, GL_STATIC_DRAW);

            glGenBuffers(1, &m_indexBufferId);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBufferId);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * 3, indices, GL_STATIC_DRAW);

            glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferId);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float[5]), 0);

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float[5]), (void *)(12));
        }

        //
        // We will use the texture in this context
        //
        m_context = std::make_shared<QOpenGLContext>();
        m_context->setShareContext(m_shareContext.get());
        m_context->create();

        {
            ContextGuard guard(m_context.get(), this);
            initializeOpenGLFunctions();

            glBindTexture(GL_TEXTURE_2D, m_TextureId);
            glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferId);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBufferId);
            m_shaderProgram->bind();

            // Tell OpenGL programmable pipeline how to locate vertex position data
            const int vertexLocation = m_shaderProgram->attributeLocation("a_position");
            m_shaderProgram->enableAttributeArray(vertexLocation);
            m_shaderProgram->setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, sizeof(float[5]));

            // Tell OpenGL programmable pipeline how to locate vertex texture coordinate data
            const int texcoordLocation = m_shaderProgram->attributeLocation("a_texcoord");
            m_shaderProgram->enableAttributeArray(texcoordLocation);
            m_shaderProgram->setAttributeBuffer(texcoordLocation, GL_FLOAT, sizeof(float[3]), 2,
                                                sizeof(float[5]));
        }

        renderLater();
    }

public:
    virtual void setBackgroundColor(int red, int green, int blue) override final
    {
        {
            ContextGuard guard(m_shareContext.get(), this);

            //
            // Update texture
            //
            const uint8_t pixel[4] = { (uint8_t)red, (uint8_t)green, (uint8_t)blue, 128 };
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        }

        renderLater();
    }
    virtual void setVisible(bool visible) override final { QWindow::setVisible(visible); }
    virtual void setParent(QWindow *parent) override final { QWindow::setParent(parent); }
    virtual bool close() override final { return QWindow::close(); }
    virtual QWindow *qWindow() override final { return static_cast<QWindow *>(this); }
    virtual void opengl_color_at_0_0(int *r, int *g, int *b) override final
    {
        ContextGuard guard(m_context.get(), this);

        *r = m_rgba[0];
        *g = m_rgba[1];
        *b = m_rgba[2];
    }

private:
    bool event(QEvent *event) override final
    {
        switch (event->type()) {
        case QEvent::UpdateRequest:
            renderNow();
            return true;
        default:
            return QWindow::event(event);
        }
    }

    void exposeEvent(QExposeEvent *event) override final
    {
        Q_UNUSED(event);

        if (isExposed())
            renderNow();
    }

    void closeEvent(QCloseEvent *ev) override final
    {
        Q_UNUSED(ev);
        delete this;
    }

    void keyPressEvent(QKeyEvent *event) override final
    {
        auto data = emscripten::val::object();
        data.set("type", emscripten::val("keyPress"));
        data.set("windowId", emscripten::val(winId()));
        data.set("windowTitle", emscripten::val(title().toStdString()));
        data.set("key", emscripten::val(event->text().toStdString()));
        emscripten::val::global("window")["testSupport"].call<void>("reportEvent", std::move(data));
    }

    void keyReleaseEvent(QKeyEvent *event) override final
    {
        auto data = emscripten::val::object();
        data.set("type", emscripten::val("keyRelease"));
        data.set("windowId", emscripten::val(winId()));
        data.set("windowTitle", emscripten::val(title().toStdString()));
        data.set("key", emscripten::val(event->text().toStdString()));
        emscripten::val::global("window")["testSupport"].call<void>("reportEvent", std::move(data));
    }
    void renderLater() { requestUpdate(); }
    void renderNow()
    {
        qDebug() << " Render now";
        ContextGuard guard(m_context.get(), this);
        const auto sz = size();
        glViewport(0, 0, sz.width(), sz.height());

        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw triangle using indices from VBO
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, nullptr);

        glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, m_rgba);
        m_context->swapBuffers(this);
    }

private:
    std::shared_ptr<QOpenGLShaderProgram> m_shaderProgram;
    GLuint m_vertexBufferId = 0;
    GLuint m_indexBufferId = 0;
    GLuint m_TextureId = 0;

    std::shared_ptr<QOpenGLContext> m_shareContext;
    std::shared_ptr<QOpenGLContext> m_context;
    uint8_t m_rgba[4]; // Color at location(0, 0)
};

namespace {
TestWindowBase *findWindowByTitle(const std::string &title)
{
    auto windows = qGuiApp->allWindows();
    auto window_it = std::find_if(windows.begin(), windows.end(), [&title](QWindow *window) {
        return window->title() == QString::fromLatin1(title);
    });
    return window_it == windows.end() ? nullptr : dynamic_cast<TestWindowBase *>(*window_it);
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
            s_instance = new WidgetStorage();

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

    TestSpinBox *findSpinBox(const std::string &name)
    {
        auto it = m_spinBoxes.find(name);
        if (it != m_spinBoxes.end())
            return it->second;
        return nullptr;
    }

    void make(const std::string &name)
    {
        auto widget = std::make_shared<TestWidget>();

        widget->setWindowTitle("Dialog");
        auto spinBox = new TestSpinBox(widget.get());
        spinBox->setToolTip(QString(u"A ToolTip"));

        widget->setMinimumSize(200, 200);
        widget->setMaximumSize(200, 200);
        widget->setGeometry(0, m_widgetY, 200, 200);
        m_widgetY += 200;

        m_widgets[name] = widget;
        m_spinBoxes[name] = spinBox;
    }
    void showContextMenu(const std::string &name)
    {
        TestSpinBox *spinBox = findSpinBox(name);
        if (spinBox)
            spinBox->ShowContextMenu();
    }
    void showToolTip(const std::string &name)
    {
        TestSpinBox *spinBox = findSpinBox(name);
        if (spinBox)
            QToolTip::showText(spinBox->mapToGlobal( QPoint( 0, 0 ) ), spinBox->toolTip());
    }
    void makeNative(const std::string &name)
    {
        auto widget = std::make_shared<TestWidget>();
        widget->setWindowTitle("Dialog");
        auto spinBox = new TestSpinBox(widget.get());
        spinBox->setToolTip(QString(u"A ToolTip"));

        widget->setMinimumSize(200, 200);
        widget->setMaximumSize(200, 200);
        widget->setGeometry(0, m_widgetY, 200, 200);
        m_widgetY += 200;

        m_widgets[name] = widget;
        m_spinBoxes[name] = spinBox;

        QFileSystemModel *model = new QFileSystemModel;
        model->setRootPath(QDir::currentPath());

        auto *scrollArea = new QScrollArea();
        auto *layout = new QVBoxLayout(widget.get());
        auto *treeView = new QTreeView(scrollArea);
        treeView->setModel(model);

        layout->addWidget(spinBox);
        layout->addWidget(scrollArea);

        treeView->setAttribute(Qt::WA_NativeWindow);
        scrollArea->setAttribute(Qt::WA_NativeWindow);
        spinBox->setAttribute(Qt::WA_NativeWindow);
        widget->setAttribute(Qt::WA_NativeWindow);
    }

    void makeNativeA11yButtonWidgets(const std::string &name)
    {
        QWasmAccessibilityEnable();
        auto widget = std::make_shared<TestWidget>();
        widget->setWindowTitle("Dialog");

        m_widgets[name] = widget;

        auto *layout = new QVBoxLayout(widget.get());
        auto *checkBoxA1 = new QCheckBox("CheckBoxA1", widget.get());
        auto *checkBoxA2 = new QCheckBox("CheckBoxA2", widget.get());
        auto *radioB1 = new QRadioButton("RadioB1", widget.get());
        auto *radioB2 = new QRadioButton("RadioB2", widget.get());
        auto *pushC1 = new QPushButton("PushC1", widget.get());
        auto *pushC2 = new QPushButton("PushC2", widget.get());

        checkBoxA1->setAccessibleIdentifier("CheckBoxA1");
        checkBoxA2->setAccessibleIdentifier("CheckBoxA2");
        radioB1->setAccessibleIdentifier("RadioB1");
        radioB2->setAccessibleIdentifier("RadioB2");
        pushC1->setAccessibleIdentifier("PushC1");
        pushC2->setAccessibleIdentifier("PushC2");

        checkBoxA1->setAccessibleDescription("CheckBoxA1 - Description");
        checkBoxA2->setAccessibleDescription("CheckBoxA2 - Description");
        radioB1->setAccessibleDescription("RadioB1 - Description");
        radioB2->setAccessibleDescription("RadioB2 - Description");
        pushC1->setAccessibleDescription("PushC1 - Description");
        pushC2->setAccessibleDescription("PushC2 - Description");

        QObject::connect(pushC1, &QPushButton::clicked, widget.get(), [pushC1]() { pushC1->setText("PushC1 - Clicked"); });
        QObject::connect(pushC2, &QPushButton::clicked, widget.get(), [pushC2]() { pushC2->setText("PushC2 - Clicked"); });

        layout->addWidget(checkBoxA1);
        layout->addWidget(checkBoxA2);
        layout->addWidget(radioB1);
        layout->addWidget(radioB2);
        layout->addWidget(pushC1);
        layout->addWidget(pushC2);
    }

    void makeNativeA11yTextWidgets(const std::string &name)
    {
        QWasmAccessibilityEnable();
        auto widget = std::make_shared<TestWidget>();
        widget->setWindowTitle("Dialog");

        m_widgets[name] = widget;

        auto *layout = new QVBoxLayout(widget.get());
        auto *lineEditA1 = new QLineEdit("LineEditA1", widget.get());
        auto *lineEditA2 = new QLineEdit("LineEditA2", widget.get());
        auto *lineEditA3 = new QLineEdit("LineEditA3", widget.get());
        auto *lineEditA4 = new QLineEdit("LineEditA4", widget.get());
        auto *lineEditA5 = new QLineEdit("LineEditA5", widget.get());

        auto *textEditB1 = new QTextEdit("TextEditB1", widget.get());
        auto *textEditB2 = new QTextEdit("TextEditB2", widget.get());

        auto *plainTextEditC1 = new QPlainTextEdit("PlainTextEditC1", widget.get());
        auto *plainTextEditC2 = new QPlainTextEdit("PlainTextEditC2", widget.get());

        auto *labelD1 = new QLabel("LabelD1", widget.get());

        lineEditA1->setAccessibleIdentifier("LineEditA1");
        lineEditA2->setAccessibleIdentifier("LineEditA2");
        lineEditA3->setAccessibleIdentifier("LineEditA3");
        lineEditA4->setAccessibleIdentifier("LineEditA4");
        lineEditA5->setAccessibleIdentifier("LineEditA5");
        textEditB1->setAccessibleIdentifier("TextEditB1");
        textEditB2->setAccessibleIdentifier("TextEditB2");
        plainTextEditC1->setAccessibleIdentifier("PlainTextEditC1");
        plainTextEditC2->setAccessibleIdentifier("PlainTextEditC2");
        labelD1->setAccessibleIdentifier("LabelD1");

        lineEditA1->setAccessibleDescription("LineEditA1 - Description");
        lineEditA2->setAccessibleDescription("LineEditA2 - Description");
        lineEditA3->setAccessibleDescription("LineEditA3 - Description");
        lineEditA4->setAccessibleDescription("LineEditA4 - Description");
        lineEditA5->setAccessibleDescription("LineEditA5 - Description");
        textEditB1->setAccessibleDescription("TextEditB1 - Description");
        textEditB2->setAccessibleDescription("TextEditB2 - Description");
        plainTextEditC1->setAccessibleDescription("PlainTextEditC1 - Description");
        plainTextEditC2->setAccessibleDescription("PlainTextEditC2 - Description");
        labelD1->setAccessibleDescription("LabelD1 - Description");

        lineEditA1->setText("LineEditA1 - Text");
        lineEditA2->setText("LineEditA2 - Text");
        lineEditA3->setText("LineEditA3 - Text");
        lineEditA4->setText("LineEditA4 - Text");
        lineEditA5->setText("LineEditA5 - Text");
        textEditB1->setText("TextEditB1 - Text");
        textEditB2->setText("TextEditB2 - Text");
        plainTextEditC1->setPlainText("PlainTextEditC1 - Text");
        plainTextEditC2->setPlainText("PlainTextEditC2 - Text");
        labelD1->setText("LabelD1 - Text");

        lineEditA2->setReadOnly(true);
        textEditB2->setReadOnly(true);
        plainTextEditC2->setReadOnly(true);

        lineEditA3->setEchoMode(QLineEdit::Password);
        lineEditA4->setEchoMode(QLineEdit::NoEcho);
        lineEditA5->setEchoMode(QLineEdit::PasswordEchoOnEdit);

        layout->addWidget(lineEditA1);
        layout->addWidget(lineEditA2);
        layout->addWidget(lineEditA3);
        layout->addWidget(lineEditA4);
        layout->addWidget(lineEditA5);
        layout->addWidget(textEditB1);
        layout->addWidget(textEditB2);
        layout->addWidget(plainTextEditC1);
        layout->addWidget(plainTextEditC2);
        layout->addWidget(labelD1);
    }

    bool closeWidget(const std::string &name)
    {
        TestWidget *widget = findWidget(name);
        if (!widget)
            return false;

        widget->close();
        return true;
    }

private:
    using TestWidgetPtr = std::shared_ptr<TestWidget>;

    static WidgetStorage                 *s_instance;
    std::map<std::string, TestWidgetPtr>  m_widgets;
    std::map<std::string, TestSpinBox *>  m_spinBoxes;
    int                                   m_widgetY = 0;
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


void createNativeWidget(const std::string &name)
{
    WidgetStorage::getInstance()->makeNative(name);
}

void createNativeA11yButtonWidgets(const std::string &name)
{
    WidgetStorage::getInstance()->makeNativeA11yButtonWidgets(name);
}

void createNativeA11yTextWidgets(const std::string &name)
{
    WidgetStorage::getInstance()->makeNativeA11yTextWidgets(name);
}

void showContextMenuWidget(const std::string &name)
{
    WidgetStorage::getInstance()->showContextMenu(name);
}

void showToolTipWidget(const std::string &name)
{
    WidgetStorage::getInstance()->showToolTip(name);
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
    auto spinBox = WidgetStorage::getInstance()->findSpinBox(name);
    if (spinBox)
        focus = spinBox->hasFocus();

    emscripten::val::global("window").call<void>("hasWidgetFocusCallback",
                                                 emscripten::val(focus));
}

void activateWidget(const std::string &name)
{
    auto w = WidgetStorage::getInstance()->findWidget(name);
    if (w)
        w->activateWindow();
}

bool closeWidget(const std::string &name)
{
    return WidgetStorage::getInstance()->closeWidget(name);
}

void clearWidgets()
{
    WidgetStorage::clearInstance();
}

void createWindow(int x, int y, int w, int h, const std::string &parentType, const std::string &parentId,
                  const std::string &title, bool opengl)
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
        auto testWindow = findWindowByTitle(parentId);

        if (!testWindow) {
            qWarning() << "No parent window: " << parentId;
            return;
        }
        parentWindow = testWindow->qWindow();
        parentScreen = parentWindow->screen();
    } else {
        qWarning() << "Wrong parent type " << parentType;
        return;
    }

    if (opengl) {
        qDebug() << "Making OpenGL window";
        auto window = new TestOpenGLWindow;
        window->setFlag(Qt::WindowTitleHint);
        window->setFlag(Qt::WindowMaximizeButtonHint);
        window->setTitle(QString::fromLatin1(title));
        window->setGeometry(x, y, w, h);
        window->setScreen(parentScreen);
        window->setParent(parentWindow);
    } else {
        qDebug() << "Making Raster window";
        auto window = new TestWindow;
        window->setFlag(Qt::WindowTitleHint);
        window->setFlag(Qt::WindowMaximizeButtonHint);
        window->setTitle(QString::fromLatin1(title));
        window->setGeometry(x, y, w, h);
        window->setScreen(parentScreen);
        window->setParent(parentWindow);
    }
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

void setWindowVisible(int windowId, bool visible)
{
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
    TestWindowBase *window = findWindowByTitle(windowTitle);
    if (!window) {
        qWarning() << "Window could not be found " << windowTitle;
        return;
    }
    TestWindowBase *parent = nullptr;
    if (parentTitle != "none") {
        if ((parent = findWindowByTitle(parentTitle)) == nullptr) {
            qWarning() << "Parent window could not be found " << parentTitle;
            return;
        }
    }
    window->setParent(parent ? parent->qWindow() : nullptr);
}

bool closeWindow(const std::string &title)
{
    TestWindowBase *window = findWindowByTitle(title);
    return window ? window->close() : false;
}

std::string colorToJs(int r, int g, int b)
{
    return "[{"
           "   r: "
            + std::to_string(r)
            + ","
              "   g: "
            + std::to_string(g)
            + ","
              "   b: "
            + std::to_string(b)
            + ""
              "}]";
}

void getOpenGLColorAt_0_0(const std::string &windowTitle)
{
    TestWindowBase *window = findWindowByTitle(windowTitle);
    int r = 0;
    int g = 0;
    int b = 0;

    if (!window) {
        qWarning() << "Window could not be found " << windowTitle;
    } else {
        window->opengl_color_at_0_0(&r, &g, &b);
    }

    emscripten::val::global("window").call<void>("getOpenGLColorAt_0_0Callback",
                                                 emscripten::val(colorToJs(r, g, b)));
}

// Reproduces QTBUG-145723: a QOpenGLContext must stay valid and reusable after
// doneCurrent(). The sequence makeCurrent() -> doneCurrent() -> makeCurrent()
// must succeed, and isValid() must report true after each step (in particular
// doneCurrent() must not invalidate the context). Returns a JS object with the
// outcome of each step so the test driver can assert on it.
void testContextReuse()
{
    QSurfaceFormat format;
    format.setVersion(3, 0);

    QOpenGLContext context;
    context.setFormat(format);
    const bool created = context.create();

    QOffscreenSurface surface;
    surface.setFormat(context.format());
    surface.create();

    const bool makeCurrent1 = context.makeCurrent(&surface);
    const bool validAfterMakeCurrent1 = context.isValid();

    context.doneCurrent();
    const bool validAfterDoneCurrent = context.isValid();

    const bool makeCurrent2 = context.makeCurrent(&surface);
    const bool validAfterMakeCurrent2 = context.isValid();

    context.doneCurrent();

    auto boolStr = [](bool value) { return value ? "true" : "false"; };
    std::ostringstream out;
    out << "({"
        << "created:" << boolStr(created) << ","
        << "makeCurrent1:" << boolStr(makeCurrent1) << ","
        << "validAfterMakeCurrent1:" << boolStr(validAfterMakeCurrent1) << ","
        << "validAfterDoneCurrent:" << boolStr(validAfterDoneCurrent) << ","
        << "makeCurrent2:" << boolStr(makeCurrent2) << ","
        << "validAfterMakeCurrent2:" << boolStr(validAfterMakeCurrent2)
        << "})";

    emscripten::val::global("window").call<void>("testContextReuseCallback",
                                                 emscripten::val(out.str()));
}

#if QT_CONFIG(wasm_jspi)
#  define EMSC_BIND_FUNC(name, afunction) \
      emscripten::function(name, afunction, emscripten::async())
#else
#  define EMSC_BIND_FUNC(name, afunction) \
      emscripten::function(name, afunction)
#endif

EMSCRIPTEN_BINDINGS(qwasmwindow)
{
    EMSC_BIND_FUNC("screenInformation", &screenInformation);
    EMSC_BIND_FUNC("windowInformation", &windowInformation);

    EMSC_BIND_FUNC("createWindow", &createWindow);
    EMSC_BIND_FUNC("setWindowVisible", &setWindowVisible);
    EMSC_BIND_FUNC("setWindowParent", &setWindowParent);
    EMSC_BIND_FUNC("closeWindow", &closeWindow);
    EMSC_BIND_FUNC("setWindowBackgroundColor", &setWindowBackgroundColor);

    EMSC_BIND_FUNC("getOpenGLColorAt_0_0", &getOpenGLColorAt_0_0);
    EMSC_BIND_FUNC("testContextReuse", &testContextReuse);

    EMSC_BIND_FUNC("createWidget", &createWidget);
    EMSC_BIND_FUNC("createNativeWidget", &createNativeWidget);
    EMSC_BIND_FUNC("createNativeA11yButtonWidgets", &createNativeA11yButtonWidgets);
    EMSC_BIND_FUNC("createNativeA11yTextWidgets", &createNativeA11yTextWidgets);
    EMSC_BIND_FUNC("showContextMenuWidget", &showContextMenuWidget);
    EMSC_BIND_FUNC("showToolTipWidget", &showToolTipWidget);
    EMSC_BIND_FUNC("setWidgetNoFocusShow", &setWidgetNoFocusShow);
    EMSC_BIND_FUNC("showWidget", &showWidget);
    EMSC_BIND_FUNC("closeWidget", &closeWidget);
    EMSC_BIND_FUNC("activateWidget", &activateWidget);
    EMSC_BIND_FUNC("hasWidgetFocus", &hasWidgetFocus);
    EMSC_BIND_FUNC("clearWidgets", &clearWidgets);
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    app.exec();
    return 0;
}

#include "tst_qwasmwindow_harness.moc"
