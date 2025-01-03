// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qvulkanfunctions_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QVulkanFunctions
    \since 5.10
    \ingroup painting-3D
    \inmodule QtGui
    \wrapper

    \brief The QVulkanFunctions class provides cross-platform access to the
    instance level core Vulkan 1.2 API.

    Qt and Qt applications do not link to any Vulkan libraries by default.
    Instead, all functions are resolved dynamically at run time. Each
    QVulkanInstance provides a QVulkanFunctions object retrievable via
    QVulkanInstance::functions(). This does not contain device level functions
    in order to avoid the potential overhead of an internal dispatching.
    Instead, functions that rely on a device, or a dispatchable child object of
    a device, are exposed via QVulkanDeviceFunctions and
    QVulkanInstance::deviceFunctions(). QVulkanFunctions and
    QVulkanDeviceFunctions together provides access to the full core Vulkan
    API, excluding any extensions.

    \note QVulkanFunctions instances cannot be constructed directly.

    The typical usage is the following:

    \snippet code/src_gui_vulkan_qvulkanfunctions.cpp 0

    \note Windowing system interface (WSI) specifics and extensions are
    excluded. This class only covers core Vulkan commands, with the exception
    of instance creation, destruction, and function resolving, since such
    functionality is covered by QVulkanInstance itself.

    To access additional functions, applications can use
    QVulkanInstance::getInstanceProcAddr() and vkGetDeviceProcAddr().
    Applications can also decide to link to a Vulkan library directly, as
    platforms with an appropriate loader will typically export function symbols
    for the core commands. See
    \l{https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkGetInstanceProcAddr.html}{the
    man page for vkGetInstanceProcAddr} for more information.

    \note The member function prototypes for Vulkan 1.1 and 1.2 commands are
    ifdefed with the appropriate \c{VK_VERSION_1_x} that is defined by the
    Vulkan headers. Therefore these functions will only be callable by an
    application when the system's (on which the application is built) Vulkan
    header is new enough and it contains 1.1 and 1.2 Vulkan API definitions.
    When building Qt from source, this has an additional consequence: the
    Vulkan headers on the build environment must also be 1.1 and 1.2 capable in
    order to get a Qt build that supports resolving the 1.1 and 1.2 API
    commands. If either of these conditions is not met, applications will only
    be able to call the Vulkan 1.0 commands through QVulkanFunctions and
    QVulkanDeviceFunctions.

    \sa QVulkanInstance, QVulkanDeviceFunctions, QWindow::setVulkanInstance(), QWindow::setSurfaceType()
*/

/*!
    \class QVulkanDeviceFunctions
    \since 5.10
    \ingroup painting-3D
    \inmodule QtGui
    \wrapper

    \brief The QVulkanDeviceFunctions class provides cross-platform access to
    the device level core Vulkan 1.2 API.

    Qt and Qt applications do not link to any Vulkan libraries by default.
    Instead, all functions are resolved dynamically at run time. Each
    QVulkanInstance provides a QVulkanFunctions object retrievable via
    QVulkanInstance::functions(). This does not contain device level functions
    in order to avoid the potential overhead of an internal dispatching.
    Instead, functions that rely on a device, or a dispatchable child object of
    a device, are exposed via QVulkanDeviceFunctions and
    QVulkanInstance::deviceFunctions(). QVulkanFunctions and
    QVulkanDeviceFunctions together provides access to the full core Vulkan
    API, excluding any extensions.

    \note QVulkanDeviceFunctions instances cannot be constructed directly.

    The typical usage is the following:

    \snippet code/src_gui_vulkan_qvulkanfunctions.cpp 1

    The QVulkanDeviceFunctions object specific to the provided VkDevice is
    created when QVulkanInstance::deviceFunctions() is first called with the
    device in question. The object is then cached internally.

    To access additional functions, applications can use
    QVulkanInstance::getInstanceProcAddr() and vkGetDeviceProcAddr().
    Applications can also decide to link to a Vulkan library directly, as many
    implementations export function symbols for the core commands. See
    \l{https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkGetInstanceProcAddr.html}{the
    man page for vkGetInstanceProcAddr} for more information.

    \sa QVulkanInstance, QVulkanFunctions, QWindow::setVulkanInstance(), QWindow::setSurfaceType()
*/

/*
   Constructs a new QVulkanFunctions for \a inst.
   \internal
 */
QVulkanFunctions::QVulkanFunctions(QVulkanInstance *inst)
    : d_ptr(new QVulkanFunctionsPrivate(inst))
{
}

/*
   Destructor.
 */
QVulkanFunctions::~QVulkanFunctions()
{
}

/*
   Constructs a new QVulkanDeviceFunctions for \a inst and the given \a device.
   \internal
 */
QVulkanDeviceFunctions::QVulkanDeviceFunctions(QVulkanInstance *inst, VkDevice device)
    : d_ptr(new QVulkanDeviceFunctionsPrivate(inst, device))
{
}

/*
   Destructor.
 */
QVulkanDeviceFunctions::~QVulkanDeviceFunctions()
{
}

QT_END_NAMESPACE
