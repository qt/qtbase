// Copyright (c) 2013 Google Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

  // (GLenum) => void
  var OpenGLES2_ActiveTexture = function(context, texture) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.activeTexture(texture);
  }

  // ppapi (GLuint, GLuint) => void
  // webgl (WebGLProgram, WebGLShader) => void
  var OpenGLES2_AttachShader = function(context, program, shader) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    var _program = resources.resolve(program, PROGRAM_RESOURCE);
    if (_program === undefined) {
      return;
    }
    var _shader = resources.resolve(shader, SHADER_RESOURCE);
    if (_shader === undefined) {
      return;
    }
    _context.ctx.attachShader(_program.native, _shader.native);
  }

  // ppapi (GLuint, GLuint, const char*) => void
  // webgl (WebGLProgram, GLuint, DOMString) => void
  var OpenGLES2_BindAttribLocation = function(context, program, index, name) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    var _program = resources.resolve(program, PROGRAM_RESOURCE);
    if (_program === undefined) {
      return;
    }
    var _name = Pointer_stringify(name);
    _context.ctx.bindAttribLocation(_program.native, index, _name);
  }

  // ppapi (GLenum, GLuint) => void
  // webgl (GLenum, WebGLBuffer) => void
  var OpenGLES2_BindBuffer = function(context, target, buffer) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    var _buffer = resources.resolve(buffer, BUFFER_RESOURCE);
    if (_buffer === undefined) {
      return;
    }
    _context.ctx.bindBuffer(target, _buffer.native);
  }

  // ppapi (GLenum, GLuint) => void
  // webgl (GLenum, WebGLFramebuffer) => void
  var OpenGLES2_BindFramebuffer = function(context, target, framebuffer) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    var _framebuffer = coerceFramebuffer(framebuffer);
    _context.ctx.bindFramebuffer(target, _framebuffer);
  }

  // ppapi (GLenum, GLuint) => void
  // webgl (GLenum, WebGLRenderbuffer) => void
  var OpenGLES2_BindRenderbuffer = function(context, target, renderbuffer) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    var _renderbuffer = coerceRenderbuffer(renderbuffer);
    _context.ctx.bindRenderbuffer(target, _renderbuffer);
  }

  // ppapi (GLenum, GLuint) => void
  // webgl (GLenum, WebGLTexture) => void
  var OpenGLES2_BindTexture = function(context, target, texture) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    var _texture = resources.resolve(texture, TEXTURE_RESOURCE);
    if (_texture === undefined) {
      return;
    }
    _context.ctx.bindTexture(target, _texture.native);
  }

  // (GLclampf, GLclampf, GLclampf, GLclampf) => void
  var OpenGLES2_BlendColor = function(context, red, green, blue, alpha) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.blendColor(red, green, blue, alpha);
  }

  // (GLenum) => void
  var OpenGLES2_BlendEquation = function(context, mode) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.blendEquation(mode);
  }

  // (GLenum, GLenum) => void
  var OpenGLES2_BlendEquationSeparate = function(context, modeRGB, modeAlpha) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.blendEquationSeparate(modeRGB, modeAlpha);
  }

  // (GLenum, GLenum) => void
  var OpenGLES2_BlendFunc = function(context, sfactor, dfactor) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.blendFunc(sfactor, dfactor);
  }

  // (GLenum, GLenum, GLenum, GLenum) => void
  var OpenGLES2_BlendFuncSeparate = function(context, srcRGB, dstRGB, srcAlpha, dstAlpha) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.blendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
  }

  var OpenGLES2_BufferData = function(context, target, size, data, usage) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    var _data = HEAP8.subarray(data, data + size);
    _context.ctx.bufferData(target, _data, usage);
  }
  var OpenGLES2_BufferSubData = function(context, target, offset, size, data) {
    throw "OpenGLES2_BufferSubData not implemented";
  }

  // (GLenum) => GLenum
  var OpenGLES2_CheckFramebufferStatus = function(context, target) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return 0x8CDD;
    }
    return _context.ctx.checkFramebufferStatus(target);
  }

  // (GLbitfield) => void
  var OpenGLES2_Clear = function(context, mask) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.clear(mask);
  }

  // (GLclampf, GLclampf, GLclampf, GLclampf) => void
  var OpenGLES2_ClearColor = function(context, red, green, blue, alpha) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.clearColor(red, green, blue, alpha);
  }

  // (GLclampf) => void
  var OpenGLES2_ClearDepthf = function(context, depth) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    _context.ctx.clearDepth(depth);
  }

  // (GLint) => void
  var OpenGLES2_ClearStencil = function(context, s) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.clearStencil(s);
  }

  // (GLboolean, GLboolean, GLboolean, GLboolean) => void
  var OpenGLES2_ColorMask = function(context, red, green, blue, alpha) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.colorMask(red, green, blue, alpha);
  }

  // ppapi (GLuint) => void
  // webgl (WebGLShader) => void
  var OpenGLES2_CompileShader = function(context, shader) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    var _shader = resources.resolve(shader, SHADER_RESOURCE);
    if (_shader === undefined) {
      return;
    }
    _context.ctx.compileShader(_shader.native);
  }

  var OpenGLES2_CompressedTexImage2D = function(context, target, level, internalformat, width, height, border, imageSize, data) {
    throw "OpenGLES2_CompressedTexImage2D not implemented";
  }

  var OpenGLES2_CompressedTexSubImage2D = function(context, target, level, xoffset, yoffset, width, height, format, imageSize, data) {
    throw "OpenGLES2_CompressedTexSubImage2D not implemented";
  }

  // (GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint) => void
  var OpenGLES2_CopyTexImage2D = function(context, target, level, internalformat, x, y, width, height, border) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.copyTexImage2D(target, level, internalformat, x, y, width, height, border);
  }

  // (GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei) => void
  var OpenGLES2_CopyTexSubImage2D = function(context, target, level, xoffset, yoffset, x, y, width, height) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.copyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
  }

  // ppapi () => GLuint
  // webgl () => WebGLProgram
  var OpenGLES2_CreateProgram = function(context) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return 0;
    }
    return resources.register(PROGRAM_RESOURCE, {native: _context.ctx.createProgram()});
  }

  // ppapi (GLenum) => GLuint
  // webgl (GLenum) => WebGLShader
  var OpenGLES2_CreateShader = function(context, type) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return 0;
    }
    return resources.register(SHADER_RESOURCE, {native: _context.ctx.createShader(type)});
  }

  // (GLenum) => void
  var OpenGLES2_CullFace = function(context, mode) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.cullFace(mode);
  }

  var OpenGLES2_DeleteBuffers = function(context, n, buffers) {
    throw "OpenGLES2_DeleteBuffers not implemented";
  }

  var OpenGLES2_DeleteFramebuffers = function(context, n, framebuffers) {
    throw "OpenGLES2_DeleteFramebuffers not implemented";
  }

  // ppapi (GLuint) => void
  // webgl (WebGLProgram) => void
  var OpenGLES2_DeleteProgram = function(context, program) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    var _program = resources.resolve(program, PROGRAM_RESOURCE);
    if (_program === undefined) {
      return;
    }
    _context.ctx.deleteProgram(_program.native);
  }

  var OpenGLES2_DeleteRenderbuffers = function(context, n, renderbuffers) {
    throw "OpenGLES2_DeleteRenderbuffers not implemented";
  }

  // ppapi (GLuint) => void
  // webgl (WebGLShader) => void
  var OpenGLES2_DeleteShader = function(context, shader) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    var _shader = resources.resolve(shader, SHADER_RESOURCE);
    if (_shader === undefined) {
      return;
    }
    _context.ctx.deleteShader(_shader.native);
  }

  var OpenGLES2_DeleteTextures = function(context, n, textures) {
    throw "OpenGLES2_DeleteTextures not implemented";
  }

  // (GLenum) => void
  var OpenGLES2_DepthFunc = function(context, func) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.depthFunc(func);
  }

  // (GLboolean) => void
  var OpenGLES2_DepthMask = function(context, flag) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.depthMask(flag);
  }

  // (GLclampf, GLclampf) => void
  var OpenGLES2_DepthRangef = function(context, zNear, zFar) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.depthRange(zNear, zFar);
  }

  // ppapi (GLuint, GLuint) => void
  // webgl (WebGLProgram, WebGLShader) => void
  var OpenGLES2_DetachShader = function(context, program, shader) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE)
    if (_context === undefined) {
      return;
    }
    var _program = resources.resolve(program, PROGRAM_RESOURCE);
    if (_program === undefined) {
      return;
    }
    var _shader = resources.resolve(shader, SHADER_RESOURCE);
    if (_shader === undefined) {
      return;
    }
    _context.ctx.detachShader(_program.native, _shader.native);
  }

  // (GLenum) => void
  var OpenGLES2_Disable = function(context, cap) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.disable(cap);
  }

  // (GLuint) => void
  var OpenGLES2_DisableVertexAttribArray = function(context, index) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.disableVertexAttribArray(index);
  }

  // (GLenum, GLint, GLsizei) => void
  var OpenGLES2_DrawArrays = function(context, mode, first, count) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.drawArrays(mode, first, count);
  }

  // ppapi (GLenum, GLsizei, GLenum, const void*) => void
  // webgl (GLenum, GLsizei, GLenum, GLintptr) => void
  var OpenGLES2_DrawElements = function(context, mode, count, type, indices) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.drawElements(mode, count, type, indices);
  }

  // (GLenum) => void
  var OpenGLES2_Enable = function(context, cap) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.enable(cap);
  }

  // (GLuint) => void
  var OpenGLES2_EnableVertexAttribArray = function(context, index) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.enableVertexAttribArray(index);
  }

  // () => void
  var OpenGLES2_Finish = function(context) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.finish();
  }

  // () => void
  var OpenGLES2_Flush = function(context) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.flush();
  }

  // ppapi (GLenum, GLenum, GLenum, GLuint) => void
  // webgl (GLenum, GLenum, GLenum, WebGLRenderbuffer) => void
  var OpenGLES2_FramebufferRenderbuffer = function(context, target, attachment, renderbuffertarget, renderbuffer) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    var _renderbuffer = coerceRenderbuffer(renderbuffer);
    _context.ctx.framebufferRenderbuffer(target, attachment, renderbuffertarget, _renderbuffer);
  }

  // ppapi (GLenum, GLenum, GLenum, GLuint, GLint) => void
  // webgl (GLenum, GLenum, GLenum, WebGLTexture, GLint) => void
  var OpenGLES2_FramebufferTexture2D = function(context, target, attachment, textarget, texture, level) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    var _texture = resources.resolve(texture, TEXTURE_RESOURCE);
    if (_texture === undefined) {
      return;
    }
    _context.ctx.framebufferTexture2D(target, attachment, textarget, _texture.native, level);
  }

  // (GLenum) => void
  var OpenGLES2_FrontFace = function(context, mode) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.frontFace(mode);
  }

  var OpenGLES2_GenBuffers = function(context, n, buffers) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    for (var i = 0; i < n; i++) {
      setValue(buffers + i * 4, resources.register(BUFFER_RESOURCE, {native: _context.ctx.createBuffer()}), 'i32');
    }
  }
  // (GLenum) => void
  var OpenGLES2_GenerateMipmap = function(context, target) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.generateMipmap(target);
  }

  var OpenGLES2_GenFramebuffers = function(context, n, framebuffers) {
    throw "OpenGLES2_GenFramebuffers not implemented";
  }

  var OpenGLES2_GenRenderbuffers = function(context, n, renderbuffers) {
    throw "OpenGLES2_GenRenderbuffers not implemented";
  }

  var OpenGLES2_GenTextures = function(context, n, textures) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    for (var i = 0; i < n; i++) {
      setValue(textures + i * 4, resources.register(TEXTURE_RESOURCE, {native: _context.ctx.createTexture()}), 'i32');
    }
  }
  var OpenGLES2_GetActiveAttrib = function(context, program, index, bufsize, length, size, type, name) {
    throw "OpenGLES2_GetActiveAttrib not implemented";
  }

  var OpenGLES2_GetActiveUniform = function(context, program, index, bufsize, length, size, type, name) {
    throw "OpenGLES2_GetActiveUniform not implemented";
  }

  var OpenGLES2_GetAttachedShaders = function(context, program, maxcount, count, shaders) {
    throw "OpenGLES2_GetAttachedShaders not implemented";
  }

  // ppapi (GLuint, const char*) => GLint
  // webgl (WebGLProgram, DOMString) => GLint
  var OpenGLES2_GetAttribLocation = function(context, program, name) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return -1;
    }
    var _program = resources.resolve(program, PROGRAM_RESOURCE);
    if (_program === undefined) {
      return;
    }
    var _name = Pointer_stringify(name);
    return _context.ctx.getAttribLocation(_program.native, _name);
  }

  var OpenGLES2_GetBooleanv = function(context, pname, params) {
    throw "OpenGLES2_GetBooleanv not implemented";
  }

  var OpenGLES2_GetBufferParameteriv = function(context, target, pname, params) {
    throw "OpenGLES2_GetBufferParameteriv not implemented";
  }

  // () => GLenum
  var OpenGLES2_GetError = function(context) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return 0;
    }
    return _context.ctx.getError();
  }

  var OpenGLES2_GetFloatv = function(context, pname, params) {
    throw "OpenGLES2_GetFloatv not implemented";
  }

  var OpenGLES2_GetFramebufferAttachmentParameteriv = function(context, target, attachment, pname, params) {
    throw "OpenGLES2_GetFramebufferAttachmentParameteriv not implemented";
  }

  var OpenGLES2_GetIntegerv = function(context, pname, params) {
    throw "OpenGLES2_GetIntegerv not implemented";
  }

  var OpenGLES2_GetProgramiv = function(context, program, pname, params) {
    throw "OpenGLES2_GetProgramiv not implemented";
  }

  var OpenGLES2_GetProgramInfoLog = function(context, program, bufsize, length, infolog) {
    throw "OpenGLES2_GetProgramInfoLog not implemented";
  }

  var OpenGLES2_GetRenderbufferParameteriv = function(context, target, pname, params) {
    throw "OpenGLES2_GetRenderbufferParameteriv not implemented";
  }

  var OpenGLES2_GetShaderiv = function(context, shader, pname, params) {
    throw "OpenGLES2_GetShaderiv not implemented";
  }

  var OpenGLES2_GetShaderInfoLog = function(context, shader, bufsize, length, infolog) {
    throw "OpenGLES2_GetShaderInfoLog not implemented";
  }

  var OpenGLES2_GetShaderPrecisionFormat = function(context, shadertype, precisiontype, range, precision) {
    throw "OpenGLES2_GetShaderPrecisionFormat not implemented";
  }

  var OpenGLES2_GetShaderSource = function(context, shader, bufsize, length, source) {
    throw "OpenGLES2_GetShaderSource not implemented";
  }

  var OpenGLES2_GetString = function(context, name) {
    throw "OpenGLES2_GetString not implemented";
  }

  var OpenGLES2_GetTexParameterfv = function(context, target, pname, params) {
    throw "OpenGLES2_GetTexParameterfv not implemented";
  }

  var OpenGLES2_GetTexParameteriv = function(context, target, pname, params) {
    throw "OpenGLES2_GetTexParameteriv not implemented";
  }

  var OpenGLES2_GetUniformfv = function(context, program, location, params) {
    throw "OpenGLES2_GetUniformfv not implemented";
  }

  var OpenGLES2_GetUniformiv = function(context, program, location, params) {
    throw "OpenGLES2_GetUniformiv not implemented";
  }

  // ppapi (GLuint, const char*) => GLint
  // webgl (WebGLProgram, DOMString) => WebGLUniformLocation
  var OpenGLES2_GetUniformLocation = function(context, program, name) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return 0;
    }
    var _program = resources.resolve(program, PROGRAM_RESOURCE);
    if (_program === undefined) {
      return;
    }
    var _name = Pointer_stringify(name);
    return resources.register(UNIFORM_LOCATION_RESOURCE, {native: _context.ctx.getUniformLocation(_program.native, _name)});
  }

  var OpenGLES2_GetVertexAttribfv = function(context, index, pname, params) {
    throw "OpenGLES2_GetVertexAttribfv not implemented";
  }

  var OpenGLES2_GetVertexAttribiv = function(context, index, pname, params) {
    throw "OpenGLES2_GetVertexAttribiv not implemented";
  }

  var OpenGLES2_GetVertexAttribPointerv = function(context, index, pname, pointer) {
    throw "OpenGLES2_GetVertexAttribPointerv not implemented";
  }

  // (GLenum, GLenum) => void
  var OpenGLES2_Hint = function(context, target, mode) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.hint(target, mode);
  }

  // ppapi (GLuint) => GLboolean
  // webgl (WebGLBuffer) => GLboolean
  var OpenGLES2_IsBuffer = function(context, buffer) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return false;
    }
    var _buffer = resources.resolve(buffer, BUFFER_RESOURCE);
    if (_buffer === undefined) {
      return;
    }
    return _context.ctx.isBuffer(_buffer.native);
  }

  // (GLenum) => GLboolean
  var OpenGLES2_IsEnabled = function(context, cap) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return false;
    }
    return _context.ctx.isEnabled(cap);
  }

  // ppapi (GLuint) => GLboolean
  // webgl (WebGLFramebuffer) => GLboolean
  var OpenGLES2_IsFramebuffer = function(context, framebuffer) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return false;
    }
    var _framebuffer = coerceFramebuffer(framebuffer);
    return _context.ctx.isFramebuffer(_framebuffer);
  }

  // ppapi (GLuint) => GLboolean
  // webgl (WebGLProgram) => GLboolean
  var OpenGLES2_IsProgram = function(context, program) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return false;
    }
    var _program = resources.resolve(program, PROGRAM_RESOURCE);
    if (_program === undefined) {
      return;
    }
    return _context.ctx.isProgram(_program.native);
  }

  // ppapi (GLuint) => GLboolean
  // webgl (WebGLRenderbuffer) => GLboolean
  var OpenGLES2_IsRenderbuffer = function(context, renderbuffer) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return false;
    }
    var _renderbuffer = coerceRenderbuffer(renderbuffer);
    return _context.ctx.isRenderbuffer(_renderbuffer);
  }

  // ppapi (GLuint) => GLboolean
  // webgl (WebGLShader) => GLboolean
  var OpenGLES2_IsShader = function(context, shader) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return false;
    }
    var _shader = resources.resolve(shader, SHADER_RESOURCE);
    if (_shader === undefined) {
      return;
    }
    return _context.ctx.isShader(_shader.native);
  }

  // ppapi (GLuint) => GLboolean
  // webgl (WebGLTexture) => GLboolean
  var OpenGLES2_IsTexture = function(context, texture) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return false;
    }
    var _texture = resources.resolve(texture, TEXTURE_RESOURCE);
    if (_texture === undefined) {
      return;
    }
    return _context.ctx.isTexture(_texture.native);
  }

  // (GLfloat) => void
  var OpenGLES2_LineWidth = function(context, width) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.lineWidth(width);
  }

  // ppapi (GLuint) => void
  // webgl (WebGLProgram) => void
  var OpenGLES2_LinkProgram = function(context, program) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    var _program = resources.resolve(program, PROGRAM_RESOURCE);
    if (_program === undefined) {
      return;
    }
    _context.ctx.linkProgram(_program.native);
  }

  // (GLenum, GLint) => void
  var OpenGLES2_PixelStorei = function(context, pname, param) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.pixelStorei(pname, param);
  }

  // (GLfloat, GLfloat) => void
  var OpenGLES2_PolygonOffset = function(context, factor, units) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.polygonOffset(factor, units);
  }

  var OpenGLES2_ReadPixels = function(context, x, y, width, height, format, type, pixels) {
    throw "OpenGLES2_ReadPixels not implemented";
  }

  var OpenGLES2_ReleaseShaderCompiler = function(context) {
    throw "OpenGLES2_ReleaseShaderCompiler not implemented";
  }

  // (GLenum, GLenum, GLsizei, GLsizei) => void
  var OpenGLES2_RenderbufferStorage = function(context, target, internalformat, width, height) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.renderbufferStorage(target, internalformat, width, height);
  }

  // (GLclampf, GLboolean) => void
  var OpenGLES2_SampleCoverage = function(context, value, invert) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.sampleCoverage(value, invert);
  }

  // (GLint, GLint, GLsizei, GLsizei) => void
  var OpenGLES2_Scissor = function(context, x, y, width, height) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.scissor(x, y, width, height);
  }

  var OpenGLES2_ShaderBinary = function(context, n, shaders, binaryformat, binary, length) {
    throw "OpenGLES2_ShaderBinary not implemented";
  }

  var OpenGLES2_ShaderSource = function(context, shader, count, str, length) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    var _shader = resources.resolve(shader, SHADER_RESOURCE);
    if (_shader === undefined) {
      return;
    }
    var chunks = [];
    for (var i = 0; i < count; i++) {
      var str_ptr = getValue(str + i * 4, 'i32');
      var l = 0;
      if (length != 0) {
        l = getValue(length + i * 4, 'i32');
      }
      if (l <= 0) {
        chunks.push(Pointer_stringify(str_ptr));
      } else {
        chunks.push(Pointer_stringify(str_ptr, l));
      }
    }
    _context.ctx.shaderSource(_shader.native, chunks.join(""));
  }
  // (GLenum, GLint, GLuint) => void
  var OpenGLES2_StencilFunc = function(context, func, ref, mask) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.stencilFunc(func, ref, mask);
  }

  // (GLenum, GLenum, GLint, GLuint) => void
  var OpenGLES2_StencilFuncSeparate = function(context, face, func, ref, mask) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.stencilFuncSeparate(face, func, ref, mask);
  }

  // (GLuint) => void
  var OpenGLES2_StencilMask = function(context, mask) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.stencilMask(mask);
  }

  // (GLenum, GLuint) => void
  var OpenGLES2_StencilMaskSeparate = function(context, face, mask) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.stencilMaskSeparate(face, mask);
  }

  // (GLenum, GLenum, GLenum) => void
  var OpenGLES2_StencilOp = function(context, fail, zfail, zpass) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.stencilOp(fail, zfail, zpass);
  }

  // (GLenum, GLenum, GLenum, GLenum) => void
  var OpenGLES2_StencilOpSeparate = function(context, face, fail, zfail, zpass) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.stencilOpSeparate(face, fail, zfail, zpass);
  }

  var OpenGLES2_TexImage2D = function(context, target, level, internalformat, width, height, border, format, type, pixels) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    // TODO handle other pixel formats.
    if (type != 0x1401) throw "Pixel format not supported.";
    var _pixels = HEAPU8.subarray(pixels, pixels + width * height * 4);
    _context.ctx.texImage2D(target, level, internalformat, width, height, border, format, type, _pixels);
  }
  // (GLenum, GLenum, GLfloat) => void
  var OpenGLES2_TexParameterf = function(context, target, pname, param) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.texParameterf(target, pname, param);
  }

  var OpenGLES2_TexParameterfv = function(context, target, pname, params) {
    throw "OpenGLES2_TexParameterfv not implemented";
  }

  // (GLenum, GLenum, GLint) => void
  var OpenGLES2_TexParameteri = function(context, target, pname, param) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.texParameteri(target, pname, param);
  }

  var OpenGLES2_TexParameteriv = function(context, target, pname, params) {
    throw "OpenGLES2_TexParameteriv not implemented";
  }

  var OpenGLES2_TexSubImage2D = function(context, target, level, xoffset, yoffset, width, height, format, type, pixels) {
    throw "OpenGLES2_TexSubImage2D not implemented";
  }

  // ppapi (GLint, GLfloat) => void
  // webgl (WebGLUniformLocation, GLfloat) => void
  var OpenGLES2_Uniform1f = function(context, location, x) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    var _location = resources.resolve(location, UNIFORM_LOCATION_RESOURCE);
    if (_location === undefined) {
      return;
    }
    _context.ctx.uniform1f(_location.native, x);
  }

  var OpenGLES2_Uniform1fv = function(context, location, count, v) {
    throw "OpenGLES2_Uniform1fv not implemented";
  }

  // ppapi (GLint, GLint) => void
  // webgl (WebGLUniformLocation, GLint) => void
  var OpenGLES2_Uniform1i = function(context, location, x) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    var _location = resources.resolve(location, UNIFORM_LOCATION_RESOURCE);
    if (_location === undefined) {
      return;
    }
    _context.ctx.uniform1i(_location.native, x);
  }

  var OpenGLES2_Uniform1iv = function(context, location, count, v) {
    throw "OpenGLES2_Uniform1iv not implemented";
  }

  // ppapi (GLint, GLfloat, GLfloat) => void
  // webgl (WebGLUniformLocation, GLfloat, GLfloat) => void
  var OpenGLES2_Uniform2f = function(context, location, x, y) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    var _location = resources.resolve(location, UNIFORM_LOCATION_RESOURCE);
    if (_location === undefined) {
      return;
    }
    _context.ctx.uniform2f(_location.native, x, y);
  }

  var OpenGLES2_Uniform2fv = function(context, location, count, v) {
    throw "OpenGLES2_Uniform2fv not implemented";
  }

  // ppapi (GLint, GLint, GLint) => void
  // webgl (WebGLUniformLocation, GLint, GLint) => void
  var OpenGLES2_Uniform2i = function(context, location, x, y) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    var _location = resources.resolve(location, UNIFORM_LOCATION_RESOURCE);
    if (_location === undefined) {
      return;
    }
    _context.ctx.uniform2i(_location.native, x, y);
  }

  var OpenGLES2_Uniform2iv = function(context, location, count, v) {
    throw "OpenGLES2_Uniform2iv not implemented";
  }

  // ppapi (GLint, GLfloat, GLfloat, GLfloat) => void
  // webgl (WebGLUniformLocation, GLfloat, GLfloat, GLfloat) => void
  var OpenGLES2_Uniform3f = function(context, location, x, y, z) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    var _location = resources.resolve(location, UNIFORM_LOCATION_RESOURCE);
    if (_location === undefined) {
      return;
    }
    _context.ctx.uniform3f(_location.native, x, y, z);
  }

  var OpenGLES2_Uniform3fv = function(context, location, count, v) {
    throw "OpenGLES2_Uniform3fv not implemented";
  }

  // ppapi (GLint, GLint, GLint, GLint) => void
  // webgl (WebGLUniformLocation, GLint, GLint, GLint) => void
  var OpenGLES2_Uniform3i = function(context, location, x, y, z) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    var _location = resources.resolve(location, UNIFORM_LOCATION_RESOURCE);
    if (_location === undefined) {
      return;
    }
    _context.ctx.uniform3i(_location.native, x, y, z);
  }

  var OpenGLES2_Uniform3iv = function(context, location, count, v) {
    throw "OpenGLES2_Uniform3iv not implemented";
  }

  // ppapi (GLint, GLfloat, GLfloat, GLfloat, GLfloat) => void
  // webgl (WebGLUniformLocation, GLfloat, GLfloat, GLfloat, GLfloat) => void
  var OpenGLES2_Uniform4f = function(context, location, x, y, z, w) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    var _location = resources.resolve(location, UNIFORM_LOCATION_RESOURCE);
    if (_location === undefined) {
      return;
    }
    _context.ctx.uniform4f(_location.native, x, y, z, w);
  }

  var OpenGLES2_Uniform4fv = function(context, location, count, v) {
    throw "OpenGLES2_Uniform4fv not implemented";
  }

  // ppapi (GLint, GLint, GLint, GLint, GLint) => void
  // webgl (WebGLUniformLocation, GLint, GLint, GLint, GLint) => void
  var OpenGLES2_Uniform4i = function(context, location, x, y, z, w) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    var _location = resources.resolve(location, UNIFORM_LOCATION_RESOURCE);
    if (_location === undefined) {
      return;
    }
    _context.ctx.uniform4i(_location.native, x, y, z, w);
  }

  var OpenGLES2_Uniform4iv = function(context, location, count, v) {
    throw "OpenGLES2_Uniform4iv not implemented";
  }

  var OpenGLES2_UniformMatrix2fv = function(context, location, count, transpose, value) {
    throw "OpenGLES2_UniformMatrix2fv not implemented";
  }

  var OpenGLES2_UniformMatrix3fv = function(context, location, count, transpose, value) {
    throw "OpenGLES2_UniformMatrix3fv not implemented";
  }

  var OpenGLES2_UniformMatrix4fv = function(context, location, count, transpose, value) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    var _location = resources.resolve(location, UNIFORM_LOCATION_RESOURCE);
    if (_location === undefined) {
      return;
    }
    var _value = HEAPF32.subarray((value>>2), (value>>2) + 16 * count);
    _context.ctx.uniformMatrix4fv(_location.native, transpose, _value);
  }
  // ppapi (GLuint) => void
  // webgl (WebGLProgram) => void
  var OpenGLES2_UseProgram = function(context, program) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    var _program = resources.resolve(program, PROGRAM_RESOURCE);
    if (_program === undefined) {
      return;
    }
    _context.ctx.useProgram(_program.native);
  }

  // ppapi (GLuint) => void
  // webgl (WebGLProgram) => void
  var OpenGLES2_ValidateProgram = function(context, program) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    var _program = resources.resolve(program, PROGRAM_RESOURCE);
    if (_program === undefined) {
      return;
    }
    _context.ctx.validateProgram(_program.native);
  }

  // (GLuint, GLfloat) => void
  var OpenGLES2_VertexAttrib1f = function(context, indx, x) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.vertexAttrib1f(indx, x);
  }

  var OpenGLES2_VertexAttrib1fv = function(context, indx, values) {
    OpenGLES2_VertexAttrib1f(context, indx, HEAPF32[(values>>2)+0]);
  }
  
  // (GLuint, GLfloat, GLfloat) => void
  var OpenGLES2_VertexAttrib2f = function(context, indx, x, y) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.vertexAttrib2f(indx, x, y);
  }

  var OpenGLES2_VertexAttrib2fv = function(context, indx, values) {
    OpenGLES2_VertexAttrib2f(context, indx, HEAPF32[(values>>2)+0], HEAPF32[(values>>2)+1]);
  }
  
  // (GLuint, GLfloat, GLfloat, GLfloat) => void
  var OpenGLES2_VertexAttrib3f = function(context, indx, x, y, z) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.vertexAttrib3f(indx, x, y, z);
  }

  var OpenGLES2_VertexAttrib3fv = function(context, indx, values) {
    OpenGLES2_VertexAttrib3f(context, indx, HEAPF32[(values>>2)+0], HEAPF32[(values>>2)+1], HEAPF32[(values>>2)+2]);
  }
  
  // (GLuint, GLfloat, GLfloat, GLfloat, GLfloat) => void
  var OpenGLES2_VertexAttrib4f = function(context, indx, x, y, z, w) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.vertexAttrib4f(indx, x, y, z, w);
  }

  var OpenGLES2_VertexAttrib4fv = function(context, indx, values) {
    OpenGLES2_VertexAttrib4f(context, indx, HEAPF32[(values>>2)+0], HEAPF32[(values>>2)+1], HEAPF32[(values>>2)+2], HEAPF32[(values>>2)+3]);
  }
  
  // ppapi (GLuint, GLint, GLenum, GLboolean, GLsizei, const void*) => void
  // webgl (GLuint, GLint, GLenum, GLboolean, GLsizei, GLintptr) => void
  var OpenGLES2_VertexAttribPointer = function(context, indx, size, type, normalized, stride, ptr) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.vertexAttribPointer(indx, size, type, normalized, stride, ptr);
  }

  // (GLint, GLint, GLsizei, GLsizei) => void
  var OpenGLES2_Viewport = function(context, x, y, width, height) {
    var _context = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    _context.ctx.viewport(x, y, width, height);
  }

  registerInterface("PPB_OpenGLES2;1.0", [
    OpenGLES2_ActiveTexture,
    OpenGLES2_AttachShader,
    OpenGLES2_BindAttribLocation,
    OpenGLES2_BindBuffer,
    OpenGLES2_BindFramebuffer,
    OpenGLES2_BindRenderbuffer,
    OpenGLES2_BindTexture,
    OpenGLES2_BlendColor,
    OpenGLES2_BlendEquation,
    OpenGLES2_BlendEquationSeparate,
    OpenGLES2_BlendFunc,
    OpenGLES2_BlendFuncSeparate,
    OpenGLES2_BufferData,
    OpenGLES2_BufferSubData,
    OpenGLES2_CheckFramebufferStatus,
    OpenGLES2_Clear,
    OpenGLES2_ClearColor,
    OpenGLES2_ClearDepthf,
    OpenGLES2_ClearStencil,
    OpenGLES2_ColorMask,
    OpenGLES2_CompileShader,
    OpenGLES2_CompressedTexImage2D,
    OpenGLES2_CompressedTexSubImage2D,
    OpenGLES2_CopyTexImage2D,
    OpenGLES2_CopyTexSubImage2D,
    OpenGLES2_CreateProgram,
    OpenGLES2_CreateShader,
    OpenGLES2_CullFace,
    OpenGLES2_DeleteBuffers,
    OpenGLES2_DeleteFramebuffers,
    OpenGLES2_DeleteProgram,
    OpenGLES2_DeleteRenderbuffers,
    OpenGLES2_DeleteShader,
    OpenGLES2_DeleteTextures,
    OpenGLES2_DepthFunc,
    OpenGLES2_DepthMask,
    OpenGLES2_DepthRangef,
    OpenGLES2_DetachShader,
    OpenGLES2_Disable,
    OpenGLES2_DisableVertexAttribArray,
    OpenGLES2_DrawArrays,
    OpenGLES2_DrawElements,
    OpenGLES2_Enable,
    OpenGLES2_EnableVertexAttribArray,
    OpenGLES2_Finish,
    OpenGLES2_Flush,
    OpenGLES2_FramebufferRenderbuffer,
    OpenGLES2_FramebufferTexture2D,
    OpenGLES2_FrontFace,
    OpenGLES2_GenBuffers,
    OpenGLES2_GenerateMipmap,
    OpenGLES2_GenFramebuffers,
    OpenGLES2_GenRenderbuffers,
    OpenGLES2_GenTextures,
    OpenGLES2_GetActiveAttrib,
    OpenGLES2_GetActiveUniform,
    OpenGLES2_GetAttachedShaders,
    OpenGLES2_GetAttribLocation,
    OpenGLES2_GetBooleanv,
    OpenGLES2_GetBufferParameteriv,
    OpenGLES2_GetError,
    OpenGLES2_GetFloatv,
    OpenGLES2_GetFramebufferAttachmentParameteriv,
    OpenGLES2_GetIntegerv,
    OpenGLES2_GetProgramiv,
    OpenGLES2_GetProgramInfoLog,
    OpenGLES2_GetRenderbufferParameteriv,
    OpenGLES2_GetShaderiv,
    OpenGLES2_GetShaderInfoLog,
    OpenGLES2_GetShaderPrecisionFormat,
    OpenGLES2_GetShaderSource,
    OpenGLES2_GetString,
    OpenGLES2_GetTexParameterfv,
    OpenGLES2_GetTexParameteriv,
    OpenGLES2_GetUniformfv,
    OpenGLES2_GetUniformiv,
    OpenGLES2_GetUniformLocation,
    OpenGLES2_GetVertexAttribfv,
    OpenGLES2_GetVertexAttribiv,
    OpenGLES2_GetVertexAttribPointerv,
    OpenGLES2_Hint,
    OpenGLES2_IsBuffer,
    OpenGLES2_IsEnabled,
    OpenGLES2_IsFramebuffer,
    OpenGLES2_IsProgram,
    OpenGLES2_IsRenderbuffer,
    OpenGLES2_IsShader,
    OpenGLES2_IsTexture,
    OpenGLES2_LineWidth,
    OpenGLES2_LinkProgram,
    OpenGLES2_PixelStorei,
    OpenGLES2_PolygonOffset,
    OpenGLES2_ReadPixels,
    OpenGLES2_ReleaseShaderCompiler,
    OpenGLES2_RenderbufferStorage,
    OpenGLES2_SampleCoverage,
    OpenGLES2_Scissor,
    OpenGLES2_ShaderBinary,
    OpenGLES2_ShaderSource,
    OpenGLES2_StencilFunc,
    OpenGLES2_StencilFuncSeparate,
    OpenGLES2_StencilMask,
    OpenGLES2_StencilMaskSeparate,
    OpenGLES2_StencilOp,
    OpenGLES2_StencilOpSeparate,
    OpenGLES2_TexImage2D,
    OpenGLES2_TexParameterf,
    OpenGLES2_TexParameterfv,
    OpenGLES2_TexParameteri,
    OpenGLES2_TexParameteriv,
    OpenGLES2_TexSubImage2D,
    OpenGLES2_Uniform1f,
    OpenGLES2_Uniform1fv,
    OpenGLES2_Uniform1i,
    OpenGLES2_Uniform1iv,
    OpenGLES2_Uniform2f,
    OpenGLES2_Uniform2fv,
    OpenGLES2_Uniform2i,
    OpenGLES2_Uniform2iv,
    OpenGLES2_Uniform3f,
    OpenGLES2_Uniform3fv,
    OpenGLES2_Uniform3i,
    OpenGLES2_Uniform3iv,
    OpenGLES2_Uniform4f,
    OpenGLES2_Uniform4fv,
    OpenGLES2_Uniform4i,
    OpenGLES2_Uniform4iv,
    OpenGLES2_UniformMatrix2fv,
    OpenGLES2_UniformMatrix3fv,
    OpenGLES2_UniformMatrix4fv,
    OpenGLES2_UseProgram,
    OpenGLES2_ValidateProgram,
    OpenGLES2_VertexAttrib1f,
    OpenGLES2_VertexAttrib1fv,
    OpenGLES2_VertexAttrib2f,
    OpenGLES2_VertexAttrib2fv,
    OpenGLES2_VertexAttrib3f,
    OpenGLES2_VertexAttrib3fv,
    OpenGLES2_VertexAttrib4f,
    OpenGLES2_VertexAttrib4fv,
    OpenGLES2_VertexAttribPointer,
    OpenGLES2_Viewport,
  ]);
})();
// BufferSubData: Cannot deal with overloads
// CompressedTexImage2D: No corresponding implementation
// CompressedTexSubImage2D: No corresponding implementation
// DeleteBuffers: No corresponding implementation
// DeleteFramebuffers: No corresponding implementation
// DeleteRenderbuffers: No corresponding implementation
// DeleteTextures: No corresponding implementation
// GenFramebuffers: No corresponding implementation
// GenRenderbuffers: No corresponding implementation
// GetActiveAttrib: Arg count mismatch
// GetActiveUniform: Arg count mismatch
// GetAttachedShaders: Arg count mismatch
// GetBooleanv: No corresponding implementation
// GetBufferParameteriv: No corresponding implementation
// GetFloatv: No corresponding implementation
// GetFramebufferAttachmentParameteriv: No corresponding implementation
// GetIntegerv: No corresponding implementation
// GetProgramiv: No corresponding implementation
// GetProgramInfoLog: Arg count mismatch
// GetRenderbufferParameteriv: No corresponding implementation
// GetShaderiv: No corresponding implementation
// GetShaderInfoLog: Arg count mismatch
// GetShaderPrecisionFormat: No corresponding implementation
// GetShaderSource: Arg count mismatch
// GetString: No corresponding implementation
// GetTexParameterfv: No corresponding implementation
// GetTexParameteriv: No corresponding implementation
// GetUniformfv: No corresponding implementation
// GetUniformiv: No corresponding implementation
// GetVertexAttribfv: No corresponding implementation
// GetVertexAttribiv: No corresponding implementation
// GetVertexAttribPointerv: No corresponding implementation
// ReadPixels: Cannot coerce argument type <void*> to <ArrayBufferView>
// ReleaseShaderCompiler: No corresponding implementation
// ShaderBinary: No corresponding implementation
// TexParameterfv: No corresponding implementation
// TexParameteriv: No corresponding implementation
// TexSubImage2D: Cannot deal with overloads
// Uniform1fv: Cannot deal with overloads
// Uniform1iv: Cannot deal with overloads
// Uniform2fv: Cannot deal with overloads
// Uniform2iv: Cannot deal with overloads
// Uniform3fv: Cannot deal with overloads
// Uniform3iv: Cannot deal with overloads
// Uniform4fv: Cannot deal with overloads
// Uniform4iv: Cannot deal with overloads
// UniformMatrix2fv: Cannot deal with overloads
// UniformMatrix3fv: Cannot deal with overloads
// 94:48
