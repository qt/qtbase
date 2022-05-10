// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDPLATFORMVULKANWINDOW_H
#define QANDROIDPLATFORMVULKANWINDOW_H

#if defined(VULKAN_H_) && !defined(VK_USE_PLATFORM_ANDROID_KHR)
#error "vulkan.h included without Android WSI"
#endif

#define VK_USE_PLATFORM_ANDROID_KHR

#include "androidsurfaceclient.h"
#include "qandroidplatformvulkaninstance.h"
#include "qandroidplatformwindow.h"

#include <QWaitCondition>
#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>

QT_BEGIN_NAMESPACE

class QAndroidPlatformVulkanWindow : public QAndroidPlatformWindow, public AndroidSurfaceClient
{
public:
    explicit QAndroidPlatformVulkanWindow(QWindow *window);
    ~QAndroidPlatformVulkanWindow();

    void setGeometry(const QRect &rect) override;
    QSurfaceFormat format() const override;
    void applicationStateChanged(Qt::ApplicationState) override;

    VkSurfaceKHR *vkSurface();

protected:
    void surfaceChanged(JNIEnv *jniEnv, jobject surface, int w, int h) override;

private:
    void sendExpose();
    void clearSurface();

    int m_nativeSurfaceId;
    ANativeWindow *m_nativeWindow;
    QJniObject m_androidSurfaceObject;
    QWaitCondition m_surfaceWaitCondition;
    QSurfaceFormat m_format;
    QRect m_oldGeometry;
    VkSurfaceKHR m_vkSurface;
    PFN_vkCreateAndroidSurfaceKHR m_createVkSurface;
    PFN_vkDestroySurfaceKHR m_destroyVkSurface;
};

QT_END_NAMESPACE

#endif // QANDROIDPLATFORMVULKANWINDOW_H
