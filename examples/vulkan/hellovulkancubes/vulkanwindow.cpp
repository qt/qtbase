/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "vulkanwindow.h"
#include "renderer.h"
#include <QMouseEvent>
#include <QKeyEvent>

VulkanWindow::VulkanWindow(bool dbg)
    : m_debug(dbg)
{
}

QVulkanWindowRenderer *VulkanWindow::createRenderer()
{
    m_renderer = new Renderer(this, 128);
    return m_renderer;
}

void VulkanWindow::addNew()
{
    m_renderer->addNew();
}

void VulkanWindow::togglePaused()
{
    m_renderer->setAnimating(!m_renderer->animating());
}

void VulkanWindow::meshSwitched(bool enable)
{
    m_renderer->setUseLogo(enable);
}

void VulkanWindow::mousePressEvent(QMouseEvent *e)
{
    m_pressed = true;
    m_lastPos = e->pos();
}

void VulkanWindow::mouseReleaseEvent(QMouseEvent *)
{
    m_pressed = false;
}

void VulkanWindow::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_pressed)
        return;

    int dx = e->pos().x() - m_lastPos.x();
    int dy = e->pos().y() - m_lastPos.y();

    if (dy)
        m_renderer->pitch(dy / 10.0f);

    if (dx)
        m_renderer->yaw(dx / 10.0f);

    m_lastPos = e->pos();
}

void VulkanWindow::keyPressEvent(QKeyEvent *e)
{
    const float amount = e->modifiers().testFlag(Qt::ShiftModifier) ? 1.0f : 0.1f;
    switch (e->key()) {
    case Qt::Key_W:
        m_renderer->walk(amount);
        break;
    case Qt::Key_S:
        m_renderer->walk(-amount);
        break;
    case Qt::Key_A:
        m_renderer->strafe(-amount);
        break;
    case Qt::Key_D:
        m_renderer->strafe(amount);
        break;
    default:
        break;
    }
}

int VulkanWindow::instanceCount() const
{
    return m_renderer->instanceCount();
}
