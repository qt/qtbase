// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qbasicvulkanplatforminstance_p.h"
#include <QCoreApplication>
#include <QList>
#include <QLoggingCategory>
#include <QVarLengthArray>

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
{
}

QBasicPlatformVulkanInstance::~QBasicPlatformVulkanInstance()
{
    if (!m_vkInst)
        return;

#ifdef VK_EXT_debug_utils
    if (m_debugMessenger)
        m_vkDestroyDebugUtilsMessengerEXT(m_vkInst, m_debugMessenger, nullptr);
#endif

    if (m_ownsVkInst)
        m_vkDestroyInstance(m_vkInst, nullptr);
}

void QBasicPlatformVulkanInstance::loadVulkanLibrary(const QString &defaultLibraryName, int defaultLibraryVersion)
{
    QVarLengthArray<std::pair<QString, int>, 3> loadList;

    // First in the list of libraries to try is the manual override, relevant on
    // embedded systems without a Vulkan loader and possibly with custom vendor
    // library names.
    if (qEnvironmentVariableIsSet("QT_VULKAN_LIB"))
        loadList.append({ QString::fromUtf8(qgetenv("QT_VULKAN_LIB")), -1 });

    // Then what the platform specified. On Linux the version is likely 1, thus
    // preferring libvulkan.so.1 over libvulkan.so.
    loadList.append({ defaultLibraryName, defaultLibraryVersion });

    // If there was a version given, we must still try without it if the first
    // attempt fails, so that libvulkan.so is picked up if the .so.1 is not
    // present on the system (so loaderless embedded systems still work).
    if (defaultLibraryVersion >= 0)
        loadList.append({ defaultLibraryName, -1 });

    bool ok = false;
    for (const auto &lib : loadList) {
        m_vulkanLib.reset(new QLibrary);
        if (lib.second >= 0)
            m_vulkanLib->setFileNameAndVersion(lib.first, lib.second);
        else
            m_vulkanLib->setFileName(lib.first);
        if (m_vulkanLib->load()) {
            ok = true;
            break;
        }
    }

    if (!ok) {
        qWarning("Failed to load %s: %s", qPrintable(m_vulkanLib->fileName()), qPrintable(m_vulkanLib->errorString()));
        return;
    }

    init(m_vulkanLib.get());
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

    // Do not rely on non-1.0 header typedefs here.
    typedef VkResult (VKAPI_PTR *T_enumerateInstanceVersion)(uint32_t* pApiVersion);
    // Determine instance-level version as described in the Vulkan 1.2 spec.
    T_enumerateInstanceVersion enumerateInstanceVersion = reinterpret_cast<T_enumerateInstanceVersion>(
        m_vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"));
    if (enumerateInstanceVersion) {
        uint32_t ver = 0;
        if (enumerateInstanceVersion(&ver) == VK_SUCCESS) {
            m_supportedApiVersion = QVersionNumber(VK_VERSION_MAJOR(ver),
                                                   VK_VERSION_MINOR(ver),
                                                   VK_VERSION_PATCH(ver));
        } else {
            m_supportedApiVersion = QVersionNumber(1, 0, 0);
        }
    } else {
        // Vulkan 1.0
        m_supportedApiVersion = QVersionNumber(1, 0, 0);
    }

    uint32_t layerCount = 0;
    m_vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    if (layerCount) {
        QList<VkLayerProperties> layerProps(layerCount);
        m_vkEnumerateInstanceLayerProperties(&layerCount, layerProps.data());
        m_supportedLayers.reserve(layerCount);
        for (const VkLayerProperties &p : std::as_const(layerProps)) {
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
        QList<VkExtensionProperties> extProps(extCount);
        m_vkEnumerateInstanceExtensionProperties(nullptr, &extCount, extProps.data());
        m_supportedExtensions.reserve(extCount);
        for (const VkExtensionProperties &p : std::as_const(extProps)) {
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

QVersionNumber QBasicPlatformVulkanInstance::supportedApiVersion() const
{
    return m_supportedApiVersion;
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
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        QByteArray appName = QCoreApplication::applicationName().toUtf8();
        appInfo.pApplicationName = appName.constData();
        const QVersionNumber apiVersion = instance->apiVersion();
        if (!apiVersion.isNull()) {
            appInfo.apiVersion = VK_MAKE_VERSION(apiVersion.majorVersion(),
                                                 apiVersion.minorVersion(),
                                                 apiVersion.microVersion());
        }

        m_enabledExtensions.append("VK_KHR_surface");
        if (!flags.testFlag(QVulkanInstance::NoPortabilityDrivers))
            m_enabledExtensions.append("VK_KHR_portability_enumeration");
        if (!flags.testFlag(QVulkanInstance::NoDebugOutputRedirect))
            m_enabledExtensions.append("VK_EXT_debug_utils");

        for (const QByteArray &ext : extraExts)
            m_enabledExtensions.append(ext);

        QByteArray envExts = qgetenv("QT_VULKAN_INSTANCE_EXTENSIONS");
        if (!envExts.isEmpty()) {
            QByteArrayList envExtList =  envExts.split(';');
            for (auto ext : m_enabledExtensions)
                envExtList.removeAll(ext);
            m_enabledExtensions.append(envExtList);
        }

        QByteArray envLayers = qgetenv("QT_VULKAN_INSTANCE_LAYERS");
        if (!envLayers.isEmpty()) {
            QByteArrayList envLayerList = envLayers.split(';');
            for (auto ext : m_enabledLayers)
                envLayerList.removeAll(ext);
            m_enabledLayers.append(envLayerList);
        }

        // No clever stuff with QSet and friends: the order for layers matters
        // and the user-provided order must be kept.
        for (int i = 0; i < m_enabledLayers.size(); ++i) {
            const QByteArray &layerName(m_enabledLayers[i]);
            if (!m_supportedLayers.contains(layerName))
                m_enabledLayers.removeAt(i--);
        }
        qDebug(lcPlatVk) << "Enabling Vulkan instance layers:" << m_enabledLayers;
        for (int i = 0; i < m_enabledExtensions.size(); ++i) {
            const QByteArray &extName(m_enabledExtensions[i]);
            if (!m_supportedExtensions.contains(extName))
                m_enabledExtensions.removeAt(i--);
        }
        qDebug(lcPlatVk) << "Enabling Vulkan instance extensions:" << m_enabledExtensions;

        VkInstanceCreateInfo instInfo = {};
        instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instInfo.pApplicationInfo = &appInfo;
        if (!flags.testFlag(QVulkanInstance::NoPortabilityDrivers)) {
            // With old Vulkan SDKs setting a non-zero flags gives a validation error.
            // Whereas from 1.3.216 on the portability bit is required for MoltenVK to function.
            // Hence the version check.
            if (m_supportedApiVersion >= QVersionNumber(1, 3, 216))
                instInfo.flags |= 0x00000001; // VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR
        }

        QList<const char *> layerNameVec;
        for (const QByteArray &ba : std::as_const(m_enabledLayers))
            layerNameVec.append(ba.constData());
        if (!layerNameVec.isEmpty()) {
            instInfo.enabledLayerCount = layerNameVec.size();
            instInfo.ppEnabledLayerNames = layerNameVec.constData();
        }

        QList<const char *> extNameVec;
        for (const QByteArray &ba : std::as_const(m_enabledExtensions))
            extNameVec.append(ba.constData());
        if (!extNameVec.isEmpty()) {
            instInfo.enabledExtensionCount = extNameVec.size();
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

    m_destroySurface = reinterpret_cast<PFN_vkDestroySurfaceKHR>(
                m_vkGetInstanceProcAddr(m_vkInst, "vkDestroySurfaceKHR"));
    if (!m_destroySurface)
        qWarning("Failed to find vkDestroySurfaceKHR");

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

void QBasicPlatformVulkanInstance::setDebugFilters(const QList<QVulkanInstance::DebugFilter> &filters)
{
    m_debugFilters = filters;
}

void QBasicPlatformVulkanInstance::setDebugUtilsFilters(const QList<QVulkanInstance::DebugUtilsFilter> &filters)
{
    m_debugUtilsFilters = filters;
}

void QBasicPlatformVulkanInstance::destroySurface(VkSurfaceKHR surface) const
{
    if (m_destroySurface && surface)
        m_destroySurface(m_vkInst, surface, nullptr);
}

#ifdef VK_EXT_debug_utils
static VKAPI_ATTR VkBool32 VKAPI_CALL defaultDebugCallbackFunc(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                               VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                               const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                               void *pUserData)
{
    QBasicPlatformVulkanInstance *self = static_cast<QBasicPlatformVulkanInstance *>(pUserData);

    // legacy filters
    for (QVulkanInstance::DebugFilter filter : *self->debugFilters()) {
        // As per docs in qvulkaninstance.cpp we pass object, messageCode,
        // pMessage to the callback with the legacy signature.
        uint64_t object = 0;
        if (pCallbackData->objectCount > 0)
            object = pCallbackData->pObjects[0].objectHandle;
        if (filter(0, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, object, 0,
                   pCallbackData->messageIdNumber, "", pCallbackData->pMessage))
        {
            return VK_FALSE;
        }
    }

    // filters with new signature
    for (QVulkanInstance::DebugUtilsFilter filter : *self->debugUtilsFilters()) {
        QVulkanInstance::DebugMessageSeverityFlags severity;
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
            severity |= QVulkanInstance::VerboseSeverity;
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
            severity |= QVulkanInstance::InfoSeverity;
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            severity |= QVulkanInstance::WarningSeverity;
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            severity |= QVulkanInstance::ErrorSeverity;
        QVulkanInstance::DebugMessageTypeFlags type;
        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
            type |= QVulkanInstance::GeneralMessage;
        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
            type |= QVulkanInstance::ValidationMessage;
        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
            type |= QVulkanInstance::PerformanceMessage;
        if (filter(severity, type, pCallbackData))
            return VK_FALSE;
    }

    // not categorized, just route to plain old qDebug
    qDebug("vkDebug: %s", pCallbackData->pMessage);

    return VK_FALSE;
}
#endif

void QBasicPlatformVulkanInstance::setupDebugOutput()
{
#ifdef VK_EXT_debug_utils
    if (!m_enabledExtensions.contains("VK_EXT_debug_utils"))
        return;

    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
                m_vkGetInstanceProcAddr(m_vkInst, "vkCreateDebugUtilsMessengerEXT"));

    m_vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                m_vkGetInstanceProcAddr(m_vkInst, "vkDestroyDebugUtilsMessengerEXT"));

    VkDebugUtilsMessengerCreateInfoEXT messengerInfo = {};
    messengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    messengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    messengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    messengerInfo.pfnUserCallback = defaultDebugCallbackFunc;
    messengerInfo.pUserData = this;
    VkResult err = vkCreateDebugUtilsMessengerEXT(m_vkInst, &messengerInfo, nullptr, &m_debugMessenger);
    if (err != VK_SUCCESS)
        qWarning("Failed to create debug report callback: %d", err);
#endif
}

QT_END_NAMESPACE
