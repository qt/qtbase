/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#ifndef QCOCOAVULKANINSTANCE_H
#define QCOCOAVULKANINSTANCE_H

// Include mvk_vulkan.h first. The order is important since
// mvk_vulkan.h just defines VK_USE_PLATFORM_MACOS_MVK (or the IOS
// variant) and includes vulkan.h. If something else included vulkan.h
// before this then we wouldn't get the MVK specifics...
#include <MoltenVK/mvk_vulkan.h>

#include <QtCore/QHash>
#include <QtVulkanSupport/private/qbasicvulkanplatforminstance_p.h>

#include <AppKit/AppKit.h>

QT_BEGIN_NAMESPACE

class QCocoaVulkanInstance : public QBasicPlatformVulkanInstance
{
public:
    QCocoaVulkanInstance(QVulkanInstance *instance);
    ~QCocoaVulkanInstance();

    void createOrAdoptInstance() override;

    VkSurfaceKHR *surface(QWindow *window);

private:
    VkSurfaceKHR createSurface(NSView *view);

    QVulkanInstance *m_instance = nullptr;
    QLibrary m_lib;
    VkSurfaceKHR m_nullSurface = nullptr;
    PFN_vkCreateMacOSSurfaceMVK m_createSurface = nullptr;
};

QT_END_NAMESPACE

#endif // QXCBVULKANINSTANCE_H
