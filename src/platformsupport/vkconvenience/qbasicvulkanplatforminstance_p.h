/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include <QtCore/QLibrary>
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
    void setDebugFilters(const QVector<QVulkanInstance::DebugFilter> &filters) override;

    void destroySurface(VkSurfaceKHR surface) const;
    const QVector<QVulkanInstance::DebugFilter> *debugFilters() const { return &m_debugFilters; }

protected:
    void loadVulkanLibrary(const QString &defaultLibraryName);
    void init(QLibrary *lib);
    void initInstance(QVulkanInstance *instance, const QByteArrayList &extraExts);

    VkInstance m_vkInst;
    PFN_vkGetInstanceProcAddr m_vkGetInstanceProcAddr;
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR m_getPhysDevSurfaceSupport;
    PFN_vkDestroySurfaceKHR m_destroySurface;

private:
    void setupDebugOutput();

    QLibrary m_vulkanLib;

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
    QVector<QVulkanInstance::DebugFilter> m_debugFilters;
};

QT_END_NAMESPACE

#endif // QBASICVULKANPLATFORMINSTANCE_P_H
