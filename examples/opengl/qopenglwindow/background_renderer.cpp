/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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
        if (!m_program->addShader(m_vertex_shader.data())) {
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
            if (!m_program->addShader(m_fragment_shader.data())) {
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
                m_program->removeShader(m_fragment_shader.data());
                m_fragment_shader.reset(nullptr);
            }
        }
    } else {
        m_recompile_shaders = true;
        if (m_program) {
            m_program->removeShader(m_fragment_shader.data());
            m_fragment_shader.reset(nullptr);
        }
    }
}
