/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qvulkaninstance.h"
#include <private/qvulkanfunctions_p.h>
#include <qpa/qplatformvulkaninstance.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>
#include <QtGui/private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QVulkanInstance
    \since 5.10
    \inmodule QtGui

    \brief The QVulkanInstance class represents a native Vulkan instance, enabling
    Vulkan rendering onto a QSurface.

    \l{https://www.khronos.org/vulkan/}{Vulkan} is a cross-platform, explicit
    graphics and compute API. This class provides support for loading a Vulkan
    library and creating an \c instance in a cross-platform manner. For an
    introduction on Vulkan instances, refer
    \l{https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#initialization-instances}{to
    section 3.2 of the specification}.

    \note Platform-specific support for Vulkan instances and windows with
    Vulkan-capable surfaces is provided by the various platform plugins. Not
    all of them will support Vulkan, however. When running on such a platform,
    create() will fail and always return \c false.

    \note Vulkan support may get automatically disabled for a given Qt build due
    to not having the necessary Vulkan headers available at build time. When
    this is the case, and the output of \c configure indicates Vulkan support is
    disabled, the QVulkan* classes will be unavailable.

    \note Some functions changed their signature between the various Vulkan
    header revisions. When building Qt and only headers with the old,
    conflicting signatures are present in a system, Vulkan support will get
    disabled. It is recommended to use headers from Vulkan 1.0.39 or newer.

    \section1 Initialization

    Similarly to QOpenGLContext, any actual Vulkan instance creation happens
    only when calling create(). This allows using QVulkanInstance as a plain
    member variable while retaining control over when to perform
    initialization.

    Querying the supported instance-level layers and extensions is possible by
    calling supportedLayers() and supportedExtensions(). These ensure the
    Vulkan library is loaded, and can therefore be called safely before
    create() as well.

    Instances store per-application Vulkan state and creating a \c VkInstance
    object initializes the Vulkan library. In practice there will typically be
    a single instance constructed early on in main(). The object then stays
    alive until exiting the application.

    Every Vulkan-based QWindow must be associated with a QVulkanInstance by
    calling QWindow::setVulkanInstance(). Thus a typical application pattern is
    the following:

    \snippet code/src_gui_vulkan_qvulkaninstance.cpp 0

    \section1 Configuration

    QVulkanInstance automatically enables the minimum set of extensions it
    needs on the newly created instance. In practice this means the
    \c{VK_KHR_*_surface} family of extensions.

    By default Vulkan debug output, for example messages from the validation
    layers, is routed to qDebug(). This can be disabled by passing the flag
    \c NoDebugOutputRedirect to setFlags() \e before invoking create().

    To enable additional layers and extensions, provide the list via
    setLayers() and setExtensions() \e before invoking create(). When a
    given layer or extension is not reported as available from the instance,
    the request is ignored. After a successful call to create(), the values
    returned from functions like layers() and extensions() reflect the actual
    enabled layers and extensions. When necessary, for example to avoid
    requesting extensions that conflict and thus would fail the Vulkan instance
    creation, the list of actually supported layers and extensions can be
    examined via supportedLayers() and supportedExtensions() before calling
    create().

    For example, to enable the standard validation layers, one could do the
    following:

    \snippet code/src_gui_vulkan_qvulkaninstance.cpp 1

    Or, alternatively, to make decisions before attempting to create a Vulkan
    instance:

    \snippet code/src_gui_vulkan_qvulkaninstance.cpp 2

    \section1 Adopting an Existing Instance

    By default QVulkanInstance creates a new Vulkan instance. When working with
    external engines and renderers, this may sometimes not be desirable. When
    there is a \c VkInstance handle already available, call setVkInstance()
    before invoking create(). This way no additional instances will get
    created, and QVulkanInstance will not own the handle.

    \note It is up to the component creating the external instance to ensure
    the necessary extensions are enabled on it. These are: \c{VK_KHR_surface},
    the WSI-specific \c{VK_KHR_*_surface} that is appropriate for the platform
    in question, and \c{VK_EXT_debug_report} in case QVulkanInstance's debug
    output redirection is desired.

    \section1 Accessing Core Vulkan Commands

    To access the \c VkInstance handle the QVulkanInstance wraps, call
    vkInstance(). To resolve Vulkan functions, call getInstanceProcAddr(). For
    core Vulkan commands manual resolving is not necessary as they are provided
    via the QVulkanFunctions and QVulkanDeviceFunctions objects accessible via
    functions() and deviceFunctions().

    \note QVulkanFunctions and QVulkanDeviceFunctions are generated from the
    Vulkan API XML specifications when building the Qt libraries. Therefore no
    documentation is provided for them. They contain the Vulkan 1.0 functions
    with the same signatures as described in the
    \l{https://www.khronos.org/registry/vulkan/specs/1.0/html/}{Vulkan API
    documentation}.

    \section1 Getting a Native Vulkan Surface for a Window

    The two common windowing system specific operations are getting a surface
    (a \c{VkSurfaceKHR} handle) for a window, and querying if a given queue
    family supports presenting to a given surface. To avoid WSI-specific bits
    in the applications, these are abstracted by QVulkanInstance and the
    underlying QPA layers.

    To create a Vulkan surface for a window, or retrieve an existing one,
    call surfaceForWindow(). Most platforms will only create the surface via
    \c{VK_KHR_*_surface} when first calling surfaceForWindow(), but there may be
    platform-specific variations in the internal behavior. Once created,
    subsequent calls to surfaceForWindow() just return the same handle. This
    fits the structure of typical Vulkan-enabled QWindow subclasses well.

    To query if a given queue family within a physical device can be used to
    perform presentation to a given surface, call supportsPresent(). This
    encapsulates both the generic \c vkGetPhysicalDeviceSurfaceSupportKHR and
    the WSI-specific \c{vkGetPhysicalDevice*PresentationSupportKHR} checks.

    \section1 Troubleshooting

    Besides returning \c false from create() or \c 0 from surfaceForWindow(),
    critical errors will also get printed to the debug output via qWarning().
    Additional logging can be requested by enabling debug output for the
    logging category \c{qt.vulkan}. The actual Vulkan error code from instance
    creation can be retrieved by calling errorCode() after a failing create().

    In some special cases it may be necessary to override the Vulkan
    library name. This can be achieved by setting the \c{QT_VULKAN_LIB}
    environment variable.

    \section1 Example

    The following is the basic outline of creating a Vulkan-capable QWindow:

    \snippet code/src_gui_vulkan_qvulkaninstance.cpp 3

    \note In addition to expose, a well-behaving window implementation will
    also have to take care of additional events like resize and
    QPlatformSurfaceEvent in order to ensure proper management of the
    swap chain. Additionally, some platforms may require releasing resources
    when not being exposed anymore.

    \section1 Using C++ Bindings for Vulkan

    Combining Qt's Vulkan enablers with a C++ Vulkan wrapper, for example
    \l{https://github.com/KhronosGroup/Vulkan-Hpp}{Vulkan-Hpp}, is possible as
    well. The pre-requisite here is that the C++ layer must be able to adopt
    native handles (VkInstance, VkSurfaceKHR) in its classes without taking
    ownership (since the ownership stays with QVulkanInstance and QWindow).
    Consider also the following:

    \list

    \li Some wrappers require exception support to be enabled. Qt does not use
    exceptions. To enable exceptions for the application, add \c{CONFIG += exceptions}
    to the \c{.pro} file.

    \li Some wrappers call Vulkan functions directly, assuming \c{vulkan.h}
    provides prototypes and the application links to a Vulkan library exporting
    all necessary symbols. Qt may not directly link to a Vulkan library.
    Therefore, on some platforms it may be necessary to add
    \c{LIBS += -lvulkan} or similar in the application's \c{.pro} file.

    \li The headers for the QVulkan classes may include \c{vulkan.h} with
    \c{VK_NO_PROTOTYPES} enabled. This can cause issues in C++ wrapper headers
    that rely on the prototypes. Hence in application code it may be
    necessary to include \c{vulkan.hpp} or similar before any of the QVulkan
    headers.

    \endlist

    \sa QVulkanFunctions, QSurface::SurfaceType
*/

/*!
    \enum QVulkanInstance::Flag
    \since 5.10

    This enum describes the flags that can be passed to setFlags(). These control
    the behavior of create().

    \value NoDebugOutputRedirect Disables Vulkan debug output (\c{VK_EXT_debug_report}) redirection to qDebug.
*/

class QVulkanInstancePrivate
{
public:
    QVulkanInstancePrivate(QVulkanInstance *q)
        : q_ptr(q),
          vkInst(VK_NULL_HANDLE),
          errorCode(VK_SUCCESS)
    { }
    ~QVulkanInstancePrivate() { reset(); }

    bool ensureVulkan();
    void reset();

    QVulkanInstance *q_ptr;
    QScopedPointer<QPlatformVulkanInstance> platformInst;
    VkInstance vkInst;
    QVulkanInstance::Flags flags;
    QByteArrayList layers;
    QByteArrayList extensions;
    QVersionNumber apiVersion;
    VkResult errorCode;
    QScopedPointer<QVulkanFunctions> funcs;
    QHash<VkDevice, QVulkanDeviceFunctions *> deviceFuncs;
    QVector<QVulkanInstance::DebugFilter> debugFilters;
};

bool QVulkanInstancePrivate::ensureVulkan()
{
    if (!platformInst) {
        platformInst.reset(QGuiApplicationPrivate::platformIntegration()->createPlatformVulkanInstance(q_ptr));
        if (!platformInst) {
            qWarning("QVulkanInstance: Failed to initialize Vulkan");
            return false;
        }
    }
    return true;
}

void QVulkanInstancePrivate::reset()
{
    qDeleteAll(deviceFuncs);
    deviceFuncs.clear();
    funcs.reset();
    platformInst.reset();
    vkInst = VK_NULL_HANDLE;
    errorCode = VK_SUCCESS;
}

/*!
    Constructs a new instance.

    \note No Vulkan initialization is performed in the constructor.
 */
QVulkanInstance::QVulkanInstance()
    : d_ptr(new QVulkanInstancePrivate(this))
{
}

/*!
    Destructor.

    \note current() will return \nullptr once the instance is destroyed.
 */
QVulkanInstance::~QVulkanInstance()
{
    destroy();
}

/*!
    \class QVulkanLayer
    \brief Represents information about a Vulkan layer.
 */

/*!
    \variable QVulkanLayer::name
    \brief The name of the layer.
 */

/*!
    \variable QVulkanLayer::version
    \brief The version of the layer. This is an integer, increasing with each backward
    compatible change.
 */

/*!
    \variable QVulkanLayer::specVersion
    \brief The Vulkan version the layer was written against.
 */

/*!
    \variable QVulkanLayer::description
    \brief The description of the layer.
 */

/*!
    \fn bool operator==(const QVulkanLayer &lhs, const QVulkanLayer &rhs)
    \since 5.10
    \relates QVulkanLayer

    Returns \c true if Vulkan layers \a lhs and \a rhs have
    the same name, version, and spec version.
*/

/*!
    \fn bool operator!=(const QVulkanLayer &lhs, const QVulkanLayer &rhs)
    \since 5.10
    \relates QVulkanLayer

    Returns \c true if Vulkan layers \a lhs and \a rhs have
    different name, version, or spec version.
*/

/*!
    \fn uint qHash(const QVulkanLayer &key, uint seed)
    \since 5.10
    \relates QVulkanLayer

    Returns the hash value for the \a key, using \a seed to seed the
    calculation.
*/

/*!
    \class QVulkanExtension
    \brief Represents information about a Vulkan extension.
 */

/*!
    \variable QVulkanExtension::name
    \brief The name of the extension.
 */

/*!
    \variable QVulkanExtension::version
    \brief The version of the extension. This is an integer, increasing with each backward
    compatible change.
 */

/*!
    \fn bool operator==(const QVulkanExtension &lhs, const QVulkanExtension &rhs)
    \since 5.10
    \relates QVulkanExtension

    Returns \c true if Vulkan extensions \a lhs and \a rhs are have the
    same name and version.
*/

/*!
    \fn bool operator!=(const QVulkanExtension &lhs, const QVulkanExtension &rhs)
    \since 5.10
    \relates QVulkanExtension

    Returns \c true if Vulkan extensions \a lhs and \a rhs are have different
    name or version.
*/

/*!
    \fn uint qHash(const QVulkanExtension &key, uint seed)
    \since 5.10
    \relates QVulkanExtension

    Returns the hash value for the \a key, using \a seed to seed the
    calculation.
*/

/*!
    \class QVulkanInfoVector
    \brief A specialized QVector for QVulkanLayer and QVulkanExtension.
 */

/*!
    \fn template<typename T> bool QVulkanInfoVector<T>::contains(const QByteArray &name) const

    \return true if the vector contains a layer or extension with the given \a name.
 */

/*!
    \fn template<typename T> bool QVulkanInfoVector<T>::contains(const QByteArray &name, int minVersion) const

    \return true if the vector contains a layer or extension with the given
    \a name and a version same as or newer than \a minVersion.
 */

/*!
    \return the list of supported instance-level layers.

    \note This function can be called before create().
 */
QVulkanInfoVector<QVulkanLayer> QVulkanInstance::supportedLayers()
{
    return d_ptr->ensureVulkan() ? d_ptr->platformInst->supportedLayers() : QVulkanInfoVector<QVulkanLayer>();
}

/*!
    \return the list of supported instance-level extensions.

    \note This function can be called before create().
 */
QVulkanInfoVector<QVulkanExtension> QVulkanInstance::supportedExtensions()
{
    return d_ptr->ensureVulkan() ? d_ptr->platformInst->supportedExtensions() : QVulkanInfoVector<QVulkanExtension>();
}

/*!
    Makes QVulkanInstance adopt an existing VkInstance handle instead of
    creating a new one.

    \note \a existingVkInstance must have at least \c{VK_KHR_surface} and the
    appropriate WSI-specific \c{VK_KHR_*_surface} extensions enabled. To ensure
    debug output redirection is functional, \c{VK_EXT_debug_report} is needed as
    well.

    \note This function can only be called before create() and has no effect if
    called afterwards.
 */
void QVulkanInstance::setVkInstance(VkInstance existingVkInstance)
{
    if (isValid()) {
        qWarning("QVulkanInstance already created; setVkInstance() has no effect");
        return;
    }

    d_ptr->vkInst = existingVkInstance;
}

/*!
    Configures the behavior of create() based on the provided \a flags.

    \note This function can only be called before create() and has no effect if
    called afterwards.
 */
void QVulkanInstance::setFlags(Flags flags)
{
    if (isValid()) {
        qWarning("QVulkanInstance already created; setFlags() has no effect");
        return;
    }

    d_ptr->flags = flags;
}

/*!
    Specifies the list of instance \a layers to enable. It is safe to specify
    unsupported layers as well because these get ignored when not supported at
    run time.

    \note This function can only be called before create() and has no effect if
    called afterwards.
 */
void QVulkanInstance::setLayers(const QByteArrayList &layers)
{
    if (isValid()) {
        qWarning("QVulkanInstance already created; setLayers() has no effect");
        return;
    }

    d_ptr->layers = layers;
}

/*!
    Specifies the list of additional instance \a extensions to enable. It is
    safe to specify unsupported extensions as well because these get ignored
    when not supported at run time. The surface-related extensions required by
    Qt will always be added automatically, no need to include them in this
    list.

    \note This function can only be called before create() and has no effect if
    called afterwards.
 */
void QVulkanInstance::setExtensions(const QByteArrayList &extensions)
{
    if (isValid()) {
        qWarning("QVulkanInstance already created; setExtensions() has no effect");
        return;
    }

    d_ptr->extensions = extensions;
}

/*!
    Specifies the Vulkan API against which the application expects to run.

    By default no \a vulkanVersion is specified, and so no version check is performed
    during Vulkan instance creation.

    \note This function can only be called before create() and has no effect if
    called afterwards.
 */
void QVulkanInstance::setApiVersion(const QVersionNumber &vulkanVersion)
{
    if (isValid()) {
        qWarning("QVulkanInstance already created; setApiVersion() has no effect");
        return;
    }

    d_ptr->apiVersion = vulkanVersion;
}

/*!
    Initializes the Vulkan library and creates a new or adopts and existing
    Vulkan instance.

    \return true if successful, false on error or when Vulkan is not supported.

    When successful, the pointer to this QVulkanInstance is retrievable via the
    static function current().

    The Vulkan instance and library is available as long as this
    QVulkanInstance exists, or until destroy() is called.
 */
bool QVulkanInstance::create()
{
    if (isValid())
        destroy();

    if (!d_ptr->ensureVulkan())
        return false;

    d_ptr->platformInst->createOrAdoptInstance();

    if (d_ptr->platformInst->isValid()) {
        d_ptr->vkInst = d_ptr->platformInst->vkInstance();
        d_ptr->layers = d_ptr->platformInst->enabledLayers();
        d_ptr->extensions = d_ptr->platformInst->enabledExtensions();
        d_ptr->errorCode = VK_SUCCESS;
        d_ptr->funcs.reset(new QVulkanFunctions(this));
        d_ptr->platformInst->setDebugFilters(d_ptr->debugFilters);
        return true;
    }

    qWarning("Failed to create platform Vulkan instance");
    if (d_ptr->platformInst) {
        d_ptr->errorCode = d_ptr->platformInst->errorCode();
        d_ptr->platformInst.reset();
    } else {
        d_ptr->errorCode = VK_NOT_READY;
    }
    return false;
}

/*!
    Destroys the underlying platform instance, thus destroying the VkInstance
    (when owned). The QVulkanInstance object is still reusable by calling
    create() again.
 */
void QVulkanInstance::destroy()
{
    d_ptr->reset();
}

/*!
    \return true if create() was successful and the instance is valid.
 */
bool QVulkanInstance::isValid() const
{
    return d_ptr->platformInst && d_ptr->platformInst->isValid();
}

/*!
    \return the Vulkan error code after an unsuccessful create(), \c VK_SUCCESS otherwise.

    The value is typically the return value from vkCreateInstance() (when
    creating a new Vulkan instance instead of adopting an existing one), but
    may also be \c VK_NOT_READY if the platform plugin does not support Vulkan.
 */
VkResult QVulkanInstance::errorCode() const
{
    return d_ptr->errorCode;
}

/*!
    \return the VkInstance handle this QVulkanInstance wraps, or \nullptr if
    create() has not yet been successfully called and no existing instance has
    been provided via setVkInstance().
 */
VkInstance QVulkanInstance::vkInstance() const
{
    return d_ptr->vkInst;
}

/*!
    \return the requested flags.
 */
QVulkanInstance::Flags QVulkanInstance::flags() const
{
    return d_ptr->flags;
}

/*!
    \return the enabled instance layers, if create() was called and was successful. The
    requested layers otherwise.
 */
QByteArrayList QVulkanInstance::layers() const
{
    return d_ptr->layers;
}

/*!
    \return the enabled instance extensions, if create() was called and was
    successful. The requested extensions otherwise.
 */
QByteArrayList QVulkanInstance::extensions() const
{
    return d_ptr->extensions;
}

/*!
    \return the requested Vulkan API version against which the application
    expects to run, or a null version number if setApiVersion() was not called
    before create().
 */
QVersionNumber QVulkanInstance::apiVersion() const
{
    return d_ptr->apiVersion;
}

/*!
    Resolves the Vulkan function with the given \a name.

    For core Vulkan commands prefer using the function wrappers retrievable from
    functions() and deviceFunctions() instead.
 */
PFN_vkVoidFunction QVulkanInstance::getInstanceProcAddr(const char *name)
{
    // The return value is PFN_vkVoidFunction instead of QFunctionPointer or
    // similar because on some platforms honoring VKAPI_PTR is important.
    return d_ptr->platformInst->getInstanceProcAddr(name);
}

/*!
    \return the platform Vulkan instance corresponding to this QVulkanInstance.

    \internal
 */
QPlatformVulkanInstance *QVulkanInstance::handle() const
{
    return d_ptr->platformInst.data();
}

/*!
    \return the corresponding QVulkanFunctions object that exposes the core
    Vulkan command set, excluding device level functions, and is guaranteed to
    be functional cross-platform.

    \note The returned object is owned and managed by the QVulkanInstance. Do
    not destroy or alter it.

    \sa deviceFunctions()
 */
QVulkanFunctions *QVulkanInstance::functions() const
{
    return d_ptr->funcs.data();
}

/*!
    \return the QVulkanDeviceFunctions object that exposes the device level
    core Vulkan command set and is guaranteed to be functional cross-platform.

    \note The Vulkan functions in the returned object must only be called with
    \a device or a child object (VkQueue, VkCommandBuffer) of \a device as
    their first parameter. This is because these functions are resolved via
    \l{https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkGetDeviceProcAddr.html}{vkGetDeviceProcAddr}
    in order to avoid the potential overhead of internal dispatching.

    \note The returned object is owned and managed by the QVulkanInstance. Do
    not destroy or alter it.

    \note The object is cached so calling this function with the same \a device
    again is a cheap operation. However, when the device gets destroyed, it is up
    to the application to notify the QVulkanInstance by calling
    resetDeviceFunctions().

    \sa functions(), resetDeviceFunctions()
 */
QVulkanDeviceFunctions *QVulkanInstance::deviceFunctions(VkDevice device)
{
    QVulkanDeviceFunctions *&f = d_ptr->deviceFuncs[device];
    if (!f)
        f = new QVulkanDeviceFunctions(this, device);
    return f;
}

/*!
    Invalidates and destroys the QVulkanDeviceFunctions object for the given
    \a device.

    This function must be called when a VkDevice, for which deviceFunctions()
    was called, gets destroyed while the application intends to continue
    running, possibly creating a new logical Vulkan device later on.

    There is no need to call this before destroying the QVulkanInstance since
    clean up is then performed automatically.

    \sa deviceFunctions()
 */
void QVulkanInstance::resetDeviceFunctions(VkDevice device)
{
    QVulkanDeviceFunctions *&f = d_ptr->deviceFuncs[device];
    delete f;
    f = nullptr;
}

/*!
    Creates or retrieves the already existing \c{VkSurfaceKHR} handle for the
    given \a window.

    \return the Vulkan surface handle or 0 when failed.
 */
VkSurfaceKHR QVulkanInstance::surfaceForWindow(QWindow *window)
{
    QPlatformNativeInterface *nativeInterface = qGuiApp->platformNativeInterface();
    // VkSurfaceKHR is non-dispatchable and maps to a pointer on x64 and a uint64 on x86.
    // Therefore a pointer is returned from the platform plugin, not the value itself.
    void *p = nativeInterface->nativeResourceForWindow(QByteArrayLiteral("vkSurface"), window);
    return p ? *static_cast<VkSurfaceKHR *>(p) : VK_NULL_HANDLE;
}

/*!
    \return true if the queue family with \a queueFamilyIndex within the
    \a physicalDevice supports presenting to \a window.

    Call this function when examining the queues of a given Vulkan device, in
    order to decide which queue can be used for performing presentation.
 */
bool QVulkanInstance::supportsPresent(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, QWindow *window)
{
    return d_ptr->platformInst->supportsPresent(physicalDevice, queueFamilyIndex, window);
}

/*!
    This function should be called by the application's renderer before queuing
    a present operation for \a window.

    While on some platforms this will be a no-op, some may perform windowing
    system dependent synchronization. For example, on Wayland this will
    add send a wl_surface.frame request in order to prevent the driver from
    blocking for minimized windows.

    \since 5.15
 */
void QVulkanInstance::presentAboutToBeQueued(QWindow *window)
{
    d_ptr->platformInst->presentAboutToBeQueued(window);
}

/*!
    This function should be called by the application's renderer after queuing
    a present operation for \a window.

    While on some platforms this will be a no-op, some may perform windowing
    system dependent synchronization. For example, on X11 this will update
    \c{_NET_WM_SYNC_REQUEST_COUNTER}.
 */
void QVulkanInstance::presentQueued(QWindow *window)
{
    d_ptr->platformInst->presentQueued(window);
}

/*!
    \typedef QVulkanInstance::DebugFilter

    Typedef for debug filtering callback functions.

    \sa installDebugOutputFilter(), removeDebugOutputFilter()
 */

/*!
    Installs a \a filter function that is called for every Vulkan debug
    message. When the callback returns \c true, the message is stopped (filtered
    out) and will not appear on the debug output.

    \note Filtering is only effective when NoDebugOutputRedirect is not
    \l{setFlags()}{set}. Installing filters has no effect otherwise.

    \note This function can be called before create().

    \sa removeDebugOutputFilter()
 */
void QVulkanInstance::installDebugOutputFilter(DebugFilter filter)
{
    if (!d_ptr->debugFilters.contains(filter)) {
        d_ptr->debugFilters.append(filter);
        if (d_ptr->platformInst)
            d_ptr->platformInst->setDebugFilters(d_ptr->debugFilters);
    }
}

/*!
    Removes a \a filter function previously installed by
    installDebugOutputFilter().

    \note This function can be called before create().

    \sa installDebugOutputFilter()
 */
void QVulkanInstance::removeDebugOutputFilter(DebugFilter filter)
{
    d_ptr->debugFilters.removeOne(filter);
    if (d_ptr->platformInst)
        d_ptr->platformInst->setDebugFilters(d_ptr->debugFilters);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QVulkanLayer &layer)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QVulkanLayer(" << layer.name << " " << layer.version
                  << " " << layer.specVersion << " " << layer.description << ")";
    return dbg;
}

QDebug operator<<(QDebug dbg, const QVulkanExtension &extension)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QVulkanExtension(" << extension.name << " " << extension.version << ")";
    return dbg;
}
#endif

QT_END_NAMESPACE
