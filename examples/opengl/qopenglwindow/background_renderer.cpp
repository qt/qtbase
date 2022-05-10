// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "background_renderer.h"

#include <qmath.h>
#include <QFileInfo>
#include <QTime>

#include <QOpenGLShaderProgram>
#include <QOpenGLContext>
#include <QOpenGLFunctions>

#include <math.h>

static const char vertex_shader[] =
    "attribute highp vec3 vertexCoord;"
    "void main() {"
    "   gl_Position = vec4(vertexCoord,1.0);"
    "}";

static const char fragment_shader[] =
    "void main() {"
    "   gl_FragColor = vec4(0.0,1.0,0.0,1.0);"
    "}";

static const float vertices[] = { -1, -1, 0,
                                  -1,  1, 0,
                                  1, -1, 0,
                                  1,  1, 0 };

FragmentToy::FragmentToy(const QString &fragmentSource, QObject *parent)
    : QObject(parent)
    , m_recompile_shaders(true)
{
    if (QFile::exists(fragmentSource)) {
        QFileInfo info(fragmentSource);
        m_fragment_file_last_modified = info.lastModified();
        m_fragment_file = fragmentSource;
#if QT_CONFIG(filesystemwatcher)
        m_watcher.addPath(info.canonicalPath());
        QObject::connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &FragmentToy::fileChanged);
#endif
    }
}

FragmentToy::~FragmentToy()
    = default;

void FragmentToy::draw(const QSize &windowSize)
{
    if (!m_program)
        initializeOpenGLFunctions();

    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    if (!m_vao.isCreated())
        m_vao.create();

    QOpenGLVertexArrayObject::Binder binder(&m_vao);

    if (!m_vertex_buffer.isCreated()) {
        m_vertex_buffer.create();
        m_vertex_buffer.bind();
        m_vertex_buffer.allocate(vertices, sizeof(vertices));
        m_vertex_buffer.release();
    }

    if (!m_program) {
        m_program.reset(new QOpenGLShaderProgram);
        m_program->create();
        m_vertex_shader.reset(new QOpenGLShader(QOpenGLShader::Vertex));
        if (!m_vertex_shader->compileSourceCode(vertex_shader)) {
            qWarning() << "Failed to compile the vertex shader:" << m_vertex_shader->log();
        }
        if (!m_program->addShader(m_vertex_shader.get())) {
            qWarning() << "Failed to add vertex shader to program:" << m_program->log();
        }
    }

    if (!m_fragment_shader && m_recompile_shaders) {
        QByteArray data;
        if (m_fragment_file.size()) {
            QFile file(m_fragment_file);
            if (file.open(QIODevice::ReadOnly)) {
                data = file.readAll();
            } else {
                qWarning() << "Failed to load input file, falling back to default";
                data = QByteArray::fromRawData(fragment_shader, sizeof(fragment_shader));
            }
        } else {
            QFile qrcFile(":/background.frag");
            if (qrcFile.open(QIODevice::ReadOnly))
                data = qrcFile.readAll();
            else
                data = QByteArray::fromRawData(fragment_shader, sizeof(fragment_shader));
        }
        if (data.size()) {
            m_fragment_shader.reset(new QOpenGLShader(QOpenGLShader::Fragment));
            if (!m_fragment_shader->compileSourceCode(data)) {
                qWarning() << "Failed to compile fragment shader:" << m_fragment_shader->log();
                m_fragment_shader.reset(nullptr);
            }
        } else {
            qWarning() << "Unknown error, no fragment shader";
        }

        if (m_fragment_shader) {
            if (!m_program->addShader(m_fragment_shader.get())) {
                qWarning() << "Failed to add fragment shader to program:" << m_program->log();
            }
        }
    }

    if (m_recompile_shaders) {
        m_recompile_shaders = false;

        if (m_program->link()) {
            m_vertex_coord_pos = m_program->attributeLocation("vertexCoord");
        } else {
            qWarning() << "Failed to link shader program" << m_program->log();
        }

    }

    if (!m_program->isLinked())
        return;

    m_program->bind();

    m_vertex_buffer.bind();
    m_program->setAttributeBuffer("vertexCoord", GL_FLOAT, 0, 3, 0);
    m_program->enableAttributeArray("vertexCoord");
    m_vertex_buffer.release();

    m_program->setUniformValue("currentTime", (uint) QDateTime::currentMSecsSinceEpoch());
    m_program->setUniformValue("windowSize", windowSize);

    QOpenGLContext::currentContext()->functions()->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    m_program->release();
}

void FragmentToy::fileChanged(const QString &path)
{
    Q_UNUSED(path);
    if (QFile::exists(m_fragment_file)) {
        QFileInfo fragment_source(m_fragment_file);
        if (fragment_source.lastModified() > m_fragment_file_last_modified) {
            m_fragment_file_last_modified = fragment_source.lastModified();
            m_recompile_shaders = true;
            if (m_program) {
                m_program->removeShader(m_fragment_shader.get());
                m_fragment_shader.reset(nullptr);
            }
        }
    } else {
        m_recompile_shaders = true;
        if (m_program) {
            m_program->removeShader(m_fragment_shader.get());
            m_fragment_shader.reset(nullptr);
        }
    }
}
