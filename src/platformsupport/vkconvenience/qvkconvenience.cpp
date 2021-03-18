/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qvkconvenience_p.h"

#include <QOpenGLTexture>

QT_BEGIN_NAMESPACE

/*!
    \class QVkConvenience
    \brief A collection of static helper functions for Vulkan support
    \since 5.14
    \internal
    \ingroup qpa
 */

#if QT_CONFIG(opengl)
VkFormat QVkConvenience::vkFormatFromGlFormat(uint glFormat)
{
    using GlFormat = QOpenGLTexture::TextureFormat;
    switch (glFormat) {
    case GlFormat::NoFormat: return VK_FORMAT_UNDEFINED; // GL_NONE

    // Unsigned normalized formats
    case GlFormat::R8_UNorm: return VK_FORMAT_R8_UNORM; // GL_R8
    case GlFormat::RG8_UNorm: return VK_FORMAT_R8G8_UNORM; // GL_RG8
    case GlFormat::RGB8_UNorm: return VK_FORMAT_R8G8B8_UNORM; // GL_RGB8
    case GlFormat::RGBA8_UNorm: return VK_FORMAT_R8G8B8A8_UNORM; // GL_RGBA8

    case GlFormat::R16_UNorm: return VK_FORMAT_R16_UNORM; // GL_R16
    case GlFormat::RG16_UNorm: return VK_FORMAT_R16G16_UNORM; // GL_RG16
    case GlFormat::RGB16_UNorm: return VK_FORMAT_R16G16B16_UNORM; // GL_RGB16
    case GlFormat::RGBA16_UNorm: return VK_FORMAT_R16G16B16A16_UNORM; // GL_RGBA16

    // Signed normalized formats
    case GlFormat::R8_SNorm: return VK_FORMAT_R8_SNORM; // GL_R8_SNORM
    case GlFormat::RG8_SNorm: return VK_FORMAT_R8G8_SNORM; // GL_RG8_SNORM
    case GlFormat::RGB8_SNorm: return VK_FORMAT_R8G8B8_SNORM; // GL_RGB8_SNORM
    case GlFormat::RGBA8_SNorm: return VK_FORMAT_R8G8B8A8_SNORM; // GL_RGBA8_SNORM

    case GlFormat::R16_SNorm: return VK_FORMAT_R16_SNORM; // GL_R16_SNORM
    case GlFormat::RG16_SNorm: return VK_FORMAT_R16G16_SNORM; // GL_RG16_SNORM
    case GlFormat::RGB16_SNorm: return VK_FORMAT_R16G16B16_SNORM; // GL_RGB16_SNORM
    case GlFormat::RGBA16_SNorm: return VK_FORMAT_R16G16B16A16_SNORM; // GL_RGBA16_SNORM

    // Unsigned integer formats
    case GlFormat::R8U: return VK_FORMAT_R8_UINT; // GL_R8UI
    case GlFormat::RG8U: return VK_FORMAT_R8G8_UINT; // GL_RG8UI
    case GlFormat::RGB8U: return VK_FORMAT_R8G8B8_UINT; // GL_RGB8UI
    case GlFormat::RGBA8U: return VK_FORMAT_R8G8B8A8_UINT; // GL_RGBA8UI

    case GlFormat::R16U: return VK_FORMAT_R16_UINT; // GL_R16UI
    case GlFormat::RG16U: return VK_FORMAT_R16G16_UINT; // GL_RG16UI
    case GlFormat::RGB16U: return VK_FORMAT_R16G16B16_UINT; // GL_RGB16UI
    case GlFormat::RGBA16U: return VK_FORMAT_R16G16B16A16_UINT; // GL_RGBA16UI

    case GlFormat::R32U: return VK_FORMAT_R32_UINT; // GL_R32UI
    case GlFormat::RG32U: return VK_FORMAT_R32G32_UINT; // GL_RG32UI
    case GlFormat::RGB32U: return VK_FORMAT_R32G32B32_UINT; // GL_RGB32UI
    case GlFormat::RGBA32U: return VK_FORMAT_R32G32B32A32_UINT; // GL_RGBA32UI

    // Signed integer formats
    case GlFormat::R8I: return VK_FORMAT_R8_SINT; // GL_R8I
    case GlFormat::RG8I: return VK_FORMAT_R8G8_SINT; // GL_RG8I
    case GlFormat::RGB8I: return VK_FORMAT_R8G8B8_SINT; // GL_RGB8I
    case GlFormat::RGBA8I: return VK_FORMAT_R8G8B8A8_SINT; // GL_RGBA8I

    case GlFormat::R16I: return VK_FORMAT_R16_SINT; // GL_R16I
    case GlFormat::RG16I: return VK_FORMAT_R16G16_SINT; // GL_RG16I
    case GlFormat::RGB16I: return VK_FORMAT_R16G16B16_SINT; // GL_RGB16I
    case GlFormat::RGBA16I: return VK_FORMAT_R16G16B16A16_SINT; // GL_RGBA16I

    case GlFormat::R32I: return VK_FORMAT_R32_SINT; // GL_R32I
    case GlFormat::RG32I: return VK_FORMAT_R32G32_SINT; // GL_RG32I
    case GlFormat::RGB32I: return VK_FORMAT_R32G32B32_SINT; // GL_RGB32I
    case GlFormat::RGBA32I: return VK_FORMAT_R32G32B32A32_SINT; // GL_RGBA32I

    // Floating point formats
    case GlFormat::R16F: return VK_FORMAT_R16_SFLOAT; // GL_R16F
    case GlFormat::RG16F: return VK_FORMAT_R16G16_SFLOAT; // GL_RG16F
    case GlFormat::RGB16F: return VK_FORMAT_R16G16B16_SFLOAT; // GL_RGB16F
    case GlFormat::RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT; // GL_RGBA16F

    case GlFormat::R32F: return VK_FORMAT_R32_SFLOAT; // GL_R32F
    case GlFormat::RG32F: return VK_FORMAT_R32G32_SFLOAT; // GL_RG32F
    case GlFormat::RGB32F: return VK_FORMAT_R32G32B32_SFLOAT; // GL_RGB32F
    case GlFormat::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT; // GL_RGBA32F

    // Packed formats
    case GlFormat::RGB9E5: return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32; // GL_RGB9_E5
    case GlFormat::RG11B10F: return VK_FORMAT_B10G11R11_UFLOAT_PACK32; // GL_R11F_G11F_B10F
//    case GlFormat::RG3B2: return VK_FORMAT_R3_G3_B2; // GL_R3_G3_B2
    case GlFormat::R5G6B5: return VK_FORMAT_R5G6B5_UNORM_PACK16; // GL_RGB565
    case GlFormat::RGB5A1: return VK_FORMAT_R5G5B5A1_UNORM_PACK16; // GL_RGB5_A1
    case GlFormat::RGBA4: return VK_FORMAT_R4G4B4A4_UNORM_PACK16; // GL_RGBA4
    case GlFormat::RGB10A2: return VK_FORMAT_A2R10G10B10_UINT_PACK32; // GL_RGB10_A2UI

    // Depth formats
//    case Format::D16: return VK_FORMAT_DEPTH_COMPONENT16; // GL_DEPTH_COMPONENT16
//    case Format::D24: return VK_FORMAT_DEPTH_COMPONENT24; // GL_DEPTH_COMPONENT24
//    case Format::D24S8: return VK_FORMAT_DEPTH24_STENCIL8; // GL_DEPTH24_STENCIL8
//    case Format::D32: return VK_FORMAT_DEPTH_COMPONENT32; // GL_DEPTH_COMPONENT32
//    case Format::D32F: return VK_FORMAT_DEPTH_COMPONENT32F; // GL_DEPTH_COMPONENT32F
//    case Format::D32FS8X24: return VK_FORMAT_DEPTH32F_STENCIL8; // GL_DEPTH32F_STENCIL8
//    case Format::S8: return VK_FORMAT_STENCIL_INDEX8; // GL_STENCIL_INDEX8

    // Compressed formats
    case GlFormat::RGB_DXT1: return VK_FORMAT_BC1_RGB_UNORM_BLOCK; // GL_COMPRESSED_RGB_S3TC_DXT1_EXT
    case GlFormat::RGBA_DXT1: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK; // GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
    case GlFormat::RGBA_DXT3: return VK_FORMAT_BC2_UNORM_BLOCK; // GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
    case GlFormat::RGBA_DXT5: return VK_FORMAT_BC3_UNORM_BLOCK; // GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
    case GlFormat::R_ATI1N_UNorm: return VK_FORMAT_BC4_UNORM_BLOCK; // GL_COMPRESSED_RED_RGTC1
    case GlFormat::R_ATI1N_SNorm: return VK_FORMAT_BC4_SNORM_BLOCK; // GL_COMPRESSED_SIGNED_RED_RGTC1
    case GlFormat::RG_ATI2N_UNorm: return VK_FORMAT_BC5_UNORM_BLOCK; // GL_COMPRESSED_RG_RGTC2
    case GlFormat::RG_ATI2N_SNorm: return VK_FORMAT_BC5_SNORM_BLOCK; // GL_COMPRESSED_SIGNED_RG_RGTC2
    case GlFormat::RGB_BP_UNSIGNED_FLOAT: return VK_FORMAT_BC6H_UFLOAT_BLOCK; // GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB
    case GlFormat::RGB_BP_SIGNED_FLOAT: return VK_FORMAT_BC6H_SFLOAT_BLOCK; // GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB
    case GlFormat::RGB_BP_UNorm: return VK_FORMAT_BC7_UNORM_BLOCK; // GL_COMPRESSED_RGBA_BPTC_UNORM_ARB
    case GlFormat::R11_EAC_UNorm: return VK_FORMAT_EAC_R11_UNORM_BLOCK; // GL_COMPRESSED_R11_EAC
    case GlFormat::R11_EAC_SNorm: return VK_FORMAT_EAC_R11_SNORM_BLOCK; // GL_COMPRESSED_SIGNED_R11_EAC
    case GlFormat::RG11_EAC_UNorm: return VK_FORMAT_EAC_R11G11_UNORM_BLOCK; // GL_COMPRESSED_RG11_EAC
    case GlFormat::RG11_EAC_SNorm: return VK_FORMAT_EAC_R11G11_SNORM_BLOCK; // GL_COMPRESSED_SIGNED_RG11_EAC
    case GlFormat::RGB8_ETC2: return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK; // GL_COMPRESSED_RGB8_ETC2
    case GlFormat::SRGB8_ETC2: return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK; // GL_COMPRESSED_SRGB8_ETC2
    case GlFormat::RGB8_PunchThrough_Alpha1_ETC2: return VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK; // GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2
    case GlFormat::SRGB8_PunchThrough_Alpha1_ETC2: return VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK; // GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2
    case GlFormat::RGBA8_ETC2_EAC: return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK; // GL_COMPRESSED_RGBA8_ETC2_EAC
    case GlFormat::SRGB8_Alpha8_ETC2_EAC: return VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK; // GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC
//    case GlFormat::RGB8_ETC1: return VK_FORMAT_ETC1_RGB8_OES; // GL_ETC1_RGB8_OES
    case GlFormat::RGBA_ASTC_4x4: return VK_FORMAT_ASTC_4x4_UNORM_BLOCK; // GL_COMPRESSED_RGBA_ASTC_4x4_KHR
    case GlFormat::RGBA_ASTC_5x4: return VK_FORMAT_ASTC_5x4_UNORM_BLOCK; // GL_COMPRESSED_RGBA_ASTC_5x4_KHR
    case GlFormat::RGBA_ASTC_5x5: return VK_FORMAT_ASTC_5x5_UNORM_BLOCK; // GL_COMPRESSED_RGBA_ASTC_5x5_KHR
    case GlFormat::RGBA_ASTC_6x5: return VK_FORMAT_ASTC_6x5_UNORM_BLOCK; // GL_COMPRESSED_RGBA_ASTC_6x5_KHR
    case GlFormat::RGBA_ASTC_6x6: return VK_FORMAT_ASTC_6x6_UNORM_BLOCK; // GL_COMPRESSED_RGBA_ASTC_6x6_KHR
    case GlFormat::RGBA_ASTC_8x5: return VK_FORMAT_ASTC_8x5_UNORM_BLOCK; // GL_COMPRESSED_RGBA_ASTC_8x5_KHR
    case GlFormat::RGBA_ASTC_8x6: return VK_FORMAT_ASTC_8x6_UNORM_BLOCK; // GL_COMPRESSED_RGBA_ASTC_8x6_KHR
    case GlFormat::RGBA_ASTC_8x8: return VK_FORMAT_ASTC_8x8_UNORM_BLOCK; // GL_COMPRESSED_RGBA_ASTC_8x8_KHR
    case GlFormat::RGBA_ASTC_10x5: return VK_FORMAT_ASTC_10x5_UNORM_BLOCK; // GL_COMPRESSED_RGBA_ASTC_10x5_KHR
    case GlFormat::RGBA_ASTC_10x6: return VK_FORMAT_ASTC_10x6_UNORM_BLOCK; // GL_COMPRESSED_RGBA_ASTC_10x6_KHR
    case GlFormat::RGBA_ASTC_10x8: return VK_FORMAT_ASTC_10x8_UNORM_BLOCK; // GL_COMPRESSED_RGBA_ASTC_10x8_KHR
    case GlFormat::RGBA_ASTC_10x10: return VK_FORMAT_ASTC_10x10_UNORM_BLOCK; // GL_COMPRESSED_RGBA_ASTC_10x10_KHR
    case GlFormat::RGBA_ASTC_12x10: return VK_FORMAT_ASTC_12x10_UNORM_BLOCK; // GL_COMPRESSED_RGBA_ASTC_12x10_KHR
    case GlFormat::RGBA_ASTC_12x12: return VK_FORMAT_ASTC_12x12_UNORM_BLOCK; // GL_COMPRESSED_RGBA_ASTC_12x12_KHR
    case GlFormat::SRGB8_Alpha8_ASTC_4x4: return VK_FORMAT_ASTC_4x4_SRGB_BLOCK; // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR
    case GlFormat::SRGB8_Alpha8_ASTC_5x4: return VK_FORMAT_ASTC_5x4_SRGB_BLOCK; // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR
    case GlFormat::SRGB8_Alpha8_ASTC_5x5: return VK_FORMAT_ASTC_5x5_SRGB_BLOCK; // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR
    case GlFormat::SRGB8_Alpha8_ASTC_6x5: return VK_FORMAT_ASTC_6x5_SRGB_BLOCK; // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR
    case GlFormat::SRGB8_Alpha8_ASTC_6x6: return VK_FORMAT_ASTC_6x6_SRGB_BLOCK; // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR
    case GlFormat::SRGB8_Alpha8_ASTC_8x5: return VK_FORMAT_ASTC_8x5_SRGB_BLOCK; // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR
    case GlFormat::SRGB8_Alpha8_ASTC_8x6: return VK_FORMAT_ASTC_8x6_SRGB_BLOCK; // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR
    case GlFormat::SRGB8_Alpha8_ASTC_8x8: return VK_FORMAT_ASTC_8x8_SRGB_BLOCK; // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR
    case GlFormat::SRGB8_Alpha8_ASTC_10x5: return VK_FORMAT_ASTC_10x5_SRGB_BLOCK; // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR
    case GlFormat::SRGB8_Alpha8_ASTC_10x6: return VK_FORMAT_ASTC_10x6_SRGB_BLOCK; // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR
    case GlFormat::SRGB8_Alpha8_ASTC_10x8: return VK_FORMAT_ASTC_10x8_SRGB_BLOCK; // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR
    case GlFormat::SRGB8_Alpha8_ASTC_10x10: return VK_FORMAT_ASTC_10x10_SRGB_BLOCK; // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR
    case GlFormat::SRGB8_Alpha8_ASTC_12x10: return VK_FORMAT_ASTC_12x10_SRGB_BLOCK; // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR
    case GlFormat::SRGB8_Alpha8_ASTC_12x12: return VK_FORMAT_ASTC_12x12_SRGB_BLOCK; // GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR

    // sRGB formats
    case GlFormat::SRGB8: return VK_FORMAT_R8G8B8_SRGB; // GL_SRGB8
    case GlFormat::SRGB8_Alpha8: return VK_FORMAT_R8G8B8A8_SRGB; // GL_SRGB8_ALPHA8
    case GlFormat::SRGB_DXT1: return VK_FORMAT_BC1_RGB_SRGB_BLOCK; // GL_COMPRESSED_SRGB_S3TC_DXT1_EXT
    case GlFormat::SRGB_Alpha_DXT1: return VK_FORMAT_BC1_RGBA_SRGB_BLOCK; // GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT
    case GlFormat::SRGB_Alpha_DXT3: return VK_FORMAT_BC2_SRGB_BLOCK; // GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT
    case GlFormat::SRGB_Alpha_DXT5: return VK_FORMAT_BC3_SRGB_BLOCK; // GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT
    case GlFormat::SRGB_BP_UNorm: return VK_FORMAT_BC7_SRGB_BLOCK; // GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB

    // ES 2 formats
//    case GlFormat::DepthFormat: return VK_FORMAT_DEPTH_COMPONENT; // GL_DEPTH_COMPONENT
//    case GlFormat::AlphaFormat: return VK_FORMAT_ALPHA; // GL_ALPHA
//    case GlFormat::RGBFormat: return VK_FORMAT_RGB; // GL_RGB
//    case GlFormat::RGBAFormat: return VK_FORMAT_RGBA; // GL_RGBA
//    case GlFormat::LuminanceFormat: return VK_FORMAT_LUMINANCE; // GL_LUMINANCE
//    case GlFormat::LuminanceAlphaFormat: return VK_FORMAT;
    default: return VK_FORMAT_UNDEFINED;
    }
}
#endif

QT_END_NAMESPACE
