/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QANDROIDPLATFORMVULKANWINDOW_H
#define QANDROIDPLATFORMVULKANWINDOW_H

#if defined(VULKAN_H_) && !defined(VK_USE_PLATFORM_ANDROID_KHR)
#error "vulkan.h included without Android WSI"
#endif

#define VK_USE_PLATFORM_ANDROID_KHR

#include <QWaitCondition>
#include <QtCore/private/qjni_p.h>

#include "androidsurfaceclient.h"
#include "qandroidplatformwindow.h"

#include "qandroidplatformvulkaninstance.h"

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
    QJNIObjectPrivate m_androidSurfaceObject;
    QWaitCondition m_surfaceWaitCondition;
    QSurfaceFormat m_format;
    QRect m_oldGeometry;
    VkSurfaceKHR m_vkSurface;
    PFN_vkCreateAndroidSurfaceKHR m_createVkSurface;
    PFN_vkDestroySurfaceKHR m_destroyVkSurface;
};

QT_END_NAMESPACE

#endif // QANDROIDPLATFORMVULKANWINDOW_H
