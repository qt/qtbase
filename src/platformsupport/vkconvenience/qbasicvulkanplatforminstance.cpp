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

#include "qbasicvulkanplatforminstance_p.h"
#include <QLibrary>
#include <QCoreApplication>
#include <QVector>
#include <QLoggingCategory>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcPlatVk, "qt.vulkan")

/*!
    \class QBasicPlatformVulkanInstance
    \brief A generic platform Vulkan instance implementation.
    \since 5.10
    \internal
    \ingroup qpa

    Implements QPlatformVulkanInstance, serving as a base for platform-specific
    implementations. The library loading and any WSI-specifics are excluded.

    Subclasses are expected to call init() from their constructor and
    initInstance() from their createOrAdoptInstance() implementation.
 */

QBasicPlatformVulkanInstance::QBasicPlatformVulkanInstance()
    : m_vkInst(VK_NULL_HANDLE),
      m_vkGetInstanceProcAddr(nullptr),
      m_ownsVkInst(false),
      m_errorCode(VK_SUCCESS),
      m_debugCallback(0)
{
}

QBasicPlatformVulkanInstance::~QBasicPlatformVulkanInstance()
{
    if (!m_vkInst)
        return;

    if (m_debugCallback && m_vkDestroyDebugReportCallbackEXT)
        m_vkDestroyDebugReportCallbackEXT(m_vkInst, m_debugCallback, nullptr);

    if (m_ownsVkInst)
        m_vkDestroyInstance(m_vkInst, nullptr);
}

void QBasicPlatformVulkanInstance::init(QLibrary *lib)
{
    if (m_vkGetInstanceProcAddr)
        return;

    qCDebug(lcPlatVk, "Vulkan init (%s)", qPrintable(lib->fileName()));

    // While not strictly required with every implementation, try to follow the spec
    // and do not rely on core functions being exported.
    //
    // 1. dlsym vkGetInstanceProcAddr
    // 2. with a special null instance resolve vkCreateInstance and vkEnumerateInstance*
    // 3. all other core functions are resolved with the created instance

    m_vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(lib->resolve("vkGetInstanceProcAddr"));
    if (!m_vkGetInstanceProcAddr) {
        qWarning("Failed to find vkGetInstanceProcAddr");
        return;
    }

    m_vkCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(m_vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance"));
    if (!m_vkCreateInstance) {
        qWarning("Failed to find vkCreateInstance");
        return;
    }
    m_vkEnumerateInstanceLayerProperties = reinterpret_cast<PFN_vkEnumerateInstanceLayerProperties>(
                m_vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceLayerProperties"));
    if (!m_vkEnumerateInstanceLayerProperties) {
        qWarning("Failed to find vkEnumerateInstanceLayerProperties");
        return;
    }
    m_vkEnumerateInstanceExtensionProperties = reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(
                m_vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceExtensionProperties"));
    if (!m_vkEnumerateInstanceExtensionProperties) {
        qWarning("Failed to find vkEnumerateInstanceExtensionProperties");
        return;
    }

    uint32_t layerCount = 0;
    m_vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    if (layerCount) {
        QVector<VkLayerProperties> layerProps(layerCount);
        m_vkEnumerateInstanceLayerProperties(&layerCount, layerProps.data());
        m_supportedLayers.reserve(layerCount);
        for (const VkLayerProperties &p : qAsConst(layerProps)) {
            QVulkanLayer layer;
            layer.name = p.layerName;
            layer.version = p.implementationVersion;
            layer.specVersion = QVersionNumber(VK_VERSION_MAJOR(p.specVersion),
                                               VK_VERSION_MINOR(p.specVersion),
                                               VK_VERSION_PATCH(p.specVersion));
            layer.description = p.description;
            m_supportedLayers.append(layer);
        }
    }
    qCDebug(lcPlatVk) << "Supported Vulkan instance layers:" << m_supportedLayers;

    uint32_t extCount = 0;
    m_vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
    if (extCount) {
        QVector<VkExtensionProperties> extProps(extCount);
        m_vkEnumerateInstanceExtensionProperties(nullptr, &extCount, extProps.data());
        m_supportedExtensions.reserve(extCount);
        for (const VkExtensionProperties &p : qAsConst(extProps)) {
            QVulkanExtension ext;
            ext.name = p.extensionName;
            ext.version = p.specVersion;
            m_supportedExtensions.append(ext);
        }
    }
    qDebug(lcPlatVk) << "Supported Vulkan instance extensions:" << m_supportedExtensions;
}

QVulkanInfoVector<QVulkanLayer> QBasicPlatformVulkanInstance::supportedLayers() const
{
    return m_supportedLayers;
}

QVulkanInfoVector<QVulkanExtension> QBasicPlatformVulkanInstance::supportedExtensions() const
{
    return m_supportedExtensions;
}

void QBasicPlatformVulkanInstance::initInstance(QVulkanInstance *instance, const QByteArrayList &extraExts)
{
    if (!m_vkGetInstanceProcAddr) {
        qWarning("initInstance: No Vulkan library available");
        return;
    }

    m_vkInst = instance->vkInstance(); // when non-null we are adopting an existing instance

    QVulkanInstance::Flags flags = instance->flags();
    m_enabledLayers = instance->layers();
    m_enabledExtensions = instance->extensions();

    if (!m_vkInst) {
        VkApplicationInfo appInfo;
        memset(&appInfo, 0, sizeof(appInfo));
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        QByteArray appName = QCoreApplication::applicationName().toUtf8();
        appInfo.pApplicationName = appName.constData();
        const QVersionNumber apiVersion = instance->apiVersion();
        if (!apiVersion.isNull()) {
            appInfo.apiVersion = VK_MAKE_VERSION(apiVersion.majorVersion(),
                                                 apiVersion.minorVersion(),
                                                 apiVersion.microVersion());
        }

        if (!flags.testFlag(QVulkanInstance::NoDebugOutputRedirect))
            m_enabledExtensions.append("VK_EXT_debug_report");

        m_enabledExtensions.append("VK_KHR_surface");

        for (const QByteArray &ext : extraExts)
            m_enabledExtensions.append(ext);

        // No clever stuff with QSet and friends: the order for layers matters
        // and the user-provided order must be kept.
        for (int i = 0; i < m_enabledLayers.count(); ++i) {
            const QByteArray &layerName(m_enabledLayers[i]);
            if (!m_supportedLayers.contains(layerName))
                m_enabledLayers.removeAt(i--);
        }
        qDebug(lcPlatVk) << "Enabling Vulkan instance layers:" << m_enabledLayers;
        for (int i = 0; i < m_enabledExtensions.count(); ++i) {
            const QByteArray &extName(m_enabledExtensions[i]);
            if (!m_supportedExtensions.contains(extName))
                m_enabledExtensions.removeAt(i--);
        }
        qDebug(lcPlatVk) << "Enabling Vulkan instance extensions:" << m_enabledExtensions;

        VkInstanceCreateInfo instInfo;
        memset(&instInfo, 0, sizeof(instInfo));
        instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instInfo.pApplicationInfo = &appInfo;

        QVector<const char *> layerNameVec;
        for (const QByteArray &ba : qAsConst(m_enabledLayers))
            layerNameVec.append(ba.constData());
        if (!layerNameVec.isEmpty()) {
            instInfo.enabledLayerCount = layerNameVec.count();
            instInfo.ppEnabledLayerNames = layerNameVec.constData();
        }

        QVector<const char *> extNameVec;
        for (const QByteArray &ba : qAsConst(m_enabledExtensions))
            extNameVec.append(ba.constData());
        if (!extNameVec.isEmpty()) {
            instInfo.enabledExtensionCount = extNameVec.count();
            instInfo.ppEnabledExtensionNames = extNameVec.constData();
        }

        m_errorCode = m_vkCreateInstance(&instInfo, nullptr, &m_vkInst);
        if (m_errorCode != VK_SUCCESS || !m_vkInst) {
            qWarning("Failed to create Vulkan instance: %d", m_errorCode);
            return;
        }

        m_vkDestroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(m_vkGetInstanceProcAddr(m_vkInst, "vkDestroyInstance"));
        if (!m_vkDestroyInstance) {
            qWarning("Failed to find vkDestroyInstance");
            m_vkInst = VK_NULL_HANDLE;
            return;
        }

        m_ownsVkInst = true;
    }

    m_getPhysDevSurfaceSupport = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(
                m_vkGetInstanceProcAddr(m_vkInst, "vkGetPhysicalDeviceSurfaceSupportKHR"));
    if (!m_getPhysDevSurfaceSupport)
        qWarning("Failed to find vkGetPhysicalDeviceSurfaceSupportKHR");

    if (!flags.testFlag(QVulkanInstance::NoDebugOutputRedirect))
        setupDebugOutput();
}

bool QBasicPlatformVulkanInstance::isValid() const
{
    return m_vkInst != VK_NULL_HANDLE;
}

VkResult QBasicPlatformVulkanInstance::errorCode() const
{
    return m_errorCode;
}

VkInstance QBasicPlatformVulkanInstance::vkInstance() const
{
    return m_vkInst;
}

QByteArrayList QBasicPlatformVulkanInstance::enabledLayers() const
{
    return m_enabledLayers;
}

QByteArrayList QBasicPlatformVulkanInstance::enabledExtensions() const
{
    return m_enabledExtensions;
}

PFN_vkVoidFunction QBasicPlatformVulkanInstance::getInstanceProcAddr(const char *name)
{
    if (!name)
        return nullptr;

    const bool needsNullInstance = !strcmp(name, "vkEnumerateInstanceLayerProperties")
            || !strcmp(name, "vkEnumerateInstanceExtensionProperties");

    return m_vkGetInstanceProcAddr(needsNullInstance ? 0 : m_vkInst, name);
}

bool QBasicPlatformVulkanInstance::supportsPresent(VkPhysicalDevice physicalDevice,
                                                   uint32_t queueFamilyIndex,
                                                   QWindow *window)
{
    if (!m_getPhysDevSurfaceSupport)
        return true;

    VkSurfaceKHR surface = QVulkanInstance::surfaceForWindow(window);
    VkBool32 supported = false;
    m_getPhysDevSurfaceSupport(physicalDevice, queueFamilyIndex, surface, &supported);

    return supported;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL defaultDebugCallbackFunc(VkDebugReportFlagsEXT flags,
                                                               VkDebugReportObjectTypeEXT objectType,
                                                               uint64_t object,
                                                               size_t location,
                                                               int32_t messageCode,
                                                               const char *pLayerPrefix,
                                                               const char *pMessage,
                                                               void *pUserData)
{
    Q_UNUSED(flags);
    Q_UNUSED(objectType);
    Q_UNUSED(object);
    Q_UNUSED(location);
    Q_UNUSED(pUserData);

    // not categorized, just route to plain old qDebug
    qDebug("vkDebug: %s: %d: %s", pLayerPrefix, messageCode, pMessage);

    return VK_FALSE;
}

void QBasicPlatformVulkanInstance::setupDebugOutput()
{
    if (!m_enabledExtensions.contains("VK_EXT_debug_report"))
        return;

    PFN_vkCreateDebugReportCallbackEXT createDebugReportCallback = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
                m_vkGetInstanceProcAddr(m_vkInst, "vkCreateDebugReportCallbackEXT"));
    m_vkDestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(
                m_vkGetInstanceProcAddr(m_vkInst, "vkDestroyDebugReportCallbackEXT"));

    VkDebugReportCallbackCreateInfoEXT dbgCallbackInfo;
    memset(&dbgCallbackInfo, 0, sizeof(dbgCallbackInfo));
    dbgCallbackInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    dbgCallbackInfo.flags =  VK_DEBUG_REPORT_ERROR_BIT_EXT
            | VK_DEBUG_REPORT_WARNING_BIT_EXT
            | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    dbgCallbackInfo.pfnCallback = defaultDebugCallbackFunc;

    VkResult err = createDebugReportCallback(m_vkInst, &dbgCallbackInfo, nullptr, &m_debugCallback);
    if (err != VK_SUCCESS)
        qWarning("Failed to create debug report callback: %d", err);
}

QT_END_NAMESPACE
