/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qvkkhrdisplayvulkaninstance.h"

QT_BEGIN_NAMESPACE

QVkKhrDisplayVulkanInstance::QVkKhrDisplayVulkanInstance(QVulkanInstance *instance)
    : m_instance(instance)
{
    loadVulkanLibrary(QStringLiteral("vulkan"), 1);
}

void QVkKhrDisplayVulkanInstance::createOrAdoptInstance()
{
    qDebug("Creating Vulkan instance for VK_KHR_display");

    const QByteArray extName = QByteArrayLiteral("VK_KHR_display");
    initInstance(m_instance, { extName });
    if (!m_vkInst)
        return;

    if (!enabledExtensions().contains(extName)) {
        qWarning("Failed to enable VK_KHR_display extension");
        return;
    }

#if VK_KHR_display
    m_getPhysicalDeviceDisplayPropertiesKHR = (PFN_vkGetPhysicalDeviceDisplayPropertiesKHR)
        m_vkGetInstanceProcAddr(m_vkInst, "vkGetPhysicalDeviceDisplayPropertiesKHR");
    m_getDisplayModePropertiesKHR = (PFN_vkGetDisplayModePropertiesKHR)
        m_vkGetInstanceProcAddr(m_vkInst, "vkGetDisplayModePropertiesKHR");
    m_getPhysicalDeviceDisplayPlanePropertiesKHR = (PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR)
        m_vkGetInstanceProcAddr(m_vkInst, "vkGetPhysicalDeviceDisplayPlanePropertiesKHR");

    m_getDisplayPlaneSupportedDisplaysKHR = (PFN_vkGetDisplayPlaneSupportedDisplaysKHR)
        m_vkGetInstanceProcAddr(m_vkInst, "vkGetDisplayPlaneSupportedDisplaysKHR");
    m_getDisplayPlaneCapabilitiesKHR = (PFN_vkGetDisplayPlaneCapabilitiesKHR)
        m_vkGetInstanceProcAddr(m_vkInst, "vkGetDisplayPlaneCapabilitiesKHR");

    m_createDisplayPlaneSurfaceKHR = (PFN_vkCreateDisplayPlaneSurfaceKHR)
        m_vkGetInstanceProcAddr(m_vkInst, "vkCreateDisplayPlaneSurfaceKHR");
#endif

    m_enumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)
        m_vkGetInstanceProcAddr(m_vkInst, "vkEnumeratePhysicalDevices");

    // Use for first physical device, unless overridden by QT_VK_PHYSICAL_DEVICE_INDEX.
    // This behavior matches what the Vulkan backend of QRhi would do.

    uint32_t physDevCount = 0;
    m_enumeratePhysicalDevices(m_vkInst, &physDevCount, nullptr);
    if (!physDevCount) {
        qWarning("No physical devices");
        return;
    }
    QVarLengthArray<VkPhysicalDevice, 4> physDevs(physDevCount);
    VkResult err = m_enumeratePhysicalDevices(m_vkInst, &physDevCount, physDevs.data());
    if (err != VK_SUCCESS || !physDevCount) {
        qWarning("Failed to enumerate physical devices: %d", err);
        return;
    }

    if (qEnvironmentVariableIsSet("QT_VK_PHYSICAL_DEVICE_INDEX")) {
        int requestedPhysDevIndex = qEnvironmentVariableIntValue("QT_VK_PHYSICAL_DEVICE_INDEX");
        if (requestedPhysDevIndex >= 0 && uint32_t(requestedPhysDevIndex) < physDevCount)
            m_physDev = physDevs[requestedPhysDevIndex];
    }

    if (m_physDev == VK_NULL_HANDLE)
        m_physDev = physDevs[0];

    if (chooseDisplay()) {
        if (m_createdCallback)
            m_createdCallback(this, m_createdCallbackUserData);
    }
}

bool QVkKhrDisplayVulkanInstance::supportsPresent(VkPhysicalDevice physicalDevice,
                                                  uint32_t queueFamilyIndex,
                                                  QWindow *window)
{
    Q_UNUSED(physicalDevice);
    Q_UNUSED(queueFamilyIndex);
    Q_UNUSED(window);
    return true;
}

bool QVkKhrDisplayVulkanInstance::chooseDisplay()
{
#if VK_KHR_display
    uint32_t displayCount = 0;
    VkResult err = m_getPhysicalDeviceDisplayPropertiesKHR(m_physDev, &displayCount, nullptr);
    if (err != VK_SUCCESS) {
        qWarning("Failed to get display properties: %d", err);
        return false;
    }

    qDebug("Display count: %u", displayCount);

    QVarLengthArray<VkDisplayPropertiesKHR, 4> displayProps(displayCount);
    m_getPhysicalDeviceDisplayPropertiesKHR(m_physDev, &displayCount, displayProps.data());

    m_display = VK_NULL_HANDLE;
    m_displayMode = VK_NULL_HANDLE;

    // Pick the first display and the first mode, unless specified via env.vars.
    uint32_t wantedDisplayIndex = 0;
    uint32_t wantedModeIndex = 0;
    if (qEnvironmentVariableIsSet("QT_VK_DISPLAY_INDEX"))
        wantedDisplayIndex = uint32_t(qEnvironmentVariableIntValue("QT_VK_DISPLAY_INDEX"));
    if (qEnvironmentVariableIsSet("QT_VK_MODE_INDEX"))
        wantedModeIndex = uint32_t(qEnvironmentVariableIntValue("QT_VK_MODE_INDEX"));

    for (uint32_t i = 0; i < displayCount; ++i) {
        const VkDisplayPropertiesKHR &disp(displayProps[i]);
        qDebug("Display #%u:\n  display: %p\n  name: %s\n  dimensions: %ux%u\n  resolution: %ux%u",
               i, (void *) disp.display, disp.displayName,
               disp.physicalDimensions.width, disp.physicalDimensions.height,
               disp.physicalResolution.width, disp.physicalResolution.height);

        if (i == wantedDisplayIndex)
            m_display = disp.display;

        uint32_t modeCount = 0;
        if (m_getDisplayModePropertiesKHR(m_physDev, disp.display, &modeCount, nullptr) != VK_SUCCESS) {
            qWarning("Failed to get modes for display");
            continue;
        }
        QVarLengthArray<VkDisplayModePropertiesKHR, 16> modeProps(modeCount);
        m_getDisplayModePropertiesKHR(m_physDev, disp.display, &modeCount, modeProps.data());
        for (uint32_t j = 0; j < modeCount; ++j) {
            const VkDisplayModePropertiesKHR &mode(modeProps[j]);
            qDebug("  Mode #%u:\n    mode: %p\n    visibleRegion: %ux%u\n    refreshRate: %u",
                   j, (void *) mode.displayMode,
                   mode.parameters.visibleRegion.width, mode.parameters.visibleRegion.height,
                   mode.parameters.refreshRate);
            if (j == wantedModeIndex) {
                m_displayMode = mode.displayMode;
                m_width = mode.parameters.visibleRegion.width;
                m_height = mode.parameters.visibleRegion.height;
            }
        }
    }

    if (m_display == VK_NULL_HANDLE || m_displayMode == VK_NULL_HANDLE) {
        qWarning("Failed to choose display and mode");
        return false;
    }

    qDebug("Using display #%u with mode #%u", wantedDisplayIndex, wantedModeIndex);

    uint32_t planeCount = 0;
    err = m_getPhysicalDeviceDisplayPlanePropertiesKHR(m_physDev, &planeCount, nullptr);
    if (err != VK_SUCCESS) {
        qWarning("Failed to get plane properties: %d", err);
        return false;
    }

    qDebug("Plane count: %u", planeCount);

    QVarLengthArray<VkDisplayPlanePropertiesKHR, 4> planeProps(planeCount);
    m_getPhysicalDeviceDisplayPlanePropertiesKHR(m_physDev, &planeCount, planeProps.data());

    m_planeIndex = UINT_MAX;
    for (uint32_t i = 0; i < planeCount; ++i) {
        uint32_t supportedDisplayCount = 0;
        err = m_getDisplayPlaneSupportedDisplaysKHR(m_physDev, i, &supportedDisplayCount, nullptr);
        if (err != VK_SUCCESS) {
            qWarning("Failed to query supported displays for plane: %d", err);
            return false;
        }

        QVarLengthArray<VkDisplayKHR, 4> supportedDisplays(supportedDisplayCount);
        m_getDisplayPlaneSupportedDisplaysKHR(m_physDev, i, &supportedDisplayCount, supportedDisplays.data());
        qDebug("Plane #%u supports %u displays, currently bound to display %p",
               i, supportedDisplayCount, (void *) planeProps[i].currentDisplay);

        VkDisplayPlaneCapabilitiesKHR caps;
        err = m_getDisplayPlaneCapabilitiesKHR(m_physDev, m_displayMode, i, &caps);
        if (err != VK_SUCCESS) {
            qWarning("Failed to query plane capabilities: %d", err);
            return false;
        }

        qDebug("  supportedAlpha: %d (1=no, 2=global, 4=per pixel, 8=per pixel premul)\n"
               "  minSrc=%d, %d %ux%u\n"
               "  maxSrc=%d, %d %ux%u\n"
               "  minDst=%d, %d %ux%u\n"
               "  maxDst=%d, %d %ux%u",
               int(caps.supportedAlpha),
               caps.minSrcPosition.x, caps.minSrcPosition.y, caps.minSrcExtent.width, caps.minSrcExtent.height,
               caps.maxSrcPosition.x, caps.maxSrcPosition.y, caps.maxSrcExtent.width, caps.maxSrcExtent.height,
               caps.minDstPosition.x, caps.minDstPosition.y, caps.minDstExtent.width, caps.minDstExtent.height,
               caps.maxDstPosition.x, caps.maxDstPosition.y, caps.maxDstExtent.width, caps.maxDstExtent.height);

        // if the plane is not in use and supports our chosen display, use that plane
        if (supportedDisplays.contains(m_display)
            && (planeProps[i].currentDisplay == VK_NULL_HANDLE || planeProps[i].currentDisplay == m_display))
        {
            m_planeIndex = i;
            m_planeStackIndex = planeProps[i].currentStackIndex;
        }
    }

    if (m_planeIndex == UINT_MAX) {
        qWarning("Failed to find a suitable plane");
        return false;
    }

    qDebug("Using plane #%u", m_planeIndex);
    return true;
#else
    return false;
#endif
}

VkSurfaceKHR QVkKhrDisplayVulkanInstance::createSurface(QWindow *window)
{
#if VK_KHR_display
    qDebug("Creating VkSurfaceKHR via VK_KHR_display for window %p", (void *) window);

    if (!m_physDev) {
        qWarning("No physical device, cannot create surface");
        return VK_NULL_HANDLE;
    }
    if (!m_display || !m_displayMode) {
        qWarning("No display mode chosen, cannot create surface");
        return VK_NULL_HANDLE;
    }

    VkDisplaySurfaceCreateInfoKHR surfaceCreateInfo = {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.displayMode = m_displayMode;
    surfaceCreateInfo.planeIndex = m_planeIndex;
    surfaceCreateInfo.planeStackIndex = m_planeStackIndex;
    surfaceCreateInfo.transform = VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR;
    surfaceCreateInfo.globalAlpha = 1.0f;
    surfaceCreateInfo.alphaMode = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR;
    surfaceCreateInfo.imageExtent = { m_width, m_height };

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult err = m_createDisplayPlaneSurfaceKHR(m_vkInst, &surfaceCreateInfo, nullptr, &surface);
    if (err != VK_SUCCESS || surface == VK_NULL_HANDLE) {
        qWarning("Failed to create surface: %d", err);
        return VK_NULL_HANDLE;
    }

    qDebug("Created surface %p", (void *) surface);

    return surface;
#else
    Q_UNUSED(window);
    return VK_NULL_HANDLE;
#endif
}

void QVkKhrDisplayVulkanInstance::presentAboutToBeQueued(QWindow *window)
{
    Q_UNUSED(window);
}

QT_END_NAMESPACE
