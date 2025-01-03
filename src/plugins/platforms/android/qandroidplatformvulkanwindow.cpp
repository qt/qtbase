// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "androiddeadlockprotector.h"
#include "androidjnimain.h"
#include "qandroideventdispatcher.h"
#include "qandroidplatformscreen.h"
#include "qandroidplatformvulkanwindow.h"

#include <QSurfaceFormat>
#include <qpa/qplatformscreen.h>
#include <qpa/qwindowsysteminterface.h>

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
    if (rect.width() > 0
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

        QJniEnvironment env;
        m_nativeWindow = ANativeWindow_fromSurface(env.jniEnv(), m_androidSurfaceObject.object());

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
