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
