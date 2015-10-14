/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "widget.h"
#include "renderwindow.h"
#include <QVBoxLayout>
#include <QComboBox>
#include <QGroupBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QSplitter>
#include <QSurfaceFormat>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QDebug>

struct Version {
    const char *str;
    int major;
    int minor;
};

static struct Version versions[] = {
    { "1.0", 1, 0 },
    { "1.1", 1, 1 },
    { "1.2", 1, 2 },
    { "1.3", 1, 3 },
    { "1.4", 1, 4 },
    { "1.5", 1, 5 },
    { "2.0", 2, 0 },
    { "2.1", 2, 1 },
    { "3.0", 3, 0 },
    { "3.1", 3, 1 },
    { "3.2", 3, 2 },
    { "3.3", 3, 3 },
    { "4.0", 4, 0 },
    { "4.1", 4, 1 },
    { "4.2", 4, 2 },
    { "4.3", 4, 3 },
    { "4.4", 4, 4 },
    { "4.5", 4, 5 }
};

struct Profile {
    const char *str;
    QSurfaceFormat::OpenGLContextProfile profile;
};

static struct Profile profiles[] = {
    { "none", QSurfaceFormat::NoProfile },
    { "core", QSurfaceFormat::CoreProfile },
    { "compatibility", QSurfaceFormat::CompatibilityProfile }
};

struct Option {
    const char *str;
    QSurfaceFormat::FormatOption option;
};

static struct Option options[] = {
    { "deprecated functions (not forward compatible)", QSurfaceFormat::DeprecatedFunctions },
    { "debug context", QSurfaceFormat::DebugContext },
    { "stereo buffers", QSurfaceFormat::StereoBuffers },
    // This is not a QSurfaceFormat option but is helpful to determine if the driver
    // allows compiling old-style shaders with core profile.
    { "force version 110 shaders", QSurfaceFormat::FormatOption(0) }
};

struct Renderable {
    const char *str;
    QSurfaceFormat::RenderableType renderable;
};

static struct Renderable renderables[] = {
    { "default", QSurfaceFormat::DefaultRenderableType },
    { "OpenGL", QSurfaceFormat::OpenGL },
    { "OpenGL ES", QSurfaceFormat::OpenGLES }
};

void Widget::addVersions(QLayout *layout)
{
    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->setSpacing(20);
    QLabel *label = new QLabel(tr("Context &version: "));
    hbox->addWidget(label);
    m_version = new QComboBox;
    m_version->setMinimumWidth(60);
    label->setBuddy(m_version);
    hbox->addWidget(m_version);
    for (size_t i = 0; i < sizeof(versions) / sizeof(Version); ++i) {
        m_version->addItem(QString::fromLatin1(versions[i].str));
        if (versions[i].major == 2 && versions[i].minor == 0)
            m_version->setCurrentIndex(m_version->count() - 1);
    }

    QPushButton *btn = new QPushButton(tr("Create context"));
    connect(btn, &QPushButton::clicked, this, &Widget::start);
    btn->setMinimumSize(120, 40);
    hbox->addWidget(btn);

    layout->addItem(hbox);
}

void Widget::addProfiles(QLayout *layout)
{
    QGroupBox *groupBox = new QGroupBox(tr("Profile"));
    QVBoxLayout *vbox = new QVBoxLayout;
    for (size_t i = 0; i < sizeof(profiles) / sizeof(Profile); ++i)
        vbox->addWidget(new QRadioButton(QString::fromLatin1(profiles[i].str)));
    static_cast<QRadioButton *>(vbox->itemAt(0)->widget())->setChecked(true);
    groupBox->setLayout(vbox);
    layout->addWidget(groupBox);
    m_profiles = vbox;
}

void Widget::addOptions(QLayout *layout)
{
    QGroupBox *groupBox = new QGroupBox(tr("Options"));
    QVBoxLayout *vbox = new QVBoxLayout;
    for (size_t i = 0; i < sizeof(options) / sizeof(Option); ++i)
        vbox->addWidget(new QCheckBox(QString::fromLatin1(options[i].str)));
    groupBox->setLayout(vbox);
    layout->addWidget(groupBox);
    m_options = vbox;
}

void Widget::addRenderableTypes(QLayout *layout)
{
    QGroupBox *groupBox = new QGroupBox(tr("Renderable type"));
    QVBoxLayout *vbox = new QVBoxLayout;
    for (size_t i = 0; i < sizeof(renderables) / sizeof(Renderable); ++i)
        vbox->addWidget(new QRadioButton(QString::fromLatin1(renderables[i].str)));
    static_cast<QRadioButton *>(vbox->itemAt(0)->widget())->setChecked(true);
    groupBox->setLayout(vbox);
    layout->addWidget(groupBox);
    m_renderables = vbox;
}

void Widget::addRenderWindow()
{
    m_renderWindowLayout->addWidget(m_renderWindowContainer);
}

static QWidget *widgetWithLayout(QLayout *layout)
{
    QWidget *w = new QWidget;
    w->setLayout(layout);
    return w;
}

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout;
    QSplitter *vsplit = new QSplitter(Qt::Vertical);
    layout->addWidget(vsplit);

    QSplitter *hsplit = new QSplitter;

    QVBoxLayout *settingsLayout = new QVBoxLayout;
    addVersions(settingsLayout);
    addProfiles(settingsLayout);
    addOptions(settingsLayout);
    addRenderableTypes(settingsLayout);
    hsplit->addWidget(widgetWithLayout(settingsLayout));

    QVBoxLayout *outputLayout = new QVBoxLayout;
    m_output = new QTextEdit;
    m_output->setReadOnly(true);
    outputLayout->addWidget(m_output);
    m_extensions = new QTextEdit;
    m_extensions->setReadOnly(true);
    outputLayout->addWidget(m_extensions);
    hsplit->addWidget(widgetWithLayout(outputLayout));

    hsplit->setStretchFactor(0, 4);
    hsplit->setStretchFactor(1, 6);
    vsplit->addWidget(hsplit);

    m_renderWindowLayout = new QVBoxLayout;
    vsplit->addWidget(widgetWithLayout(m_renderWindowLayout));
    vsplit->setStretchFactor(1, 5);

    m_renderWindowContainer = new QWidget;
    addRenderWindow();

    setLayout(layout);
}

void Widget::start()
{
    QSurfaceFormat fmt;

    int idx = m_version->currentIndex();
    if (idx < 0)
        return;
    fmt.setVersion(versions[idx].major, versions[idx].minor);

    for (size_t i = 0; i < sizeof(profiles) / sizeof(Profile); ++i)
        if (static_cast<QRadioButton *>(m_profiles->itemAt(int(i))->widget())->isChecked()) {
            fmt.setProfile(profiles[i].profile);
            break;
        }

    bool forceGLSL110 = false;
    for (size_t i = 0; i < sizeof(options) / sizeof(Option); ++i)
        if (static_cast<QCheckBox *>(m_options->itemAt(int(i))->widget())->isChecked()) {
            if (options[i].option)
                fmt.setOption(options[i].option);
            else if (i == 3)
                forceGLSL110 = true;
        }

    for (size_t i = 0; i < sizeof(renderables) / sizeof(Renderable); ++i)
        if (static_cast<QRadioButton *>(m_renderables->itemAt(int(i))->widget())->isChecked()) {
            fmt.setRenderableType(renderables[i].renderable);
            break;
        }

    // The example rendering will need a depth buffer.
    fmt.setDepthBufferSize(16);

    m_output->clear();
    m_extensions->clear();
    qDebug() << "Requesting surface format" << fmt;

    m_renderWindowLayout->removeWidget(m_renderWindowContainer);
    delete m_renderWindowContainer;

    RenderWindow *renderWindow = new RenderWindow(fmt);
    if (!renderWindow->context()) {
        m_output->append(tr("Failed to create context"));
        delete renderWindow;
        m_renderWindowContainer = new QWidget;
        addRenderWindow();
        return;
    }
    m_surface = renderWindow;

    renderWindow->setForceGLSL110(forceGLSL110);
    connect(renderWindow, &RenderWindow::ready, this, &Widget::renderWindowReady);
    connect(renderWindow, &RenderWindow::error, this, &Widget::renderWindowError);

    m_renderWindowContainer = QWidget::createWindowContainer(renderWindow);
    addRenderWindow();
}

void Widget::printFormat(const QSurfaceFormat &format)
{
    m_output->append(tr("OpenGL version: %1.%2").arg(format.majorVersion()).arg(format.minorVersion()));

    for (size_t i = 0; i < sizeof(profiles) / sizeof(Profile); ++i)
        if (profiles[i].profile == format.profile()) {
            m_output->append(tr("Profile: %1").arg(QString::fromLatin1(profiles[i].str)));
            break;
        }

    QString opts;
    for (size_t i = 0; i < sizeof(options) / sizeof(Option); ++i)
        if (format.testOption(options[i].option))
            opts += QString::fromLatin1(options[i].str) + QLatin1Char(' ');
    m_output->append(tr("Options: %1").arg(opts));

    for (size_t i = 0; i < sizeof(renderables) / sizeof(Renderable); ++i)
        if (renderables[i].renderable == format.renderableType()) {
            m_output->append(tr("Renderable type: %1").arg(QString::fromLatin1(renderables[i].str)));
            break;
        }

    m_output->append(tr("Depth buffer size: %1").arg(QString::number(format.depthBufferSize())));
    m_output->append(tr("Stencil buffer size: %1").arg(QString::number(format.stencilBufferSize())));
    m_output->append(tr("Samples: %1").arg(QString::number(format.samples())));
    m_output->append(tr("Red buffer size: %1").arg(QString::number(format.redBufferSize())));
    m_output->append(tr("Green buffer size: %1").arg(QString::number(format.greenBufferSize())));
    m_output->append(tr("Blue buffer size: %1").arg(QString::number(format.blueBufferSize())));
    m_output->append(tr("Alpha buffer size: %1").arg(QString::number(format.alphaBufferSize())));
    m_output->append(tr("Swap interval: %1").arg(QString::number(format.swapInterval())));
}

void Widget::renderWindowReady()
{
    QOpenGLContext *context = QOpenGLContext::currentContext();
    Q_ASSERT(context);

    QString vendor, renderer, version, glslVersion;
    const GLubyte *p;
    QOpenGLFunctions *f = context->functions();
    if ((p = f->glGetString(GL_VENDOR)))
        vendor = QString::fromLatin1(reinterpret_cast<const char *>(p));
    if ((p = f->glGetString(GL_RENDERER)))
        renderer = QString::fromLatin1(reinterpret_cast<const char *>(p));
    if ((p = f->glGetString(GL_VERSION)))
        version = QString::fromLatin1(reinterpret_cast<const char *>(p));
    if ((p = f->glGetString(GL_SHADING_LANGUAGE_VERSION)))
        glslVersion = QString::fromLatin1(reinterpret_cast<const char *>(p));

    m_output->append(tr("*** Context information ***"));
    m_output->append(tr("Vendor: %1").arg(vendor));
    m_output->append(tr("Renderer: %1").arg(renderer));
    m_output->append(tr("OpenGL version: %1").arg(version));
    m_output->append(tr("GLSL version: %1").arg(glslVersion));

    m_output->append(tr("\n*** QSurfaceFormat from context ***"));
    printFormat(context->format());

    m_output->append(tr("\n*** QSurfaceFormat from window surface ***"));
    printFormat(m_surface->format());

    m_output->append(tr("\n*** Qt build information ***"));
    const char *gltype[] = { "Desktop", "GLES 2", "GLES 1" };
    m_output->append(tr("Qt OpenGL configuration: %1")
                     .arg(QString::fromLatin1(gltype[QOpenGLContext::openGLModuleType()])));
    m_output->append(tr("Qt OpenGL library handle: %1")
                     .arg(QString::number(qintptr(QOpenGLContext::openGLModuleHandle()), 16)));

    QList<QByteArray> extensionList = context->extensions().toList();
    std::sort(extensionList.begin(), extensionList.end());
    m_extensions->append(tr("Found %1 extensions:").arg(extensionList.count()));
    Q_FOREACH (const QByteArray &ext, extensionList)
        m_extensions->append(QString::fromLatin1(ext));

    m_output->moveCursor(QTextCursor::Start);
    m_extensions->moveCursor(QTextCursor::Start);
}

void Widget::renderWindowError(const QString &msg)
{
    m_output->append(tr("An error has occurred:\n%1").arg(msg));
}
