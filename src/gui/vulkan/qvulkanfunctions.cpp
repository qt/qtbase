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

#include <private/qvulkanfunctions_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QVulkanFunctions
    \since 5.10
    \ingroup painting-3D
    \inmodule QtGui
    \wrapper

    \brief The QVulkanFunctions class provides cross-platform access to the
    instance level core Vulkan 1.0 API.

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

    \code
    void Window::render()
    {
        QVulkanInstance *inst = vulkanInstance();
        QVulkanFunctions *f = inst->functions();
        ...
        VkResult err = f->vkAllocateCommandBuffers(device, &cmdBufInfo, &cmdBuf);
        ...
    }
    \endcode

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

    \sa QVulkanInstance, QVulkanDeviceFunctions, QWindow::setVulkanInstance(), QWindow::setSurfaceType()
*/

/*!
    \class QVulkanDeviceFunctions
    \since 5.10
    \ingroup painting-3D
    \inmodule QtGui
    \wrapper

    \brief The QVulkanDeviceFunctions class provides cross-platform access to
    the device level core Vulkan 1.0 API.

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

    \code
    void Window::render()
    {
        QVulkanInstance *inst = vulkanInstance();
        QVulkanDeviceFunctions *df = inst->deviceFunctions(device);
        VkResult err = df->vkAllocateCommandBuffers(device, &cmdBufInfo, &cmdBuf);
        ...
    }
    \endcode

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
