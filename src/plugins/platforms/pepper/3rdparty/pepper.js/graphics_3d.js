// Copyright (c) 2013 Google Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

  var getContext = function(c, params) {
    return c.getContext('webgl', params) || c.getContext("experimental-webgl", params);
  };

  var Graphics3D_GetAttribMaxValue = function(instance, attribute, value) {
    throw "Graphics3D_GetAttribMaxValue not implemented";
  };

  var Graphics3D_Create = function(instance, share_context, attrib_list) {
    var i = resources.resolve(instance, INSTANCE_RESOURCE);
    if (i === undefined) {
      return 0;
    }
    if (share_context !== 0) {
      throw "Graphics3D shared contexts not supported.";
    }

    var alpha_size = 0;
    var blue_size = 0;
    var green_size = 0;
    var red_size = 0;
    var depth_size = 0;
    var stencil_size = 0;
    var samples = 0;
    var sample_buffers = 0;
    var width = 0;
    var height = 0;
    var swap_behavior = ppapi.PP_GRAPHICS3DATTRIB_BUFFER_DESTROYED;
    var gpu_preference = ppapi.PP_GRAPHICS3DATTRIB_GPU_PREFERENCE_PERFORMANCE;

    if (attrib_list) {
      var ptr = attrib_list;
      while (true) {
        var name = getValue(ptr, 'i32');
        // name === 0 isn't part of the spec, but support it anyways.
        if (name === ppapi.PP_GRAPHICS3DATTRIB_NONE || name === 0) break;
        ptr += 4;
        var value = getValue(ptr, 'i32');
        ptr += 4;
        switch (name) {
        case ppapi.PP_GRAPHICS3DATTRIB_ALPHA_SIZE:
          alpha_size = value;
          break;
        case ppapi.PP_GRAPHICS3DATTRIB_BLUE_SIZE:
          blue_size = value;
          break;
        case ppapi.PP_GRAPHICS3DATTRIB_GREEN_SIZE:
          green_size = value;
          break;
        case ppapi.PP_GRAPHICS3DATTRIB_RED_SIZE:
          red_size = value;
          break;
        case ppapi.PP_GRAPHICS3DATTRIB_DEPTH_SIZE:
          depth_size = value;
          break;
        case ppapi.PP_GRAPHICS3DATTRIB_STENCIL_SIZE:
          stencil_size = value;
          break;
        case ppapi.PP_GRAPHICS3DATTRIB_SAMPLES:
          samples = value;
          break;
        case ppapi.PP_GRAPHICS3DATTRIB_SAMPLE_BUFFERS:
          sample_buffers = value;
          break;
        case ppapi.PP_GRAPHICS3DATTRIB_HEIGHT:
          height = value;
          break;
        case ppapi.PP_GRAPHICS3DATTRIB_WIDTH:
          width = value;
          break;
        case ppapi.PP_GRAPHICS3DATTRIB_SWAP_BEHAVIOR:
          swap_behavior = value;
          break;
        case ppapi.PP_GRAPHICS3DATTRIB_GPU_PREFERENCE_PERFORMANCE:
          gpu_preference = value;
        default:
          console.error("Unreconized GRAPHICS3DATTRIB " + name);
          return 0;
        }
      }
    }

    // Note: the canvas is not opaque.
    var canvas = i.createCanvas(width, height, false);
    canvas.style.width = "100%";
    canvas.style.height = "100%";

    return resources.register(GRAPHICS_3D_RESOURCE, {
      canvas: canvas,
      bound: false,
      ctx: getContext(canvas, {
        "alpha": alpha_size > 0,
        "depth": depth_size > 0,
        "stencil": stencil_size > 0,
        "antialias": sample_buffers !== 0 && samples > 1,
        "preserveDrawingBuffer": swap_behavior === ppapi.PP_GRAPHICS3DATTRIB_BUFFER_PRESERVED
      }),
      notifyBound: function(instance) {
        this.bound = true;
        instance.element.appendChild(this.canvas);
      },
      notifyUnbound: function(instance) {
        instance.element.removeChild(this.canvas);
        this.bound = false;
      }
    });
  };

  var Graphics3D_IsGraphics3D = function(resource) {
    return resources.is(resource, GRAPHICS_3D_RESOURCE);
  };

  var Graphics3D_GetAttribs = function(context, attrib_list) {
    throw "Graphics3D_GetAttribs not implemented";
  };

  var Graphics3D_SetAttribs = function(context, attrib_list) {
    throw "Graphics3D_SetAttribs not implemented";
  };

  var Graphics3D_GetError = function(context) {
    throw "Graphics3D_GetError not implemented";
  };

  var Graphics3D_ResizeBuffers = function(context, width, height) {
    var c = resources.resolve(context, GRAPHICS_3D_RESOURCE);
    if (c === undefined) {
      return ppapi.PP_ERROR_BADRESOURCE;
    }
    if (width < 0 || height < 0) {
      return ppapi.PP_ERROR_BADARGUMENT;
    }
    c.canvas.width = width;
    c.canvas.height = height;
    return ppapi.PP_OK;
  };

  var Graphics3D_SwapBuffers = function(context, callback) {
    // TODO double buffering.
    var c = glue.getCompletionCallback(callback);
    Module.requestAnimationFrame(function() {
      c(0);
    });
  };

  registerInterface("PPB_Graphics3D;1.0", [
    Graphics3D_GetAttribMaxValue,
    Graphics3D_Create,
    Graphics3D_IsGraphics3D,
    Graphics3D_GetAttribs,
    Graphics3D_SetAttribs,
    Graphics3D_GetError,
    Graphics3D_ResizeBuffers,
    Graphics3D_SwapBuffers,
  ], function() {
      return !!getContext(document.createElement("canvas"));
  });
})();
