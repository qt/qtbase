/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QOPENGLEXTRAFUNCTIONS_H
#define QOPENGLEXTRAFUNCTIONS_H

#include <QtCore/qglobal.h>

#ifndef QT_NO_OPENGL

#include <QtGui/qopenglfunctions.h>

QT_BEGIN_NAMESPACE

class QOpenGLExtraFunctionsPrivate;

#undef glReadBuffer
#undef glDrawRangeElements
#undef glTexImage3D
#undef glTexSubImage3D
#undef glCopyTexSubImage3D
#undef glCompressedTexImage3D
#undef glCompressedTexSubImage3D
#undef glGenQueries
#undef glDeleteQueries
#undef glIsQuery
#undef glBeginQuery
#undef glEndQuery
#undef glGetQueryiv
#undef glGetQueryObjectuiv
#undef glUnmapBuffer
#undef glGetBufferPointerv
#undef glDrawBuffers
#undef glUniformMatrix2x3fv
#undef glUniformMatrix3x2fv
#undef glUniformMatrix2x4fv
#undef glUniformMatrix4x2fv
#undef glUniformMatrix3x4fv
#undef glUniformMatrix4x3fv
#undef glBlitFramebuffer
#undef glRenderbufferStorageMultisample
#undef glFramebufferTextureLayer
#undef glMapBufferRange
#undef glFlushMappedBufferRange
#undef glBindVertexArray
#undef glDeleteVertexArrays
#undef glGenVertexArrays
#undef glIsVertexArray
#undef glGetIntegeri_v
#undef glBeginTransformFeedback
#undef glEndTransformFeedback
#undef glBindBufferRange
#undef glBindBufferBase
#undef glTransformFeedbackVaryings
#undef glGetTransformFeedbackVarying
#undef glVertexAttribIPointer
#undef glGetVertexAttribIiv
#undef glGetVertexAttribIuiv
#undef glVertexAttribI4i
#undef glVertexAttribI4ui
#undef glVertexAttribI4iv
#undef glVertexAttribI4uiv
#undef glGetUniformuiv
#undef glGetFragDataLocation
#undef glUniform1ui
#undef glUniform2ui
#undef glUniform3ui
#undef glUniform4ui
#undef glUniform1uiv
#undef glUniform2uiv
#undef glUniform3uiv
#undef glUniform4uiv
#undef glClearBufferiv
#undef glClearBufferuiv
#undef glClearBufferfv
#undef glClearBufferfi
#undef glGetStringi
#undef glCopyBufferSubData
#undef glGetUniformIndices
#undef glGetActiveUniformsiv
#undef glGetUniformBlockIndex
#undef glGetActiveUniformBlockiv
#undef glGetActiveUniformBlockName
#undef glUniformBlockBinding
#undef glDrawArraysInstanced
#undef glDrawElementsInstanced
#undef glFenceSync
#undef glIsSync
#undef glDeleteSync
#undef glClientWaitSync
#undef glWaitSync
#undef glGetInteger64v
#undef glGetSynciv
#undef glGetInteger64i_v
#undef glGetBufferParameteri64v
#undef glGenSamplers
#undef glDeleteSamplers
#undef glIsSampler
#undef glBindSampler
#undef glSamplerParameteri
#undef glSamplerParameteriv
#undef glSamplerParameterf
#undef glSamplerParameterfv
#undef glGetSamplerParameteriv
#undef glGetSamplerParameterfv
#undef glVertexAttribDivisor
#undef glBindTransformFeedback
#undef glDeleteTransformFeedbacks
#undef glGenTransformFeedbacks
#undef glIsTransformFeedback
#undef glPauseTransformFeedback
#undef glResumeTransformFeedback
#undef glGetProgramBinary
#undef glProgramBinary
#undef glProgramParameteri
#undef glInvalidateFramebuffer
#undef glInvalidateSubFramebuffer
#undef glTexStorage2D
#undef glTexStorage3D
#undef glGetInternalformativ

#undef glDispatchCompute
#undef glDispatchComputeIndirect
#undef glDrawArraysIndirect
#undef glDrawElementsIndirect
#undef glFramebufferParameteri
#undef glGetFramebufferParameteriv
#undef glGetProgramInterfaceiv
#undef glGetProgramResourceIndex
#undef glGetProgramResourceName
#undef glGetProgramResourceiv
#undef glGetProgramResourceLocation
#undef glUseProgramStages
#undef glActiveShaderProgram
#undef glCreateShaderProgramv
#undef glBindProgramPipeline
#undef glDeleteProgramPipelines
#undef glGenProgramPipelines
#undef glIsProgramPipeline
#undef glGetProgramPipelineiv
#undef glProgramUniform1i
#undef glProgramUniform2i
#undef glProgramUniform3i
#undef glProgramUniform4i
#undef glProgramUniform1ui
#undef glProgramUniform2ui
#undef glProgramUniform3ui
#undef glProgramUniform4ui
#undef glProgramUniform1f
#undef glProgramUniform2f
#undef glProgramUniform3f
#undef glProgramUniform4f
#undef glProgramUniform1iv
#undef glProgramUniform2iv
#undef glProgramUniform3iv
#undef glProgramUniform4iv
#undef glProgramUniform1uiv
#undef glProgramUniform2uiv
#undef glProgramUniform3uiv
#undef glProgramUniform4uiv
#undef glProgramUniform1fv
#undef glProgramUniform2fv
#undef glProgramUniform3fv
#undef glProgramUniform4fv
#undef glProgramUniformMatrix2fv
#undef glProgramUniformMatrix3fv
#undef glProgramUniformMatrix4fv
#undef glProgramUniformMatrix2x3fv
#undef glProgramUniformMatrix3x2fv
#undef glProgramUniformMatrix2x4fv
#undef glProgramUniformMatrix4x2fv
#undef glProgramUniformMatrix3x4fv
#undef glProgramUniformMatrix4x3fv
#undef glValidateProgramPipeline
#undef glGetProgramPipelineInfoLog
#undef glBindImageTexture
#undef glGetBooleani_v
#undef glMemoryBarrier
#undef glMemoryBarrierByRegion
#undef glTexStorage2DMultisample
#undef glGetMultisamplefv
#undef glSampleMaski
#undef glGetTexLevelParameteriv
#undef glGetTexLevelParameterfv
#undef glBindVertexBuffer
#undef glVertexAttribFormat
#undef glVertexAttribIFormat
#undef glVertexAttribBinding
#undef glVertexBindingDivisor

class Q_GUI_EXPORT QOpenGLExtraFunctions : public QOpenGLFunctions
{
    Q_DECLARE_PRIVATE(QOpenGLExtraFunctions)

public:
    QOpenGLExtraFunctions();
    QOpenGLExtraFunctions(QOpenGLContext *context);
    ~QOpenGLExtraFunctions() {}

    // GLES3
    void glReadBuffer(GLenum mode);
    void glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices);
    void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels);
    void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
    void glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
    void glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data);
    void glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data);
    void glGenQueries(GLsizei n, GLuint *ids);
    void glDeleteQueries(GLsizei n, const GLuint *ids);
    GLboolean glIsQuery(GLuint id);
    void glBeginQuery(GLenum target, GLuint id);
    void glEndQuery(GLenum target);
    void glGetQueryiv(GLenum target, GLenum pname, GLint *params);
    void glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint *params);
    GLboolean glUnmapBuffer(GLenum target);
    void glGetBufferPointerv(GLenum target, GLenum pname, void **params);
    void glDrawBuffers(GLsizei n, const GLenum *bufs);
    void glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
    void glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
    void glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
    void *glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
    void glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length);
    void glBindVertexArray(GLuint array);
    void glDeleteVertexArrays(GLsizei n, const GLuint *arrays);
    void glGenVertexArrays(GLsizei n, GLuint *arrays);
    GLboolean glIsVertexArray(GLuint array);
    void glGetIntegeri_v(GLenum target, GLuint index, GLint *data);
    void glBeginTransformFeedback(GLenum primitiveMode);
    void glEndTransformFeedback(void);
    void glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
    void glBindBufferBase(GLenum target, GLuint index, GLuint buffer);
    void glTransformFeedbackVaryings(GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode);
    void glGetTransformFeedbackVarying(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name);
    void glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);
    void glGetVertexAttribIiv(GLuint index, GLenum pname, GLint *params);
    void glGetVertexAttribIuiv(GLuint index, GLenum pname, GLuint *params);
    void glVertexAttribI4i(GLuint index, GLint x, GLint y, GLint z, GLint w);
    void glVertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w);
    void glVertexAttribI4iv(GLuint index, const GLint *v);
    void glVertexAttribI4uiv(GLuint index, const GLuint *v);
    void glGetUniformuiv(GLuint program, GLint location, GLuint *params);
    GLint glGetFragDataLocation(GLuint program, const GLchar *name);
    void glUniform1ui(GLint location, GLuint v0);
    void glUniform2ui(GLint location, GLuint v0, GLuint v1);
    void glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2);
    void glUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
    void glUniform1uiv(GLint location, GLsizei count, const GLuint *value);
    void glUniform2uiv(GLint location, GLsizei count, const GLuint *value);
    void glUniform3uiv(GLint location, GLsizei count, const GLuint *value);
    void glUniform4uiv(GLint location, GLsizei count, const GLuint *value);
    void glClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint *value);
    void glClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint *value);
    void glClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat *value);
    void glClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil);
    const GLubyte *glGetStringi(GLenum name, GLuint index);
    void glCopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);
    void glGetUniformIndices(GLuint program, GLsizei uniformCount, const GLchar *const*uniformNames, GLuint *uniformIndices);
    void glGetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params);
    GLuint glGetUniformBlockIndex(GLuint program, const GLchar *uniformBlockName);
    void glGetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params);
    void glGetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName);
    void glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
    void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
    void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount);
    GLsync glFenceSync(GLenum condition, GLbitfield flags);
    GLboolean glIsSync(GLsync sync);
    void glDeleteSync(GLsync sync);
    GLenum glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout);
    void glWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout);
    void glGetInteger64v(GLenum pname, GLint64 *data);
    void glGetSynciv(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values);
    void glGetInteger64i_v(GLenum target, GLuint index, GLint64 *data);
    void glGetBufferParameteri64v(GLenum target, GLenum pname, GLint64 *params);
    void glGenSamplers(GLsizei count, GLuint *samplers);
    void glDeleteSamplers(GLsizei count, const GLuint *samplers);
    GLboolean glIsSampler(GLuint sampler);
    void glBindSampler(GLuint unit, GLuint sampler);
    void glSamplerParameteri(GLuint sampler, GLenum pname, GLint param);
    void glSamplerParameteriv(GLuint sampler, GLenum pname, const GLint *param);
    void glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param);
    void glSamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat *param);
    void glGetSamplerParameteriv(GLuint sampler, GLenum pname, GLint *params);
    void glGetSamplerParameterfv(GLuint sampler, GLenum pname, GLfloat *params);
    void glVertexAttribDivisor(GLuint index, GLuint divisor);
    void glBindTransformFeedback(GLenum target, GLuint id);
    void glDeleteTransformFeedbacks(GLsizei n, const GLuint *ids);
    void glGenTransformFeedbacks(GLsizei n, GLuint *ids);
    GLboolean glIsTransformFeedback(GLuint id);
    void glPauseTransformFeedback(void);
    void glResumeTransformFeedback(void);
    void glGetProgramBinary(GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary);
    void glProgramBinary(GLuint program, GLenum binaryFormat, const void *binary, GLsizei length);
    void glProgramParameteri(GLuint program, GLenum pname, GLint value);
    void glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum *attachments);
    void glInvalidateSubFramebuffer(GLenum target, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height);
    void glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
    void glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
    void glGetInternalformativ(GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint *params);

    // GLES 3.1
    void glDispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);
    void glDispatchComputeIndirect(GLintptr indirect);
    void glDrawArraysIndirect(GLenum mode, const void *indirect);
    void glDrawElementsIndirect(GLenum mode, GLenum type, const void *indirect);
    void glFramebufferParameteri(GLenum target, GLenum pname, GLint param);
    void glGetFramebufferParameteriv(GLenum target, GLenum pname, GLint *params);
    void glGetProgramInterfaceiv(GLuint program, GLenum programInterface, GLenum pname, GLint *params);
    GLuint glGetProgramResourceIndex(GLuint program, GLenum programInterface, const GLchar *name);
    void glGetProgramResourceName(GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name);
    void glGetProgramResourceiv(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei bufSize, GLsizei *length, GLint *params);
    GLint glGetProgramResourceLocation(GLuint program, GLenum programInterface, const GLchar *name);
    void glUseProgramStages(GLuint pipeline, GLbitfield stages, GLuint program);
    void glActiveShaderProgram(GLuint pipeline, GLuint program);
    GLuint glCreateShaderProgramv(GLenum type, GLsizei count, const GLchar *const*strings);
    void glBindProgramPipeline(GLuint pipeline);
    void glDeleteProgramPipelines(GLsizei n, const GLuint *pipelines);
    void glGenProgramPipelines(GLsizei n, GLuint *pipelines);
    GLboolean glIsProgramPipeline(GLuint pipeline);
    void glGetProgramPipelineiv(GLuint pipeline, GLenum pname, GLint *params);
    void glProgramUniform1i(GLuint program, GLint location, GLint v0);
    void glProgramUniform2i(GLuint program, GLint location, GLint v0, GLint v1);
    void glProgramUniform3i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2);
    void glProgramUniform4i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
    void glProgramUniform1ui(GLuint program, GLint location, GLuint v0);
    void glProgramUniform2ui(GLuint program, GLint location, GLuint v0, GLuint v1);
    void glProgramUniform3ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2);
    void glProgramUniform4ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
    void glProgramUniform1f(GLuint program, GLint location, GLfloat v0);
    void glProgramUniform2f(GLuint program, GLint location, GLfloat v0, GLfloat v1);
    void glProgramUniform3f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
    void glProgramUniform4f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
    void glProgramUniform1iv(GLuint program, GLint location, GLsizei count, const GLint *value);
    void glProgramUniform2iv(GLuint program, GLint location, GLsizei count, const GLint *value);
    void glProgramUniform3iv(GLuint program, GLint location, GLsizei count, const GLint *value);
    void glProgramUniform4iv(GLuint program, GLint location, GLsizei count, const GLint *value);
    void glProgramUniform1uiv(GLuint program, GLint location, GLsizei count, const GLuint *value);
    void glProgramUniform2uiv(GLuint program, GLint location, GLsizei count, const GLuint *value);
    void glProgramUniform3uiv(GLuint program, GLint location, GLsizei count, const GLuint *value);
    void glProgramUniform4uiv(GLuint program, GLint location, GLsizei count, const GLuint *value);
    void glProgramUniform1fv(GLuint program, GLint location, GLsizei count, const GLfloat *value);
    void glProgramUniform2fv(GLuint program, GLint location, GLsizei count, const GLfloat *value);
    void glProgramUniform3fv(GLuint program, GLint location, GLsizei count, const GLfloat *value);
    void glProgramUniform4fv(GLuint program, GLint location, GLsizei count, const GLfloat *value);
    void glProgramUniformMatrix2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void glProgramUniformMatrix3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void glProgramUniformMatrix4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void glProgramUniformMatrix2x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void glProgramUniformMatrix3x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void glProgramUniformMatrix2x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void glProgramUniformMatrix4x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void glProgramUniformMatrix3x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void glProgramUniformMatrix4x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void glValidateProgramPipeline(GLuint pipeline);
    void glGetProgramPipelineInfoLog(GLuint pipeline, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
    void glBindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format);
    void glGetBooleani_v(GLenum target, GLuint index, GLboolean *data);
    void glMemoryBarrier(GLbitfield barriers);
    void glMemoryBarrierByRegion(GLbitfield barriers);
    void glTexStorage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
    void glGetMultisamplefv(GLenum pname, GLuint index, GLfloat *val);
    void glSampleMaski(GLuint maskNumber, GLbitfield mask);
    void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params);
    void glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params);
    void glBindVertexBuffer(GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride);
    void glVertexAttribFormat(GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset);
    void glVertexAttribIFormat(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
    void glVertexAttribBinding(GLuint attribindex, GLuint bindingindex);
    void glVertexBindingDivisor(GLuint bindingindex, GLuint divisor);

private:
    static bool isInitialized(const QOpenGLExtraFunctionsPrivate *d) { return d != Q_NULLPTR; }
};

class QOpenGLExtraFunctionsPrivate : public QOpenGLFunctionsPrivate
{
public:
    QOpenGLExtraFunctionsPrivate(QOpenGLContext *ctx);

    // GLES3
    void (QOPENGLF_APIENTRYP ReadBuffer)(GLenum mode);
    void (QOPENGLF_APIENTRYP DrawRangeElements)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices);
    void (QOPENGLF_APIENTRYP TexImage3D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels);
    void (QOPENGLF_APIENTRYP TexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
    void (QOPENGLF_APIENTRYP CopyTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
    void (QOPENGLF_APIENTRYP CompressedTexImage3D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data);
    void (QOPENGLF_APIENTRYP CompressedTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data);
    void (QOPENGLF_APIENTRYP GenQueries)(GLsizei n, GLuint *ids);
    void (QOPENGLF_APIENTRYP DeleteQueries)(GLsizei n, const GLuint *ids);
    GLboolean (QOPENGLF_APIENTRYP IsQuery)(GLuint id);
    void (QOPENGLF_APIENTRYP BeginQuery)(GLenum target, GLuint id);
    void (QOPENGLF_APIENTRYP EndQuery)(GLenum target);
    void (QOPENGLF_APIENTRYP GetQueryiv)(GLenum target, GLenum pname, GLint *params);
    void (QOPENGLF_APIENTRYP GetQueryObjectuiv)(GLuint id, GLenum pname, GLuint *params);
    GLboolean (QOPENGLF_APIENTRYP UnmapBuffer)(GLenum target);
    void (QOPENGLF_APIENTRYP GetBufferPointerv)(GLenum target, GLenum pname, void **params);
    void (QOPENGLF_APIENTRYP DrawBuffers)(GLsizei n, const GLenum *bufs);
    void (QOPENGLF_APIENTRYP UniformMatrix2x3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void (QOPENGLF_APIENTRYP UniformMatrix3x2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void (QOPENGLF_APIENTRYP UniformMatrix2x4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void (QOPENGLF_APIENTRYP UniformMatrix4x2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void (QOPENGLF_APIENTRYP UniformMatrix3x4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void (QOPENGLF_APIENTRYP UniformMatrix4x3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void (QOPENGLF_APIENTRYP BlitFramebuffer)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
    void (QOPENGLF_APIENTRYP RenderbufferStorageMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
    void (QOPENGLF_APIENTRYP FramebufferTextureLayer)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
    void *(QOPENGLF_APIENTRYP MapBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
    void (QOPENGLF_APIENTRYP FlushMappedBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length);
    void (QOPENGLF_APIENTRYP BindVertexArray)(GLuint array);
    void (QOPENGLF_APIENTRYP DeleteVertexArrays)(GLsizei n, const GLuint *arrays);
    void (QOPENGLF_APIENTRYP GenVertexArrays)(GLsizei n, GLuint *arrays);
    GLboolean (QOPENGLF_APIENTRYP IsVertexArray)(GLuint array);
    void (QOPENGLF_APIENTRYP GetIntegeri_v)(GLenum target, GLuint index, GLint *data);
    void (QOPENGLF_APIENTRYP BeginTransformFeedback)(GLenum primitiveMode);
    void (QOPENGLF_APIENTRYP EndTransformFeedback)(void);
    void (QOPENGLF_APIENTRYP BindBufferRange)(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
    void (QOPENGLF_APIENTRYP BindBufferBase)(GLenum target, GLuint index, GLuint buffer);
    void (QOPENGLF_APIENTRYP TransformFeedbackVaryings)(GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode);
    void (QOPENGLF_APIENTRYP GetTransformFeedbackVarying)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name);
    void (QOPENGLF_APIENTRYP VertexAttribIPointer)(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);
    void (QOPENGLF_APIENTRYP GetVertexAttribIiv)(GLuint index, GLenum pname, GLint *params);
    void (QOPENGLF_APIENTRYP GetVertexAttribIuiv)(GLuint index, GLenum pname, GLuint *params);
    void (QOPENGLF_APIENTRYP VertexAttribI4i)(GLuint index, GLint x, GLint y, GLint z, GLint w);
    void (QOPENGLF_APIENTRYP VertexAttribI4ui)(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w);
    void (QOPENGLF_APIENTRYP VertexAttribI4iv)(GLuint index, const GLint *v);
    void (QOPENGLF_APIENTRYP VertexAttribI4uiv)(GLuint index, const GLuint *v);
    void (QOPENGLF_APIENTRYP GetUniformuiv)(GLuint program, GLint location, GLuint *params);
    GLint (QOPENGLF_APIENTRYP GetFragDataLocation)(GLuint program, const GLchar *name);
    void (QOPENGLF_APIENTRYP Uniform1ui)(GLint location, GLuint v0);
    void (QOPENGLF_APIENTRYP Uniform2ui)(GLint location, GLuint v0, GLuint v1);
    void (QOPENGLF_APIENTRYP Uniform3ui)(GLint location, GLuint v0, GLuint v1, GLuint v2);
    void (QOPENGLF_APIENTRYP Uniform4ui)(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
    void (QOPENGLF_APIENTRYP Uniform1uiv)(GLint location, GLsizei count, const GLuint *value);
    void (QOPENGLF_APIENTRYP Uniform2uiv)(GLint location, GLsizei count, const GLuint *value);
    void (QOPENGLF_APIENTRYP Uniform3uiv)(GLint location, GLsizei count, const GLuint *value);
    void (QOPENGLF_APIENTRYP Uniform4uiv)(GLint location, GLsizei count, const GLuint *value);
    void (QOPENGLF_APIENTRYP ClearBufferiv)(GLenum buffer, GLint drawbuffer, const GLint *value);
    void (QOPENGLF_APIENTRYP ClearBufferuiv)(GLenum buffer, GLint drawbuffer, const GLuint *value);
    void (QOPENGLF_APIENTRYP ClearBufferfv)(GLenum buffer, GLint drawbuffer, const GLfloat *value);
    void (QOPENGLF_APIENTRYP ClearBufferfi)(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil);
    const GLubyte *(QOPENGLF_APIENTRYP GetStringi)(GLenum name, GLuint index);
    void (QOPENGLF_APIENTRYP CopyBufferSubData)(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);
    void (QOPENGLF_APIENTRYP GetUniformIndices)(GLuint program, GLsizei uniformCount, const GLchar *const*uniformNames, GLuint *uniformIndices);
    void (QOPENGLF_APIENTRYP GetActiveUniformsiv)(GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params);
    GLuint (QOPENGLF_APIENTRYP GetUniformBlockIndex)(GLuint program, const GLchar *uniformBlockName);
    void (QOPENGLF_APIENTRYP GetActiveUniformBlockiv)(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params);
    void (QOPENGLF_APIENTRYP GetActiveUniformBlockName)(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName);
    void (QOPENGLF_APIENTRYP UniformBlockBinding)(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
    void (QOPENGLF_APIENTRYP DrawArraysInstanced)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
    void (QOPENGLF_APIENTRYP DrawElementsInstanced)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount);
    GLsync (QOPENGLF_APIENTRYP FenceSync)(GLenum condition, GLbitfield flags);
    GLboolean (QOPENGLF_APIENTRYP IsSync)(GLsync sync);
    void (QOPENGLF_APIENTRYP DeleteSync)(GLsync sync);
    GLenum (QOPENGLF_APIENTRYP ClientWaitSync)(GLsync sync, GLbitfield flags, GLuint64 timeout);
    void (QOPENGLF_APIENTRYP WaitSync)(GLsync sync, GLbitfield flags, GLuint64 timeout);
    void (QOPENGLF_APIENTRYP GetInteger64v)(GLenum pname, GLint64 *data);
    void (QOPENGLF_APIENTRYP GetSynciv)(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values);
    void (QOPENGLF_APIENTRYP GetInteger64i_v)(GLenum target, GLuint index, GLint64 *data);
    void (QOPENGLF_APIENTRYP GetBufferParameteri64v)(GLenum target, GLenum pname, GLint64 *params);
    void (QOPENGLF_APIENTRYP GenSamplers)(GLsizei count, GLuint *samplers);
    void (QOPENGLF_APIENTRYP DeleteSamplers)(GLsizei count, const GLuint *samplers);
    GLboolean (QOPENGLF_APIENTRYP IsSampler)(GLuint sampler);
    void (QOPENGLF_APIENTRYP BindSampler)(GLuint unit, GLuint sampler);
    void (QOPENGLF_APIENTRYP SamplerParameteri)(GLuint sampler, GLenum pname, GLint param);
    void (QOPENGLF_APIENTRYP SamplerParameteriv)(GLuint sampler, GLenum pname, const GLint *param);
    void (QOPENGLF_APIENTRYP SamplerParameterf)(GLuint sampler, GLenum pname, GLfloat param);
    void (QOPENGLF_APIENTRYP SamplerParameterfv)(GLuint sampler, GLenum pname, const GLfloat *param);
    void (QOPENGLF_APIENTRYP GetSamplerParameteriv)(GLuint sampler, GLenum pname, GLint *params);
    void (QOPENGLF_APIENTRYP GetSamplerParameterfv)(GLuint sampler, GLenum pname, GLfloat *params);
    void (QOPENGLF_APIENTRYP VertexAttribDivisor)(GLuint index, GLuint divisor);
    void (QOPENGLF_APIENTRYP BindTransformFeedback)(GLenum target, GLuint id);
    void (QOPENGLF_APIENTRYP DeleteTransformFeedbacks)(GLsizei n, const GLuint *ids);
    void (QOPENGLF_APIENTRYP GenTransformFeedbacks)(GLsizei n, GLuint *ids);
    GLboolean (QOPENGLF_APIENTRYP IsTransformFeedback)(GLuint id);
    void (QOPENGLF_APIENTRYP PauseTransformFeedback)(void);
    void (QOPENGLF_APIENTRYP ResumeTransformFeedback)(void);
    void (QOPENGLF_APIENTRYP GetProgramBinary)(GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary);
    void (QOPENGLF_APIENTRYP ProgramBinary)(GLuint program, GLenum binaryFormat, const void *binary, GLsizei length);
    void (QOPENGLF_APIENTRYP ProgramParameteri)(GLuint program, GLenum pname, GLint value);
    void (QOPENGLF_APIENTRYP InvalidateFramebuffer)(GLenum target, GLsizei numAttachments, const GLenum *attachments);
    void (QOPENGLF_APIENTRYP InvalidateSubFramebuffer)(GLenum target, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height);
    void (QOPENGLF_APIENTRYP TexStorage2D)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
    void (QOPENGLF_APIENTRYP TexStorage3D)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
    void (QOPENGLF_APIENTRYP GetInternalformativ)(GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint *params);

    // GLES 3.1
    void (QOPENGLF_APIENTRYP DispatchCompute)(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);
    void (QOPENGLF_APIENTRYP DispatchComputeIndirect)(GLintptr indirect);
    void (QOPENGLF_APIENTRYP DrawArraysIndirect)(GLenum mode, const void *indirect);
    void (QOPENGLF_APIENTRYP DrawElementsIndirect)(GLenum mode, GLenum type, const void *indirect);
    void (QOPENGLF_APIENTRYP FramebufferParameteri)(GLenum target, GLenum pname, GLint param);
    void (QOPENGLF_APIENTRYP GetFramebufferParameteriv)(GLenum target, GLenum pname, GLint *params);
    void (QOPENGLF_APIENTRYP GetProgramInterfaceiv)(GLuint program, GLenum programInterface, GLenum pname, GLint *params);
    GLuint (QOPENGLF_APIENTRYP GetProgramResourceIndex)(GLuint program, GLenum programInterface, const GLchar *name);
    void (QOPENGLF_APIENTRYP GetProgramResourceName)(GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name);
    void (QOPENGLF_APIENTRYP GetProgramResourceiv)(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei bufSize, GLsizei *length, GLint *params);
    GLint (QOPENGLF_APIENTRYP GetProgramResourceLocation)(GLuint program, GLenum programInterface, const GLchar *name);
    void (QOPENGLF_APIENTRYP UseProgramStages)(GLuint pipeline, GLbitfield stages, GLuint program);
    void (QOPENGLF_APIENTRYP ActiveShaderProgram)(GLuint pipeline, GLuint program);
    GLuint (QOPENGLF_APIENTRYP CreateShaderProgramv)(GLenum type, GLsizei count, const GLchar *const*strings);
    void (QOPENGLF_APIENTRYP BindProgramPipeline)(GLuint pipeline);
    void (QOPENGLF_APIENTRYP DeleteProgramPipelines)(GLsizei n, const GLuint *pipelines);
    void (QOPENGLF_APIENTRYP GenProgramPipelines)(GLsizei n, GLuint *pipelines);
    GLboolean (QOPENGLF_APIENTRYP IsProgramPipeline)(GLuint pipeline);
    void (QOPENGLF_APIENTRYP GetProgramPipelineiv)(GLuint pipeline, GLenum pname, GLint *params);
    void (QOPENGLF_APIENTRYP ProgramUniform1i)(GLuint program, GLint location, GLint v0);
    void (QOPENGLF_APIENTRYP ProgramUniform2i)(GLuint program, GLint location, GLint v0, GLint v1);
    void (QOPENGLF_APIENTRYP ProgramUniform3i)(GLuint program, GLint location, GLint v0, GLint v1, GLint v2);
    void (QOPENGLF_APIENTRYP ProgramUniform4i)(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
    void (QOPENGLF_APIENTRYP ProgramUniform1ui)(GLuint program, GLint location, GLuint v0);
    void (QOPENGLF_APIENTRYP ProgramUniform2ui)(GLuint program, GLint location, GLuint v0, GLuint v1);
    void (QOPENGLF_APIENTRYP ProgramUniform3ui)(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2);
    void (QOPENGLF_APIENTRYP ProgramUniform4ui)(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
    void (QOPENGLF_APIENTRYP ProgramUniform1f)(GLuint program, GLint location, GLfloat v0);
    void (QOPENGLF_APIENTRYP ProgramUniform2f)(GLuint program, GLint location, GLfloat v0, GLfloat v1);
    void (QOPENGLF_APIENTRYP ProgramUniform3f)(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
    void (QOPENGLF_APIENTRYP ProgramUniform4f)(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
    void (QOPENGLF_APIENTRYP ProgramUniform1iv)(GLuint program, GLint location, GLsizei count, const GLint *value);
    void (QOPENGLF_APIENTRYP ProgramUniform2iv)(GLuint program, GLint location, GLsizei count, const GLint *value);
    void (QOPENGLF_APIENTRYP ProgramUniform3iv)(GLuint program, GLint location, GLsizei count, const GLint *value);
    void (QOPENGLF_APIENTRYP ProgramUniform4iv)(GLuint program, GLint location, GLsizei count, const GLint *value);
    void (QOPENGLF_APIENTRYP ProgramUniform1uiv)(GLuint program, GLint location, GLsizei count, const GLuint *value);
    void (QOPENGLF_APIENTRYP ProgramUniform2uiv)(GLuint program, GLint location, GLsizei count, const GLuint *value);
    void (QOPENGLF_APIENTRYP ProgramUniform3uiv)(GLuint program, GLint location, GLsizei count, const GLuint *value);
    void (QOPENGLF_APIENTRYP ProgramUniform4uiv)(GLuint program, GLint location, GLsizei count, const GLuint *value);
    void (QOPENGLF_APIENTRYP ProgramUniform1fv)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
    void (QOPENGLF_APIENTRYP ProgramUniform2fv)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
    void (QOPENGLF_APIENTRYP ProgramUniform3fv)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
    void (QOPENGLF_APIENTRYP ProgramUniform4fv)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
    void (QOPENGLF_APIENTRYP ProgramUniformMatrix2fv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void (QOPENGLF_APIENTRYP ProgramUniformMatrix3fv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void (QOPENGLF_APIENTRYP ProgramUniformMatrix4fv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void (QOPENGLF_APIENTRYP ProgramUniformMatrix2x3fv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void (QOPENGLF_APIENTRYP ProgramUniformMatrix3x2fv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void (QOPENGLF_APIENTRYP ProgramUniformMatrix2x4fv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void (QOPENGLF_APIENTRYP ProgramUniformMatrix4x2fv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void (QOPENGLF_APIENTRYP ProgramUniformMatrix3x4fv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void (QOPENGLF_APIENTRYP ProgramUniformMatrix4x3fv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void (QOPENGLF_APIENTRYP ValidateProgramPipeline)(GLuint pipeline);
    void (QOPENGLF_APIENTRYP GetProgramPipelineInfoLog)(GLuint pipeline, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
    void (QOPENGLF_APIENTRYP BindImageTexture)(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format);
    void (QOPENGLF_APIENTRYP GetBooleani_v)(GLenum target, GLuint index, GLboolean *data);
    void (QOPENGLF_APIENTRYP MemoryBarrierFunc)(GLbitfield barriers);
    void (QOPENGLF_APIENTRYP MemoryBarrierByRegion)(GLbitfield barriers);
    void (QOPENGLF_APIENTRYP TexStorage2DMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
    void (QOPENGLF_APIENTRYP GetMultisamplefv)(GLenum pname, GLuint index, GLfloat *val);
    void (QOPENGLF_APIENTRYP SampleMaski)(GLuint maskNumber, GLbitfield mask);
    void (QOPENGLF_APIENTRYP GetTexLevelParameteriv)(GLenum target, GLint level, GLenum pname, GLint *params);
    void (QOPENGLF_APIENTRYP GetTexLevelParameterfv)(GLenum target, GLint level, GLenum pname, GLfloat *params);
    void (QOPENGLF_APIENTRYP BindVertexBuffer)(GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride);
    void (QOPENGLF_APIENTRYP VertexAttribFormat)(GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset);
    void (QOPENGLF_APIENTRYP VertexAttribIFormat)(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
    void (QOPENGLF_APIENTRYP VertexAttribBinding)(GLuint attribindex, GLuint bindingindex);
    void (QOPENGLF_APIENTRYP VertexBindingDivisor)(GLuint bindingindex, GLuint divisor);
};

// GLES 3.0 and 3.1

inline void QOpenGLExtraFunctions::glBeginQuery(GLenum target, GLuint id)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->BeginQuery(target, id);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glBeginTransformFeedback(GLenum primitiveMode)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->BeginTransformFeedback(primitiveMode);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glBindBufferBase(GLenum target, GLuint index, GLuint buffer)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->BindBufferBase(target, index, buffer);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->BindBufferRange(target, index, buffer, offset, size);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glBindSampler(GLuint unit, GLuint sampler)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->BindSampler(unit, sampler);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glBindTransformFeedback(GLenum target, GLuint id)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->BindTransformFeedback(target, id);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glBindVertexArray(GLuint array)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->BindVertexArray(array);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->BlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ClearBufferfi(buffer, drawbuffer, depth, stencil);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ClearBufferfv(buffer, drawbuffer, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ClearBufferiv(buffer, drawbuffer, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ClearBufferuiv(buffer, drawbuffer, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLenum QOpenGLExtraFunctions::glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    GLenum result = d->ClientWaitSync(sync, flags, timeout);
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLExtraFunctions::glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void * data)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->CompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, data);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void * data)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->CompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glCopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->CopyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->CopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glDeleteQueries(GLsizei n, const GLuint * ids)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->DeleteQueries(n, ids);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glDeleteSamplers(GLsizei count, const GLuint * samplers)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->DeleteSamplers(count, samplers);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glDeleteSync(GLsync sync)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->DeleteSync(sync);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glDeleteTransformFeedbacks(GLsizei n, const GLuint * ids)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->DeleteTransformFeedbacks(n, ids);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glDeleteVertexArrays(GLsizei n, const GLuint * arrays)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->DeleteVertexArrays(n, arrays);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->DrawArraysInstanced(mode, first, count, instancecount);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glDrawBuffers(GLsizei n, const GLenum * bufs)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->DrawBuffers(n, bufs);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void * indices, GLsizei instancecount)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->DrawElementsInstanced(mode, count, type, indices, instancecount);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void * indices)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->DrawRangeElements(mode, start, end, count, type, indices);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glEndQuery(GLenum target)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->EndQuery(target);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glEndTransformFeedback()
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->EndTransformFeedback();
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLsync QOpenGLExtraFunctions::glFenceSync(GLenum condition, GLbitfield flags)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    GLsync result = d->FenceSync(condition, flags);
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLExtraFunctions::glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->FlushMappedBufferRange(target, offset, length);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->FramebufferTextureLayer(target, attachment, texture, level, layer);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGenQueries(GLsizei n, GLuint* ids)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GenQueries(n, ids);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGenSamplers(GLsizei count, GLuint* samplers)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GenSamplers(count, samplers);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGenTransformFeedbacks(GLsizei n, GLuint* ids)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GenTransformFeedbacks(n, ids);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGenVertexArrays(GLsizei n, GLuint* arrays)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GenVertexArrays(n, arrays);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei* length, GLchar* uniformBlockName)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetActiveUniformBlockName(program, uniformBlockIndex, bufSize, length, uniformBlockName);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetActiveUniformBlockiv(program, uniformBlockIndex, pname, params);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint * uniformIndices, GLenum pname, GLint* params)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetActiveUniformsiv(program, uniformCount, uniformIndices, pname, params);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGetBufferParameteri64v(GLenum target, GLenum pname, GLint64* params)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetBufferParameteri64v(target, pname, params);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGetBufferPointerv(GLenum target, GLenum pname, void ** params)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetBufferPointerv(target, pname, params);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLint QOpenGLExtraFunctions::glGetFragDataLocation(GLuint program, const GLchar * name)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    GLint result = d->GetFragDataLocation(program, name);
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLExtraFunctions::glGetInteger64i_v(GLenum target, GLuint index, GLint64* data)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetInteger64i_v(target, index, data);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGetInteger64v(GLenum pname, GLint64* data)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetInteger64v(pname, data);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGetIntegeri_v(GLenum target, GLuint index, GLint* data)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetIntegeri_v(target, index, data);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGetInternalformativ(GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint* params)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetInternalformativ(target, internalformat, pname, bufSize, params);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGetProgramBinary(GLuint program, GLsizei bufSize, GLsizei* length, GLenum* binaryFormat, void * binary)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetProgramBinary(program, bufSize, length, binaryFormat, binary);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint* params)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetQueryObjectuiv(id, pname, params);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGetQueryiv(GLenum target, GLenum pname, GLint* params)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetQueryiv(target, pname, params);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGetSamplerParameterfv(GLuint sampler, GLenum pname, GLfloat* params)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetSamplerParameterfv(sampler, pname, params);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGetSamplerParameteriv(GLuint sampler, GLenum pname, GLint* params)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetSamplerParameteriv(sampler, pname, params);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline const GLubyte * QOpenGLExtraFunctions::glGetStringi(GLenum name, GLuint index)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    const GLubyte * result = d->GetStringi(name, index);
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLExtraFunctions::glGetSynciv(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei* length, GLint* values)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetSynciv(sync, pname, bufSize, length, values);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGetTransformFeedbackVarying(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLsizei* size, GLenum* type, GLchar* name)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetTransformFeedbackVarying(program, index, bufSize, length, size, type, name);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLuint QOpenGLExtraFunctions::glGetUniformBlockIndex(GLuint program, const GLchar * uniformBlockName)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    GLuint result = d->GetUniformBlockIndex(program, uniformBlockName);
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLExtraFunctions::glGetUniformIndices(GLuint program, GLsizei uniformCount, const GLchar *const* uniformNames, GLuint* uniformIndices)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetUniformIndices(program, uniformCount, uniformNames, uniformIndices);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGetUniformuiv(GLuint program, GLint location, GLuint* params)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetUniformuiv(program, location, params);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGetVertexAttribIiv(GLuint index, GLenum pname, GLint* params)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetVertexAttribIiv(index, pname, params);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGetVertexAttribIuiv(GLuint index, GLenum pname, GLuint* params)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetVertexAttribIuiv(index, pname, params);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum * attachments)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->InvalidateFramebuffer(target, numAttachments, attachments);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glInvalidateSubFramebuffer(GLenum target, GLsizei numAttachments, const GLenum * attachments, GLint x, GLint y, GLsizei width, GLsizei height)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->InvalidateSubFramebuffer(target, numAttachments, attachments, x, y, width, height);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLboolean QOpenGLExtraFunctions::glIsQuery(GLuint id)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    GLboolean result = d->IsQuery(id);
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline GLboolean QOpenGLExtraFunctions::glIsSampler(GLuint sampler)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    GLboolean result = d->IsSampler(sampler);
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline GLboolean QOpenGLExtraFunctions::glIsSync(GLsync sync)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    GLboolean result = d->IsSync(sync);
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline GLboolean QOpenGLExtraFunctions::glIsTransformFeedback(GLuint id)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    GLboolean result = d->IsTransformFeedback(id);
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline GLboolean QOpenGLExtraFunctions::glIsVertexArray(GLuint array)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    GLboolean result = d->IsVertexArray(array);
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void * QOpenGLExtraFunctions::glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    void *result = d->MapBufferRange(target, offset, length, access);
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLExtraFunctions::glPauseTransformFeedback()
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->PauseTransformFeedback();
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramBinary(GLuint program, GLenum binaryFormat, const void * binary, GLsizei length)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramBinary(program, binaryFormat, binary, length);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramParameteri(GLuint program, GLenum pname, GLint value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramParameteri(program, pname, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glReadBuffer(GLenum src)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ReadBuffer(src);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->RenderbufferStorageMultisample(target, samples, internalformat, width, height);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glResumeTransformFeedback()
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ResumeTransformFeedback();
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->SamplerParameterf(sampler, pname, param);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glSamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat * param)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->SamplerParameterfv(sampler, pname, param);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glSamplerParameteri(GLuint sampler, GLenum pname, GLint param)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->SamplerParameteri(sampler, pname, param);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glSamplerParameteriv(GLuint sampler, GLenum pname, const GLint * param)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->SamplerParameteriv(sampler, pname, param);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void * pixels)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->TexImage3D(target, level, internalformat, width, height, depth, border, format, type, pixels);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->TexStorage2D(target, levels, internalformat, width, height);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->TexStorage3D(target, levels, internalformat, width, height, depth);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void * pixels)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->TexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glTransformFeedbackVaryings(GLuint program, GLsizei count, const GLchar *const* varyings, GLenum bufferMode)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->TransformFeedbackVaryings(program, count, varyings, bufferMode);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glUniform1ui(GLint location, GLuint v0)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->Uniform1ui(location, v0);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glUniform1uiv(GLint location, GLsizei count, const GLuint * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->Uniform1uiv(location, count, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glUniform2ui(GLint location, GLuint v0, GLuint v1)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->Uniform2ui(location, v0, v1);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glUniform2uiv(GLint location, GLsizei count, const GLuint * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->Uniform2uiv(location, count, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->Uniform3ui(location, v0, v1, v2);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glUniform3uiv(GLint location, GLsizei count, const GLuint * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->Uniform3uiv(location, count, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->Uniform4ui(location, v0, v1, v2, v3);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glUniform4uiv(GLint location, GLsizei count, const GLuint * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->Uniform4uiv(location, count, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->UniformBlockBinding(program, uniformBlockIndex, uniformBlockBinding);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->UniformMatrix2x3fv(location, count, transpose, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->UniformMatrix2x4fv(location, count, transpose, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->UniformMatrix3x2fv(location, count, transpose, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->UniformMatrix3x4fv(location, count, transpose, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->UniformMatrix4x2fv(location, count, transpose, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->UniformMatrix4x3fv(location, count, transpose, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLboolean QOpenGLExtraFunctions::glUnmapBuffer(GLenum target)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    GLboolean result = d->UnmapBuffer(target);
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLExtraFunctions::glVertexAttribDivisor(GLuint index, GLuint divisor)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->VertexAttribDivisor(index, divisor);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glVertexAttribI4i(GLuint index, GLint x, GLint y, GLint z, GLint w)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->VertexAttribI4i(index, x, y, z, w);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glVertexAttribI4iv(GLuint index, const GLint * v)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->VertexAttribI4iv(index, v);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glVertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->VertexAttribI4ui(index, x, y, z, w);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glVertexAttribI4uiv(GLuint index, const GLuint * v)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->VertexAttribI4uiv(index, v);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void * pointer)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->VertexAttribIPointer(index, size, type, stride, pointer);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->WaitSync(sync, flags, timeout);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glActiveShaderProgram(GLuint pipeline, GLuint program)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ActiveShaderProgram(pipeline, program);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glBindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->BindImageTexture(unit, texture, level, layered, layer, access, format);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glBindProgramPipeline(GLuint pipeline)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->BindProgramPipeline(pipeline);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glBindVertexBuffer(GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->BindVertexBuffer(bindingindex, buffer, offset, stride);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLuint QOpenGLExtraFunctions::glCreateShaderProgramv(GLenum type, GLsizei count, const GLchar *const* strings)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    GLuint result = d->CreateShaderProgramv(type, count, strings);
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLExtraFunctions::glDeleteProgramPipelines(GLsizei n, const GLuint * pipelines)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->DeleteProgramPipelines(n, pipelines);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glDispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->DispatchCompute(num_groups_x, num_groups_y, num_groups_z);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glDispatchComputeIndirect(GLintptr indirect)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->DispatchComputeIndirect(indirect);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glDrawArraysIndirect(GLenum mode, const void * indirect)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->DrawArraysIndirect(mode, indirect);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glDrawElementsIndirect(GLenum mode, GLenum type, const void * indirect)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->DrawElementsIndirect(mode, type, indirect);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glFramebufferParameteri(GLenum target, GLenum pname, GLint param)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->FramebufferParameteri(target, pname, param);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGenProgramPipelines(GLsizei n, GLuint* pipelines)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GenProgramPipelines(n, pipelines);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGetBooleani_v(GLenum target, GLuint index, GLboolean* data)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetBooleani_v(target, index, data);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGetFramebufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetFramebufferParameteriv(target, pname, params);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGetMultisamplefv(GLenum pname, GLuint index, GLfloat* val)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetMultisamplefv(pname, index, val);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGetProgramInterfaceiv(GLuint program, GLenum programInterface, GLenum pname, GLint* params)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetProgramInterfaceiv(program, programInterface, pname, params);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGetProgramPipelineInfoLog(GLuint pipeline, GLsizei bufSize, GLsizei* length, GLchar* infoLog)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetProgramPipelineInfoLog(pipeline, bufSize, length, infoLog);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGetProgramPipelineiv(GLuint pipeline, GLenum pname, GLint* params)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetProgramPipelineiv(pipeline, pname, params);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLuint QOpenGLExtraFunctions::glGetProgramResourceIndex(GLuint program, GLenum programInterface, const GLchar * name)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    GLuint result = d->GetProgramResourceIndex(program, programInterface, name);
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline GLint QOpenGLExtraFunctions::glGetProgramResourceLocation(GLuint program, GLenum programInterface, const GLchar * name)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    GLint result = d->GetProgramResourceLocation(program, programInterface, name);
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLExtraFunctions::glGetProgramResourceName(GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei* length, GLchar* name)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetProgramResourceName(program, programInterface, index, bufSize, length, name);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGetProgramResourceiv(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum * props, GLsizei bufSize, GLsizei* length, GLint* params)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetProgramResourceiv(program, programInterface, index, propCount, props, bufSize, length, params);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat* params)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetTexLevelParameterfv(target, level, pname, params);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint* params)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->GetTexLevelParameteriv(target, level, pname, params);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLboolean QOpenGLExtraFunctions::glIsProgramPipeline(GLuint pipeline)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    GLboolean result = d->IsProgramPipeline(pipeline);
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLExtraFunctions::glMemoryBarrier(GLbitfield barriers)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->MemoryBarrierFunc(barriers);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glMemoryBarrierByRegion(GLbitfield barriers)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->MemoryBarrierByRegion(barriers);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniform1f(GLuint program, GLint location, GLfloat v0)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniform1f(program, location, v0);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniform1fv(GLuint program, GLint location, GLsizei count, const GLfloat * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniform1fv(program, location, count, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniform1i(GLuint program, GLint location, GLint v0)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniform1i(program, location, v0);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniform1iv(GLuint program, GLint location, GLsizei count, const GLint * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniform1iv(program, location, count, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniform1ui(GLuint program, GLint location, GLuint v0)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniform1ui(program, location, v0);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniform1uiv(GLuint program, GLint location, GLsizei count, const GLuint * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniform1uiv(program, location, count, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniform2f(GLuint program, GLint location, GLfloat v0, GLfloat v1)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniform2f(program, location, v0, v1);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniform2fv(GLuint program, GLint location, GLsizei count, const GLfloat * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniform2fv(program, location, count, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniform2i(GLuint program, GLint location, GLint v0, GLint v1)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniform2i(program, location, v0, v1);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniform2iv(GLuint program, GLint location, GLsizei count, const GLint * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniform2iv(program, location, count, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniform2ui(GLuint program, GLint location, GLuint v0, GLuint v1)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniform2ui(program, location, v0, v1);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniform2uiv(GLuint program, GLint location, GLsizei count, const GLuint * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniform2uiv(program, location, count, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniform3f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniform3f(program, location, v0, v1, v2);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniform3fv(GLuint program, GLint location, GLsizei count, const GLfloat * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniform3fv(program, location, count, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniform3i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniform3i(program, location, v0, v1, v2);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniform3iv(GLuint program, GLint location, GLsizei count, const GLint * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniform3iv(program, location, count, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniform3ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniform3ui(program, location, v0, v1, v2);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniform3uiv(GLuint program, GLint location, GLsizei count, const GLuint * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniform3uiv(program, location, count, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniform4f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniform4f(program, location, v0, v1, v2, v3);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniform4fv(GLuint program, GLint location, GLsizei count, const GLfloat * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniform4fv(program, location, count, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniform4i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniform4i(program, location, v0, v1, v2, v3);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniform4iv(GLuint program, GLint location, GLsizei count, const GLint * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniform4iv(program, location, count, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniform4ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniform4ui(program, location, v0, v1, v2, v3);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniform4uiv(GLuint program, GLint location, GLsizei count, const GLuint * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniform4uiv(program, location, count, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniformMatrix2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniformMatrix2fv(program, location, count, transpose, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniformMatrix2x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniformMatrix2x3fv(program, location, count, transpose, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniformMatrix2x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniformMatrix2x4fv(program, location, count, transpose, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniformMatrix3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniformMatrix3fv(program, location, count, transpose, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniformMatrix3x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniformMatrix3x2fv(program, location, count, transpose, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniformMatrix3x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniformMatrix3x4fv(program, location, count, transpose, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniformMatrix4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniformMatrix4fv(program, location, count, transpose, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniformMatrix4x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniformMatrix4x2fv(program, location, count, transpose, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glProgramUniformMatrix4x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ProgramUniformMatrix4x3fv(program, location, count, transpose, value);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glSampleMaski(GLuint maskNumber, GLbitfield mask)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->SampleMaski(maskNumber, mask);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glTexStorage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->TexStorage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glUseProgramStages(GLuint pipeline, GLbitfield stages, GLuint program)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->UseProgramStages(pipeline, stages, program);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glValidateProgramPipeline(GLuint pipeline)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->ValidateProgramPipeline(pipeline);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glVertexAttribBinding(GLuint attribindex, GLuint bindingindex)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->VertexAttribBinding(attribindex, bindingindex);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glVertexAttribFormat(GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->VertexAttribFormat(attribindex, size, type, normalized, relativeoffset);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glVertexAttribIFormat(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->VertexAttribIFormat(attribindex, size, type, relativeoffset);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtraFunctions::glVertexBindingDivisor(GLuint bindingindex, GLuint divisor)
{
    Q_D(QOpenGLExtraFunctions);
    Q_ASSERT(QOpenGLExtraFunctions::isInitialized(d));
    d->VertexBindingDivisor(bindingindex, divisor);
    Q_OPENGL_FUNCTIONS_DEBUG
}

QT_END_NAMESPACE

#endif // QT_NO_OPENGL

#endif
