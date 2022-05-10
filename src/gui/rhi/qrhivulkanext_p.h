// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRHIVULKANEXT_P_H
#define QRHIVULKANEXT_P_H

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

#include "qrhivulkan_p.h"

QT_BEGIN_NAMESPACE

#ifndef VK_EXT_vertex_attribute_divisor
#define VK_EXT_vertex_attribute_divisor 1
#define VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_SPEC_VERSION 2
#define VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME "VK_EXT_vertex_attribute_divisor"

typedef struct VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT {
    VkStructureType    sType;
    void*              pNext;
    uint32_t           maxVertexAttribDivisor;
} VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT;

typedef struct VkVertexInputBindingDivisorDescriptionEXT {
    uint32_t    binding;
    uint32_t    divisor;
} VkVertexInputBindingDivisorDescriptionEXT;

typedef struct VkPipelineVertexInputDivisorStateCreateInfoEXT {
    VkStructureType                                     sType;
    const void*                                         pNext;
    uint32_t                                            vertexBindingDivisorCount;
    const VkVertexInputBindingDivisorDescriptionEXT*    pVertexBindingDivisors;
} VkPipelineVertexInputDivisorStateCreateInfoEXT;
#endif // VK_EXT_vertex_attribute_divisor

QT_END_NAMESPACE

#endif
