/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qandroidplatformvulkanwindow.h"
#include "qandroidplatformscreen.h"
#include "androidjnimain.h"
#include "qandroideventdispatcher.h"
#include "androiddeadlockprotector.h"

#include <QSurfaceFormat>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformscreen.h>

#include <android/native_window.h>
#include <android/native_window_jni.h>

QT_BEGIN_NAMESPACE

QAndroidPlatformVulkanWindow::QAndroidPlatformVulkanWindow(QWindow *window)
    : QAndroidPlatformWindow(window),
      m_nativeSurfaceId(-1),
      m_nativeWindow(nullptr),
      m_vkSurface(0),
      m_createVkSurface(nullptr),
      m_destroyVkSurface(nullptr)
{
}

QAndroidPlatformVulkanWindow::~QAndroidPlatformVulkanWindow()
{
    m_surfaceWaitCondition.wakeOne();
    lockSurface();
    if (m_nativeSurfaceId != -1)
        QtAndroid::destroySurface(m_nativeSurfaceId);
    clearSurface();
    unlockSurface();
}

void QAndroidPlatformVulkanWindow::setGeometry(const QRect &rect)
{
    if (rect == geometry())
        return;

    m_oldGeometry = geometry();

    QAndroidPlatformWindow::setGeometry(rect);
    if (m_nativeSurfaceId != -1)
        QtAndroid::setSurfaceGeometry(m_nativeSurfaceId, rect);

    QRect availableGeometry = screen()->availableGeometry();
    if (m_oldGeometry.width() == 0
            && m_oldGeometry.height() == 0
            && rect.width() > 0
            && rect.height() > 0
            && availableGeometry.width() > 0
            && availableGeometry.height() > 0) {
        QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(0, 0), rect.size()));
    }

    if (rect.topLeft() != m_oldGeometry.topLeft())
        repaint(QRegion(rect));
}

void QAndroidPlatformVulkanWindow::applicationStateChanged(Qt::ApplicationState state)
{
    QAndroidPlatformWindow::applicationStateChanged(state);
    if (state <= Qt::ApplicationHidden) {
        lockSurface();
        if (m_nativeSurfaceId != -1) {
            QtAndroid::destroySurface(m_nativeSurfaceId);
            m_nativeSurfaceId = -1;
        }
        clearSurface();
        unlockSurface();
    }
}

QSurfaceFormat QAndroidPlatformVulkanWindow::format() const
{
    return window()->requestedFormat();
}

void QAndroidPlatformVulkanWindow::clearSurface()
{
    if (m_vkSurface && m_destroyVkSurface) {
        m_destroyVkSurface(window()->vulkanInstance()->vkInstance(), m_vkSurface, nullptr);
        m_vkSurface = 0;
    }

    if (m_nativeWindow) {
        ANativeWindow_release(m_nativeWindow);
        m_nativeWindow = nullptr;
    }
}

void QAndroidPlatformVulkanWindow::sendExpose()
{
    QRect availableGeometry = screen()->availableGeometry();
    if (geometry().width() > 0 && geometry().height() > 0 && availableGeometry.width() > 0 && availableGeometry.height() > 0)
        QWindowSystemInterface::handleExposeEvent(window(), QRegion(QRect(QPoint(), geometry().size())));
}

void QAndroidPlatformVulkanWindow::surfaceChanged(JNIEnv *jniEnv, jobject surface, int w, int h)
{
    Q_UNUSED(jniEnv);
    Q_UNUSED(w);
    Q_UNUSED(h);

    lockSurface();
    m_androidSurfaceObject = surface;
    if (surface)
        m_surfaceWaitCondition.wakeOne();
    unlockSurface();

    if (surface)
        sendExpose();
}

VkSurfaceKHR *QAndroidPlatformVulkanWindow::vkSurface()
{
    if (QAndroidEventDispatcherStopper::stopped())
        return &m_vkSurface;

    bool needsExpose = false;
    if (!m_vkSurface) {
        clearSurface();

        QMutexLocker lock(&m_surfaceMutex);
        if (m_nativeSurfaceId == -1) {
            AndroidDeadlockProtector protector;
            if (!protector.acquire())
                return &m_vkSurface;
            const bool windowStaysOnTop = bool(window()->flags() & Qt::WindowStaysOnTopHint);
            m_nativeSurfaceId = QtAndroid::createSurface(this, geometry(), windowStaysOnTop, 32);
            m_surfaceWaitCondition.wait(&m_surfaceMutex);
        }

        if (m_nativeSurfaceId == -1 || !m_androidSurfaceObject.isValid())
            return &m_vkSurface;

        QJNIEnvironmentPrivate env;
        m_nativeWindow = ANativeWindow_fromSurface(env, m_androidSurfaceObject.object());

        VkAndroidSurfaceCreateInfoKHR surfaceInfo;
        memset(&surfaceInfo, 0, sizeof(surfaceInfo));
        surfaceInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
        surfaceInfo.window = m_nativeWindow;
        QVulkanInstance *inst = window()->vulkanInstance();
        if (!inst) {
            qWarning("Attempted to create Vulkan surface without an instance; was QWindow::setVulkanInstance() called?");
            return &m_vkSurface;
        }
        if (!m_createVkSurface) {
            m_createVkSurface = reinterpret_cast<PFN_vkCreateAndroidSurfaceKHR>(
                        inst->getInstanceProcAddr("vkCreateAndroidSurfaceKHR"));
        }
        if (!m_destroyVkSurface) {
            m_destroyVkSurface = reinterpret_cast<PFN_vkDestroySurfaceKHR>(
                        inst->getInstanceProcAddr("vkDestroySurfaceKHR"));
        }
        VkResult err = m_createVkSurface(inst->vkInstance(), &surfaceInfo, nullptr, &m_vkSurface);
        if (err != VK_SUCCESS)
            qWarning("Failed to create Android VkSurface: %d", err);

        needsExpose = true;
    }

    if (needsExpose)
        sendExpose();

    return &m_vkSurface;
}

QT_END_NAMESPACE
