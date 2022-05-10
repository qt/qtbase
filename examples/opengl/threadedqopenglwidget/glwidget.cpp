// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "glwidget.h"
#include "renderer.h"

#include <qmath.h>
#include <QGuiApplication>

GLWidget::GLWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    setMinimumSize(300, 250);

    connect(this, &QOpenGLWidget::aboutToCompose, this, &GLWidget::onAboutToCompose);
    connect(this, &QOpenGLWidget::frameSwapped, this, &GLWidget::onFrameSwapped);
    connect(this, &QOpenGLWidget::aboutToResize, this, &GLWidget::onAboutToResize);
    connect(this, &QOpenGLWidget::resized, this, &GLWidget::onResized);

    m_thread = new QThread;
    m_renderer = new Renderer(this);
    m_renderer->moveToThread(m_thread);
    connect(m_thread, &QThread::finished, m_renderer, &QObject::deleteLater);

    connect(this, &GLWidget::renderRequested, m_renderer, &Renderer::render);
    connect(m_renderer, &Renderer::contextWanted, this, &GLWidget::grabContext);

    m_thread->start();
}

GLWidget::~GLWidget()
{
    m_renderer->prepareExit();
    m_thread->quit();
    m_thread->wait();
    delete m_thread;
}

void GLWidget::onAboutToCompose()
{
    // We are on the gui thread here. Composition is about to
    // begin. Wait until the render thread finishes.
    m_renderer->lockRenderer();
}

void GLWidget::onFrameSwapped()
{
    m_renderer->unlockRenderer();
    // Assuming a blocking swap, our animation is driven purely by the
    // vsync in this example.
    emit renderRequested();
}

void GLWidget::onAboutToResize()
{
    m_renderer->lockRenderer();
}

void GLWidget::onResized()
{
    m_renderer->unlockRenderer();
}

void GLWidget::grabContext()
{
    m_renderer->lockRenderer();
    QMutexLocker lock(m_renderer->grabMutex());
    context()->moveToThread(m_thread);
    m_renderer->grabCond()->wakeAll();
    m_renderer->unlockRenderer();
}
