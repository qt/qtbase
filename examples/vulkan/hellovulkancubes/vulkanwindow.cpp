// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
    m_lastPos = e->position().toPoint();
}

void VulkanWindow::mouseReleaseEvent(QMouseEvent *)
{
    m_pressed = false;
}

void VulkanWindow::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_pressed)
        return;

    int dx = e->position().toPoint().x() - m_lastPos.x();
    int dy = e->position().toPoint().y() - m_lastPos.y();

    if (dy)
        m_renderer->pitch(dy / 10.0f);

    if (dx)
        m_renderer->yaw(dx / 10.0f);

    m_lastPos = e->position().toPoint();
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
