// Copyright (c) 2013 Google Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
  // PPB_OpenGLES2InstancedArrays

  // (GLenum mode, GLint first, GLsizei count, GLsizei primcount) => void
  var OpenGLES2InstancedArrays_DrawArraysInstancedANGLE = function(context_uid, mode, first, count, primcount) {
    var _context = resources.resolve(context_uid, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    throw "OpenGLES2InstancedArrays_DrawArraysInstancedANGLE not implemented";
  };

  // (GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei primcount) => void
  var OpenGLES2InstancedArrays_DrawElementsInstancedANGLE = function(context_uid, mode, count, type, indices, primcount) {
    var _context = resources.resolve(context_uid, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    throw "OpenGLES2InstancedArrays_DrawElementsInstancedANGLE not implemented";
  };

  // (GLuint index, GLuint divisor) => void
  var OpenGLES2InstancedArrays_VertexAttribDivisorANGLE = function(context_uid, index, divisor) {
    var _context = resources.resolve(context_uid, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    throw "OpenGLES2InstancedArrays_VertexAttribDivisorANGLE not implemented";
  };

  registerInterface("PPB_OpenGLES2InstancedArrays;1.0", [
    OpenGLES2InstancedArrays_DrawArraysInstancedANGLE,
    OpenGLES2InstancedArrays_DrawElementsInstancedANGLE,
    OpenGLES2InstancedArrays_VertexAttribDivisorANGLE,
  ]);



  // PPB_OpenGLES2FramebufferBlit

  // (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) => void
  var OpenGLES2FramebufferBlit_BlitFramebufferEXT = function(context_uid, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter) {
    var _context = resources.resolve(context_uid, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    throw "OpenGLES2FramebufferBlit_BlitFramebufferEXT not implemented";
  };

  registerInterface("PPB_OpenGLES2FramebufferBlit;1.0", [
    OpenGLES2FramebufferBlit_BlitFramebufferEXT,
  ]);



  // PPB_OpenGLES2FramebufferMultisample

  // (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) => void
  var OpenGLES2FramebufferMultisample_RenderbufferStorageMultisampleEXT = function(context_uid, target, samples, internalformat, width, height) {
    var _context = resources.resolve(context_uid, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    throw "OpenGLES2FramebufferMultisample_RenderbufferStorageMultisampleEXT not implemented";
  };

  registerInterface("PPB_OpenGLES2FramebufferMultisample;1.0", [
    OpenGLES2FramebufferMultisample_RenderbufferStorageMultisampleEXT,
  ]);



  // PPB_OpenGLES2ChromiumEnableFeature

  // (const char* feature) => GLboolean
  var OpenGLES2ChromiumEnableFeature_EnableFeatureCHROMIUM = function(context_uid, feature) {
    var _context = resources.resolve(context_uid, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    throw "OpenGLES2ChromiumEnableFeature_EnableFeatureCHROMIUM not implemented";
  };

  registerInterface("PPB_OpenGLES2ChromiumEnableFeature;1.0", [
    OpenGLES2ChromiumEnableFeature_EnableFeatureCHROMIUM,
  ]);



  // PPB_OpenGLES2ChromiumMapSub

  // (GLuint target, GLintptr offset, GLsizeiptr size, GLenum access) => void*
  var OpenGLES2ChromiumMapSub_MapBufferSubDataCHROMIUM = function(context_uid, target, offset, size, access) {
    var _context = resources.resolve(context_uid, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    throw "OpenGLES2ChromiumMapSub_MapBufferSubDataCHROMIUM not implemented";
  };

  // (const void* mem) => void
  var OpenGLES2ChromiumMapSub_UnmapBufferSubDataCHROMIUM = function(context_uid, mem) {
    var _context = resources.resolve(context_uid, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    throw "OpenGLES2ChromiumMapSub_UnmapBufferSubDataCHROMIUM not implemented";
  };

  // (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLenum access) => void*
  var OpenGLES2ChromiumMapSub_MapTexSubImage2DCHROMIUM = function(context_uid, target, level, xoffset, yoffset, width, height, format, type, access) {
    var _context = resources.resolve(context_uid, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    throw "OpenGLES2ChromiumMapSub_MapTexSubImage2DCHROMIUM not implemented";
  };

  // (const void* mem) => void
  var OpenGLES2ChromiumMapSub_UnmapTexSubImage2DCHROMIUM = function(context_uid, mem) {
    var _context = resources.resolve(context_uid, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    throw "OpenGLES2ChromiumMapSub_UnmapTexSubImage2DCHROMIUM not implemented";
  };

  registerInterface("PPB_OpenGLES2ChromiumMapSub;1.0", [
    OpenGLES2ChromiumMapSub_MapBufferSubDataCHROMIUM,
    OpenGLES2ChromiumMapSub_UnmapBufferSubDataCHROMIUM,
    OpenGLES2ChromiumMapSub_MapTexSubImage2DCHROMIUM,
    OpenGLES2ChromiumMapSub_UnmapTexSubImage2DCHROMIUM,
  ]);



  // PPB_OpenGLES2Query

  // (GLsizei n, GLuint* queries) => void
  var OpenGLES2Query_GenQueriesEXT = function(context_uid, n, queries) {
    var _context = resources.resolve(context_uid, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    throw "OpenGLES2Query_GenQueriesEXT not implemented";
  };

  // (GLsizei n, const GLuint* queries) => void
  var OpenGLES2Query_DeleteQueriesEXT = function(context_uid, n, queries) {
    var _context = resources.resolve(context_uid, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    throw "OpenGLES2Query_DeleteQueriesEXT not implemented";
  };

  // (GLuint id) => GLboolean
  var OpenGLES2Query_IsQueryEXT = function(context_uid, id) {
    var _context = resources.resolve(context_uid, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    throw "OpenGLES2Query_IsQueryEXT not implemented";
  };

  // (GLenum target, GLuint id) => void
  var OpenGLES2Query_BeginQueryEXT = function(context_uid, target, id) {
    var _context = resources.resolve(context_uid, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    throw "OpenGLES2Query_BeginQueryEXT not implemented";
  };

  // (GLenum target) => void
  var OpenGLES2Query_EndQueryEXT = function(context_uid, target) {
    var _context = resources.resolve(context_uid, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    throw "OpenGLES2Query_EndQueryEXT not implemented";
  };

  // (GLenum target, GLenum pname, GLint* params) => void
  var OpenGLES2Query_GetQueryivEXT = function(context_uid, target, pname, params) {
    var _context = resources.resolve(context_uid, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    throw "OpenGLES2Query_GetQueryivEXT not implemented";
  };

  // (GLuint id, GLenum pname, GLuint* params) => void
  var OpenGLES2Query_GetQueryObjectuivEXT = function(context_uid, id, pname, params) {
    var _context = resources.resolve(context_uid, GRAPHICS_3D_RESOURCE);
    if (_context === undefined) {
      return;
    }
    throw "OpenGLES2Query_GetQueryObjectuivEXT not implemented";
  };

  registerInterface("PPB_OpenGLES2Query;1.0", [
    OpenGLES2Query_GenQueriesEXT,
    OpenGLES2Query_DeleteQueriesEXT,
    OpenGLES2Query_IsQueryEXT,
    OpenGLES2Query_BeginQueryEXT,
    OpenGLES2Query_EndQueryEXT,
    OpenGLES2Query_GetQueryivEXT,
    OpenGLES2Query_GetQueryObjectuivEXT,
  ]);

})();
