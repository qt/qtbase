/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QOPENGL_EXTENSIONS_P_H
#define QOPENGL_EXTENSIONS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Qt OpenGL classes.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "qopenglfunctions.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#if 0
#ifndef GL_ARB_vertex_buffer_object
typedef ptrdiff_t GLintptrARB;
typedef ptrdiff_t GLsizeiptrARB;
#endif
#endif

#ifndef GL_VERSION_2_0
typedef char GLchar;
#endif

class QOpenGLExtensionsPrivate;

class Q_GUI_EXPORT QOpenGLExtensions : public QOpenGLFunctions
{
    Q_DECLARE_PRIVATE(QOpenGLExtensions)
public:
    QOpenGLExtensions();
    QOpenGLExtensions(QOpenGLContext *context);
    ~QOpenGLExtensions() {}

    enum OpenGLExtension {
        TextureRectangle        = 0x00000001,
        GenerateMipmap          = 0x00000002,
        TextureCompression      = 0x00000004,
        MirroredRepeat          = 0x00000008,
        FramebufferMultisample  = 0x00000010,
        StencilTwoSide          = 0x00000020,
        StencilWrap             = 0x00000040,
        PackedDepthStencil      = 0x00000080,
        NVFloatBuffer           = 0x00000100,
        PixelBufferObject       = 0x00000200,
        FramebufferBlit         = 0x00000400,
        BGRATextureFormat       = 0x00000800,
        DDSTextureCompression   = 0x00001000,
        ETC1TextureCompression  = 0x00002000,
        PVRTCTextureCompression = 0x00004000,
        ElementIndexUint        = 0x00008000,
        Depth24                 = 0x00010000,
        SRGBFrameBuffer         = 0x00020000,
        MapBuffer               = 0x00040000,
        GeometryShaders         = 0x00080000
    };
    Q_DECLARE_FLAGS(OpenGLExtensions, OpenGLExtension)

    OpenGLExtensions openGLExtensions();
    bool hasOpenGLExtension(QOpenGLExtensions::OpenGLExtension extension) const;

    void initializeGLExtensions();

    GLvoid *glMapBuffer(GLenum target, GLenum access);
    GLboolean glUnmapBuffer(GLenum target);

    void glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                           GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                           GLbitfield mask, GLenum filter);

    void glRenderbufferStorageMultisample(GLenum target, GLsizei samples,
                                          GLenum internalFormat,
                                          GLsizei width, GLsizei height);

    void glGetBufferSubData(GLenum target, qopengl_GLintptr offset, qopengl_GLsizeiptr size, GLvoid *data);

private:
    static bool isInitialized(const QOpenGLFunctionsPrivate *d) { return d != 0; }
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QOpenGLExtensions::OpenGLExtensions)

class QOpenGLExtensionsPrivate : public QOpenGLFunctionsPrivate
{
public:
    explicit QOpenGLExtensionsPrivate(QOpenGLContext *ctx);

    GLvoid* (QOPENGLF_APIENTRYP MapBuffer)(GLenum target, GLenum access);
    GLboolean (QOPENGLF_APIENTRYP UnmapBuffer)(GLenum target);
    void (QOPENGLF_APIENTRYP BlitFramebuffer)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                           GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                           GLbitfield mask, GLenum filter);
    void (QOPENGLF_APIENTRYP RenderbufferStorageMultisample)(GLenum target, GLsizei samples,
                                          GLenum internalFormat,
                                          GLsizei width, GLsizei height);
    void (QOPENGLF_APIENTRYP GetBufferSubData)(GLenum target, qopengl_GLintptr offset, qopengl_GLsizeiptr size, GLvoid *data);
};

inline GLvoid *QOpenGLExtensions::glMapBuffer(GLenum target, GLenum access)
{
    Q_D(QOpenGLExtensions);
    Q_ASSERT(QOpenGLExtensions::isInitialized(d));
    GLvoid *result = d->MapBuffer(target, access);
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline GLboolean QOpenGLExtensions::glUnmapBuffer(GLenum target)
{
    Q_D(QOpenGLExtensions);
    Q_ASSERT(QOpenGLExtensions::isInitialized(d));
    GLboolean result = d->UnmapBuffer(target);
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLExtensions::glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                       GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                       GLbitfield mask, GLenum filter)
{
    Q_D(QOpenGLExtensions);
    Q_ASSERT(QOpenGLExtensions::isInitialized(d));
    d->BlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtensions::glRenderbufferStorageMultisample(GLenum target, GLsizei samples,
                                      GLenum internalFormat,
                                      GLsizei width, GLsizei height)
{
    Q_D(QOpenGLExtensions);
    Q_ASSERT(QOpenGLExtensions::isInitialized(d));
    d->RenderbufferStorageMultisample(target, samples, internalFormat, width, height);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtensions::glGetBufferSubData(GLenum target, qopengl_GLintptr offset, qopengl_GLsizeiptr size, GLvoid *data)
{
    Q_D(QOpenGLExtensions);
    Q_ASSERT(QOpenGLExtensions::isInitialized(d));
    d->GetBufferSubData(target, offset, size, data);
    Q_OPENGL_FUNCTIONS_DEBUG
}

#ifndef GL_FRAMEBUFFER_SRGB_CAPABLE
#define GL_FRAMEBUFFER_SRGB_CAPABLE 0x8DBA
#endif
#ifndef GL_FRAMEBUFFER_SRGB
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#endif
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_TEXTURE_RECTANGLE
#define GL_TEXTURE_RECTANGLE 0x84F5
#endif
#ifndef GL_TEXTURE_BINDING_RECTANGLE
#define GL_TEXTURE_BINDING_RECTANGLE 0x84F6
#endif
#ifndef GL_PROXY_TEXTURE_RECTANGLE
#define GL_PROXY_TEXTURE_RECTANGLE 0x84F7
#endif
#ifndef GL_MAX_RECTANGLE_TEXTURE_SIZE
#define GL_MAX_RECTANGLE_TEXTURE_SIZE 0x84F8
#endif
#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif
#ifndef GL_RGB16
#define GL_RGB16 0x8054
#endif
#ifndef GL_UNSIGNED_SHORT_5_6_5
#define GL_UNSIGNED_SHORT_5_6_5 0x8363
#endif
#ifndef GL_UNSIGNED_INT_8_8_8_8_REV
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#endif
#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_MIRRORED_REPEAT
#define GL_MIRRORED_REPEAT 0x8370
#endif
#ifndef GL_GENERATE_MIPMAP
#define GL_GENERATE_MIPMAP 0x8191
#endif
#ifndef GL_GENERATE_MIPMAP_HINT
#define GL_GENERATE_MIPMAP_HINT 0x8192
#endif
#ifndef GL_FRAGMENT_PROGRAM
#define GL_FRAGMENT_PROGRAM 0x8804
#endif
#ifndef GL_PROGRAM_FORMAT_ASCII
#define GL_PROGRAM_FORMAT_ASCII 0x8875
#endif
#ifndef GL_PIXEL_UNPACK_BUFFER
#define GL_PIXEL_UNPACK_BUFFER 0x88EC
#endif
#ifndef GL_WRITE_ONLY
#define GL_WRITE_ONLY 0x88B9
#endif
#ifndef GL_STREAM_DRAW
#define GL_STREAM_DRAW 0x88E0
#endif
#ifndef GL_STENCIL_TEST_TWO_SIDE
#define GL_STENCIL_TEST_TWO_SIDE 0x8910
#endif
#ifndef GL_INCR_WRAP
#define GL_INCR_WRAP 0x8507
#endif
#ifndef GL_DECR_WRAP
#define GL_DECR_WRAP 0x8508
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_TEXTURE1
#define GL_TEXTURE1 0x84C1
#endif
#ifndef GL_DEPTH_COMPONENT16
#define GL_DEPTH_COMPONENT16 0x81A5
#endif
#ifndef GL_DEPTH_COMPONENT24
#define GL_DEPTH_COMPONENT24 0x81A6
#endif
#ifndef GL_INVALID_FRAMEBUFFER_OPERATION
#define GL_INVALID_FRAMEBUFFER_OPERATION 0x0506
#endif
#ifndef GL_MAX_RENDERBUFFER_SIZE
#define GL_MAX_RENDERBUFFER_SIZE 0x84E8
#endif
#ifndef GL_FRAMEBUFFER_BINDING
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#endif
#ifndef GL_RENDERBUFFER_BINDING
#define GL_RENDERBUFFER_BINDING 0x8CA7
#endif
#ifndef GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE 0x8CD0
#endif
#ifndef GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME 0x8CD1
#endif
#ifndef GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL 0x8CD2
#endif
#ifndef GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE 0x8CD3
#endif
#ifndef GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET 0x8CD4
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT 0x8CD6
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT 0x8CD7
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT
#define GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT 0x8CD8
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS 0x8CD9
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_FORMATS
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS 0x8CDA
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER 0x8CDB
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER 0x8CDC
#endif
#ifndef GL_FRAMEBUFFER_UNSUPPORTED
#define GL_FRAMEBUFFER_UNSUPPORTED 0x8CDD
#endif
#ifndef GL_MAX_COLOR_ATTACHMENTS
#define GL_MAX_COLOR_ATTACHMENTS 0x8CDF
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#endif
#ifndef GL_COLOR_ATTACHMENT1
#define GL_COLOR_ATTACHMENT1 0x8CE1
#endif
#ifndef GL_COLOR_ATTACHMENT2
#define GL_COLOR_ATTACHMENT2 0x8CE2
#endif
#ifndef GL_COLOR_ATTACHMENT3
#define GL_COLOR_ATTACHMENT3 0x8CE3
#endif
#ifndef GL_COLOR_ATTACHMENT4
#define GL_COLOR_ATTACHMENT4 0x8CE4
#endif
#ifndef GL_COLOR_ATTACHMENT5
#define GL_COLOR_ATTACHMENT5 0x8CE5
#endif
#ifndef GL_COLOR_ATTACHMENT6
#define GL_COLOR_ATTACHMENT6 0x8CE6
#endif
#ifndef GL_COLOR_ATTACHMENT7
#define GL_COLOR_ATTACHMENT7 0x8CE7
#endif
#ifndef GL_COLOR_ATTACHMENT8
#define GL_COLOR_ATTACHMENT8 0x8CE8
#endif
#ifndef GL_COLOR_ATTACHMENT9
#define GL_COLOR_ATTACHMENT9 0x8CE9
#endif
#ifndef GL_COLOR_ATTACHMENT10
#define GL_COLOR_ATTACHMENT10 0x8CEA
#endif
#ifndef GL_COLOR_ATTACHMENT11
#define GL_COLOR_ATTACHMENT11 0x8CEB
#endif
#ifndef GL_COLOR_ATTACHMENT12
#define GL_COLOR_ATTACHMENT12 0x8CEC
#endif
#ifndef GL_COLOR_ATTACHMENT13
#define GL_COLOR_ATTACHMENT13 0x8CED
#endif
#ifndef GL_COLOR_ATTACHMENT14
#define GL_COLOR_ATTACHMENT14 0x8CEE
#endif
#ifndef GL_COLOR_ATTACHMENT15
#define GL_COLOR_ATTACHMENT15 0x8CEF
#endif
#ifndef GL_DEPTH_ATTACHMENT
#define GL_DEPTH_ATTACHMENT 0x8D00
#endif
#ifndef GL_STENCIL_ATTACHMENT
#define GL_STENCIL_ATTACHMENT 0x8D20
#endif
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif
#ifndef GL_RENDERBUFFER
#define GL_RENDERBUFFER 0x8D41
#endif
#ifndef GL_RENDERBUFFER_WIDTH
#define GL_RENDERBUFFER_WIDTH 0x8D42
#endif
#ifndef GL_RENDERBUFFER_HEIGHT
#define GL_RENDERBUFFER_HEIGHT 0x8D43
#endif
#ifndef GL_RENDERBUFFER_INTERNAL_FORMAT
#define GL_RENDERBUFFER_INTERNAL_FORMAT 0x8D44
#endif
#ifndef GL_STENCIL_INDEX
#define GL_STENCIL_INDEX 0x8D45
#endif
#ifndef GL_STENCIL_INDEX1
#define GL_STENCIL_INDEX1 0x8D46
#endif
#ifndef GL_STENCIL_INDEX4
#define GL_STENCIL_INDEX4 0x8D47
#endif
#ifndef GL_STENCIL_INDEX8
#define GL_STENCIL_INDEX8 0x8D48
#endif
#ifndef GL_STENCIL_INDEX16
#define GL_STENCIL_INDEX16 0x8D49
#endif
#ifndef GL_RENDERBUFFER_RED_SIZE
#define GL_RENDERBUFFER_RED_SIZE 0x8D50
#endif
#ifndef GL_RENDERBUFFER_GREEN_SIZE
#define GL_RENDERBUFFER_GREEN_SIZE 0x8D51
#endif
#ifndef GL_RENDERBUFFER_BLUE_SIZE
#define GL_RENDERBUFFER_BLUE_SIZE 0x8D52
#endif
#ifndef GL_RENDERBUFFER_ALPHA_SIZE
#define GL_RENDERBUFFER_ALPHA_SIZE 0x8D53
#endif
#ifndef GL_RENDERBUFFER_DEPTH_SIZE
#define GL_RENDERBUFFER_DEPTH_SIZE 0x8D54
#endif
#ifndef GL_RENDERBUFFER_STENCIL_SIZE
#define GL_RENDERBUFFER_STENCIL_SIZE 0x8D55
#endif
#ifndef GL_READ_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER 0x8CA8
#endif
#ifndef GL_RENDERBUFFER_SAMPLES
#define GL_RENDERBUFFER_SAMPLES 0x8CAB
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE 0x8D56
#endif
#ifndef GL_MAX_SAMPLES
#define GL_MAX_SAMPLES 0x8D57
#endif
#ifndef GL_DRAW_FRAMEBUFFER
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#endif
#ifndef GL_DEPTH_STENCIL
#define GL_DEPTH_STENCIL 0x84F9
#endif
#ifndef GL_UNSIGNED_INT_24_8
#define GL_UNSIGNED_INT_24_8 0x84FA
#endif
#ifndef GL_DEPTH24_STENCIL8
#define GL_DEPTH24_STENCIL8 0x88F0
#endif
#ifndef GL_TEXTURE_STENCIL_SIZE
#define GL_TEXTURE_STENCIL_SIZE 0x88F1
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_PACK_SKIP_IMAGES
#define GL_PACK_SKIP_IMAGES 0x806B
#endif
#ifndef GL_PACK_IMAGE_HEIGHT
#define GL_PACK_IMAGE_HEIGHT 0x806C
#endif
#ifndef GL_UNPACK_SKIP_IMAGES
#define GL_UNPACK_SKIP_IMAGES 0x806D
#endif
#ifndef GL_UNPACK_IMAGE_HEIGHT
#define GL_UNPACK_IMAGE_HEIGHT 0x806E
#endif
#ifndef GL_CONSTANT_COLOR
#define GL_CONSTANT_COLOR 0x8001
#endif
#ifndef GL_ONE_MINUS_CONSTANT_COLOR
#define GL_ONE_MINUS_CONSTANT_COLOR 0x8002
#endif
#ifndef GL_CONSTANT_ALPHA
#define GL_CONSTANT_ALPHA 0x8003
#endif
#ifndef GL_ONE_MINUS_CONSTANT_ALPHA
#define GL_ONE_MINUS_CONSTANT_ALPHA 0x8004
#endif
#ifndef GL_INCR_WRAP
#define GL_INCR_WRAP 0x8507
#endif
#ifndef GL_DECR_WRAP
#define GL_DECR_WRAP 0x8508
#endif
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#endif
#ifndef GL_STREAM_DRAW
#define GL_STREAM_DRAW 0x88E0
#endif
#ifndef GL_STREAM_READ
#define GL_STREAM_READ 0x88E1
#endif
#ifndef GL_STREAM_COPY
#define GL_STREAM_COPY 0x88E2
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_STATIC_READ
#define GL_STATIC_READ 0x88E5
#endif
#ifndef GL_STATIC_COPY
#define GL_STATIC_COPY 0x88E6
#endif
#ifndef GL_DYNAMIC_DRAW
#define GL_DYNAMIC_DRAW 0x88E8
#endif
#ifndef GL_DYNAMIC_READ
#define GL_DYNAMIC_READ 0x88E9
#endif
#ifndef GL_DYNAMIC_COPY
#define GL_DYNAMIC_COPY 0x88EA
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_FLOAT_VEC2
#define GL_FLOAT_VEC2 0x8B50
#endif
#ifndef GL_FLOAT_VEC3
#define GL_FLOAT_VEC3 0x8B51
#endif
#ifndef GL_FLOAT_VEC4
#define GL_FLOAT_VEC4 0x8B52
#endif
#ifndef GL_INT_VEC2
#define GL_INT_VEC2 0x8B53
#endif
#ifndef GL_INT_VEC3
#define GL_INT_VEC3 0x8B54
#endif
#ifndef GL_INT_VEC4
#define GL_INT_VEC4 0x8B55
#endif
#ifndef GL_BOOL
#define GL_BOOL 0x8B56
#endif
#ifndef GL_BOOL_VEC2
#define GL_BOOL_VEC2 0x8B57
#endif
#ifndef GL_BOOL_VEC3
#define GL_BOOL_VEC3 0x8B58
#endif
#ifndef GL_BOOL_VEC4
#define GL_BOOL_VEC4 0x8B59
#endif
#ifndef GL_FLOAT_MAT2
#define GL_FLOAT_MAT2 0x8B5A
#endif
#ifndef GL_FLOAT_MAT3
#define GL_FLOAT_MAT3 0x8B5B
#endif
#ifndef GL_FLOAT_MAT4
#define GL_FLOAT_MAT4 0x8B5C
#endif
#ifndef GL_SAMPLER_1D
#define GL_SAMPLER_1D 0x8B5D
#endif
#ifndef GL_SAMPLER_2D
#define GL_SAMPLER_2D 0x8B5E
#endif
#ifndef GL_SAMPLER_3D
#define GL_SAMPLER_3D 0x8B5F
#endif
#ifndef GL_SAMPLER_CUBE
#define GL_SAMPLER_CUBE 0x8B60
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_INFO_LOG_LENGTH
#define GL_INFO_LOG_LENGTH 0x8B84
#endif
#ifndef GL_ACTIVE_UNIFORMS
#define GL_ACTIVE_UNIFORMS 0x8B86
#endif
#ifndef GL_ACTIVE_UNIFORM_MAX_LENGTH
#define GL_ACTIVE_UNIFORM_MAX_LENGTH 0x8B87
#endif
#ifndef GL_ACTIVE_ATTRIBUTES
#define GL_ACTIVE_ATTRIBUTES 0x8B89
#endif
#ifndef GL_ACTIVE_ATTRIBUTE_MAX_LENGTH
#define GL_ACTIVE_ATTRIBUTE_MAX_LENGTH 0x8B8A
#endif
#ifndef GL_GEOMETRY_SHADER
#define GL_GEOMETRY_SHADER 0x8DD9
#endif
#ifndef GL_GEOMETRY_VERTICES_OUT
#define GL_GEOMETRY_VERTICES_OUT 0x8DDA
#endif
#ifndef GL_GEOMETRY_INPUT_TYPE
#define GL_GEOMETRY_INPUT_TYPE 0x8DDB
#endif
#ifndef GL_GEOMETRY_OUTPUT_TYPE
#define GL_GEOMETRY_OUTPUT_TYPE 0x8DDC
#endif
#ifndef GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS
#define GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS 0x8C29
#endif
#ifndef GL_MAX_GEOMETRY_VARYING_COMPONENTS
#define GL_MAX_GEOMETRY_VARYING_COMPONENTS 0x8DDD
#endif
#ifndef GL_MAX_VERTEX_VARYING_COMPONENTS
#define GL_MAX_VERTEX_VARYING_COMPONENTS 0x8DDE
#endif
#ifndef GL_MAX_VARYING_COMPONENTS
#define GL_MAX_VARYING_COMPONENTS 0x8B4B
#endif
#ifndef GL_MAX_GEOMETRY_UNIFORM_COMPONENTS
#define GL_MAX_GEOMETRY_UNIFORM_COMPONENTS 0x8DDF
#endif
#ifndef GL_MAX_GEOMETRY_OUTPUT_VERTICES
#define GL_MAX_GEOMETRY_OUTPUT_VERTICES 0x8DE0
#endif
#ifndef GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS
#define GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS 0x8DE1
#endif
#ifndef GL_LINES_ADJACENCY
#define GL_LINES_ADJACENCY 0xA
#endif
#ifndef GL_LINE_STRIP_ADJACENCY
#define GL_LINE_STRIP_ADJACENCY 0xB
#endif
#ifndef GL_TRIANGLES_ADJACENCY
#define GL_TRIANGLES_ADJACENCY 0xC
#endif
#ifndef GL_TRIANGLE_STRIP_ADJACENCY
#define GL_TRIANGLE_STRIP_ADJACENCY 0xD
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS
#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS 0x8DA8
#endif
#ifndef GL_FRAMEBUFFER_INCOMPLETE_LAYER_COUNT
#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_COUNT 0x8DA9
#endif
#ifndef GL_FRAMEBUFFER_ATTACHMENT_LAYERED
#define GL_FRAMEBUFFER_ATTACHMENT_LAYERED 0x8DA7
#endif
#ifndef GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER 0x8CD4
#endif
#ifndef GL_PROGRAM_POINT_SIZE
#define GL_PROGRAM_POINT_SIZE 0x8642
#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif // QOPENGL_EXTENSIONS_P_H
