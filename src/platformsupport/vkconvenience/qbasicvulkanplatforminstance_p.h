/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QBASICVULKANPLATFORMINSTANCE_P_H
#define QBASICVULKANPLATFORMINSTANCE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qpa/qplatformvulkaninstance.h>

QT_BEGIN_NAMESPACE

class QLibrary;

class QBasicPlatformVulkanInstance : public QPlatformVulkanInstance
{
public:
    QBasicPlatformVulkanInstance();
    ~QBasicPlatformVulkanInstance();

    QVulkanInfoVector<QVulkanLayer> supportedLayers() const override;
    QVulkanInfoVector<QVulkanExtension> supportedExtensions() const override;
    bool isValid() const override;
    VkResult errorCode() const override;
    VkInstance vkInstance() const override;
    QByteArrayList enabledLayers() const override;
    QByteArrayList enabledExtensions() const override;
    PFN_vkVoidFunction getInstanceProcAddr(const char *name) override;
    bool supportsPresent(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, QWindow *window) override;

protected:
    void init(QLibrary *lib);
    void initInstance(QVulkanInstance *instance, const QByteArrayList &extraExts);

    VkInstance m_vkInst;
    PFN_vkGetInstanceProcAddr m_vkGetInstanceProcAddr;
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR m_getPhysDevSurfaceSupport;

private:
    void setupDebugOutput();

    bool m_ownsVkInst;
    VkResult m_errorCode;
    QVulkanInfoVector<QVulkanLayer> m_supportedLayers;
    QVulkanInfoVector<QVulkanExtension> m_supportedExtensions;
    QByteArrayList m_enabledLayers;
    QByteArrayList m_enabledExtensions;

    PFN_vkCreateInstance m_vkCreateInstance;
    PFN_vkEnumerateInstanceLayerProperties m_vkEnumerateInstanceLayerProperties;
    PFN_vkEnumerateInstanceExtensionProperties m_vkEnumerateInstanceExtensionProperties;

    PFN_vkDestroyInstance m_vkDestroyInstance;

    VkDebugReportCallbackEXT m_debugCallback;
    PFN_vkDestroyDebugReportCallbackEXT m_vkDestroyDebugReportCallbackEXT;
};

QT_END_NAMESPACE

#endif // QBASICVULKANPLATFORMINSTANCE_P_H
