// Copyright (c) 2013 Google Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

  var Graphics2D_Create = function(instance, size_ptr, is_always_opaque) {
    var size = glue.getSize(size_ptr);
    var i = resources.resolve(instance, INSTANCE_RESOURCE);
    if (i === undefined) {
      return 0;
    }

    return resources.register(GRAPHICS_2D_RESOURCE, {
      size: size,
      canvas: null,
      bound: false,
      ctx_: null,
      always_opaque: is_always_opaque,
      scale: 1,
      flushCallback: null,
      // Lazy allocate to reduce memory presure when creating a new context.
      lazyInit: function() {
        if (this.canvas === null) {
          // TODO(ncbray): move off instance object.
          this.canvas = i.createCanvas(0, 0, this.always_opaque);
          this.setScale(this.scale);
          this.ctx_ = this.canvas.getContext('2d');
          this.ctx_.imageSmoothingEnabled = false;
          this.ctx_.webkitImageSmoothingEnabled = false;
          this.ctx_.mozImageSmoothingEnabled = false;
          // Undo the "helpful" effects of backingStorePixelRatio.
          this.backingStoreRatio =
              this.ctx_.webkitBackingStorePixelRatio ||
              this.ctx_.mozBackingStorePixelRatio ||
              this.ctx_.msBackingStorePixelRatio ||
              this.ctx_.oBackingStorePixelRatio ||
              this.ctx_.backingStorePixelRatio ||
              1;
          this.canvas.width = this.size.width / this.backingStoreRatio;
          this.canvas.height = this.size.height / this.backingStoreRatio;
        }
      },
      putImageData: function() {
        if (this.ctx_ === null) {
          this.lazyInit();
        }
        // Because Safari.
        var f = this.ctx_.webkitPutImageDataHD || this.ctx_.putImageData;
        f.apply(this.ctx_, arguments);
      },
      getImageData: function() {
        if (this.ctx_ === null) {
          this.lazyInit();
        }
        // Because Safari.
        var f = this.ctx_.webkitGetImageDataHD || this.ctx_.getImageData;
        return f.apply(this.ctx_, arguments);
      },
      setScale: function(scale) {
        this.scale = scale;
        if (this.canvas) {
          this.canvas.style.width = (this.size.width * scale) + "px";
          this.canvas.style.height = (this.size.height * scale) + "px";
        }
      },
      notifyBound: function(instance) {
        this.bound = true;
        this.lazyInit();
        instance.element.appendChild(this.canvas);
      },
      notifyUnbound: function(instance) {
        // TODO convert pending callbacks.
        instance.element.removeChild(this.canvas);
        this.bound = false;
      },
      destroy: function() {
        this.flushCallback = null;
      }
    });
  };

  var Graphics2D_IsGraphics2D = function(resource) {
    return resources.is(resource, GRAPHICS_2D_RESOURCE);
  };

  var Graphics2D_Describe = function(resource, size_ptr, is_always_opaque_ptr) {
    var g2d = resources.resolve(resource, GRAPHICS_2D_RESOURCE);
    if (g2d === undefined) {
      setValue(size_ptr, 0, 'i32');
      setValue(size_ptr + 4, 0, 'i32');
      setValue(is_always_opaque_ptr, 0, 'i32');
      return 0;
    }
    setValue(size_ptr, g2d.size.width, 'i32');
    setValue(size_ptr + 4, g2d.size.height, 'i32');
    setValue(is_always_opaque_ptr, g2d.always_opaque, 'i32');
    return 1;
  };

  var Graphics2D_PaintImageData = function(resource, image_data, top_left_ptr, src_rect_ptr) {
    var g2d = resources.resolve(resource, GRAPHICS_2D_RESOURCE);
    // Eat any errors that occur, same as the implementation in Chrome.
    if (g2d === undefined) {
      return;
    }
    var res = resources.resolve(image_data, IMAGE_DATA_RESOURCE);
    if (res === undefined) {
      return;
    }
    // Note: PPAPI in Chrome does not draw the data if it is partially outside the bounds of the context.  This implementation does.
    // TODO(ncbray): only sync the portion being drawn?
    syncImageData(res);
    var top_left = glue.getPoint(top_left_ptr);
    if (src_rect_ptr == 0) {
      g2d.putImageData(res.image_data, top_left.x, top_left.y);
    } else {
      var src_rect = glue.getRect(src_rect_ptr);
      g2d.putImageData(res.image_data, top_left.x, top_left.y, src_rect.point.x, src_rect.point.y, src_rect.size.width, src_rect.size.height);
    }
  };

  var Graphics2D_Scroll = function(resource, clip_rect_ptr, amount_ptr) {
    var g2d = resources.resolve(resource, GRAPHICS_2D_RESOURCE);
    // Eat any errors that occur, same as the implementation in Chrome.
    if (g2d === undefined) {
      return;
    }
    var clip_rect = glue.getRect(clip_rect_ptr);
    var amount = glue.getPoint(amount_ptr);
    var x = clip_rect.point.x;
    var y = clip_rect.point.y;
    var w =  clip_rect.size.width;
    var h =  clip_rect.size.height;
    var dx = amount.x;
    var dy = amount.y;

    // Clip the shifted image by shifting and clipping the source.
    if (dx < 0) {
      x -= dx;
      w += dx;
    } else {
      w -= dx;
    }
    if (dy < 0) {
      y -= dy;
      h += dy;
    } else {
      h -= dy;
    }

    // Shifting everything out of the clip rect results in a no-op.
    if (w <= 0 || h <= 0) {
      return;
    }

    var data = g2d.getImageData(x, y, w, h);
    g2d.putImageData(data, x + dx, y + dy);
  };

  var Graphics2D_ReplaceContents = function(resource, image_data) {
    var g2d = resources.resolve(resource, GRAPHICS_2D_RESOURCE);
    // Eat any errors that occur, same as the implementation in Chrome.
    if (g2d === undefined) {
      return;
    }
    var res = resources.resolve(image_data, IMAGE_DATA_RESOURCE);
    if (res === undefined) {
      return;
    }
    syncImageData(res);
    g2d.putImageData(res.image_data, 0, 0);
  };

  var Graphics2D_Flush = function(resource, callback) {
    var g2d = resources.resolve(resource, GRAPHICS_2D_RESOURCE);
    if (g2d === undefined) {
      return ppapi.PP_ERROR_BADRESOURCE;
    }
    if (g2d.flushCallback !== null) {
      return ppapi.PP_ERROR_INPROGRESS;
    }
    var flushCallback = {
      callback: glue.getCompletionCallback(callback),
      trigger: function() {
        if (g2d.flushCallback === this) {
          g2d.flushCallback = null;
          this.callback(ppapi.PP_OK);
        }
      },
      cancel: function() {
        if (g2d.flushCallback === this) {
          g2d.flushCallback = null;
        }
      }
    };
    g2d.flushCallback = flushCallback;
    Module.requestAnimationFrame(function() {
      flushCallback.trigger();
    });
    return ppapi.PP_OK_COMPLETIONPENDING;
  };

  var Graphics2D_SetScale = function(resource, scale) {
    var g2d = resources.resolve(resource, GRAPHICS_2D_RESOURCE);
    if (g2d === undefined) {
      return 0;
    }
    if (scale <= 0) {
      return 0;
    }
    g2d.setScale(scale);
    return 1;
  };

  var Graphics2D_GetScale = function(resource) {
    var g2d = resources.resolve(resource, GRAPHICS_2D_RESOURCE);
    if (g2d === undefined) {
      return 0;
    }
    return g2d.scale;
  };


  registerInterface("PPB_Graphics2D;1.0", [
    Graphics2D_Create,
    Graphics2D_IsGraphics2D,
    Graphics2D_Describe,
    Graphics2D_PaintImageData,
    Graphics2D_Scroll,
    Graphics2D_ReplaceContents,
    Graphics2D_Flush,
  ]);

  registerInterface("PPB_Graphics2D;1.1", [
    Graphics2D_Create,
    Graphics2D_IsGraphics2D,
    Graphics2D_Describe,
    Graphics2D_PaintImageData,
    Graphics2D_Scroll,
    Graphics2D_ReplaceContents,
    Graphics2D_Flush,
    Graphics2D_SetScale,
    Graphics2D_GetScale,
  ]);


  // Copy the data from Emscripten's memory space into the ImageData object.
  var syncImageData = function(res) {
    if (res.view !== null) {
      res.image_data.data.set(res.view);
    } else {
      var image_data = res.image_data;
      var base = res.memory;
      var bytes = res.size.width * res.size.height * 4;

      if (res.format === 0) {
        // BGRA
        for (var i = 0; i < bytes; i += 4) {
          image_data.data[i]     = HEAPU8[base + i + 2];
          image_data.data[i + 1] = HEAPU8[base + i + 1];
          image_data.data[i + 2] = HEAPU8[base + i];
          image_data.data[i + 3] = HEAPU8[base + i + 3];
        }
      } else {
        // RGBA
        for (var i = 0; i < bytes; i += 4) {
          image_data.data[i]     = HEAPU8[base + i];
          image_data.data[i + 1] = HEAPU8[base + i + 1];
          image_data.data[i + 2] = HEAPU8[base + i + 2];
          image_data.data[i + 3] = HEAPU8[base + i + 3];
        }
      }
    }
  }


  var ImageData_GetNativeImageDataFormat = function() {
    // PP_IMAGEDATAFORMAT_RGBA_PREMUL
    return 1;
  };

  // We only support RGBA.
  // To simplify porting we're pretending that we also support BGRA and really giving an RGBA buffer instead.
  var ImageData_IsImageDataFormatSupported = function(format) {
    return format == 0 || format == 1;
  };

  var ImageData_Create = function(instance, format, size_ptr, init_to_zero) {
    if (!ImageData_IsImageDataFormatSupported(format)) {
      return 0;
    }
    var size = glue.getSize(size_ptr);
    if (size.width <= 0 || size.height <= 0) {
      return 0;
    }

    // HACK for creating an image data without having a 2D context available.
    var c = document.createElement('canvas');
    var ctx = c.getContext('2d');
    var image_data;
    try {
      // Due to limitations of the canvas API, we need to create an intermediate "ImageData" buffer.
      image_data = ctx.createImageData(size.width, size.height);
    } catch(err) {
      // Calls in the try block may return range errors if the sizes are too big.
      return 0;
    }

    var bytes = size.width * size.height * 4;
    var memory = _malloc(bytes);
    if (memory === 0) {
      return 0;
    }
    if (init_to_zero) {
      _memset(memory, 0, bytes);
    }

    var view = null;
    // Direct copies are only supported if:
    // 1) The image format is RGBA
    // 2) The canvas API implements image_data.data as a typed array.
    // 3) Uint8ClampedArray is defined (this is likely redundant with condition 2)
    // Note that Closure appears to minimize window.Uint8ClampedArray.
    var fast_path_supported = format === 1 && "set" in image_data.data && window["Uint8ClampedArray"] !== undefined;
    if (fast_path_supported) {
      try {
        // Note: "buffer" is an implementation detail of Emscripten and is likely not a stable interface.
        view = new Uint8ClampedArray(buffer, memory, bytes);
      } catch(err) {
        _free(memory);
        return 0;
      }
    }

    var uid = resources.register(IMAGE_DATA_RESOURCE, {
      format: format,
      size: size,
      memory: memory,
      view: view,
      image_data: image_data,
      destroy: function() {
        _free(memory);
      }
    });
    return uid;
  };

  var ImageData_IsImageData = function (image_data) {
    return resources.is(image_data, IMAGE_DATA_RESOURCE);
  };

  var ImageData_Describe = function(image_data, desc_ptr) {
    var res = resources.resolve(image_data, IMAGE_DATA_RESOURCE);
    if (res !== undefined) {
      setValue(desc_ptr + 0, res.format, 'i32');
      setValue(desc_ptr + 4, res.size.width, 'i32');
      setValue(desc_ptr + 8, res.size.height, 'i32');
      setValue(desc_ptr + 12, res.size.width*4, 'i32');
      return 1;
    } else {
      _memset(desc_ptr, 0, 16);
      return 0;
    }
  };

  var ImageData_Map = function(image_data) {
    var res = resources.resolve(image_data, IMAGE_DATA_RESOURCE);
    if (res === undefined) {
      return 0;
    }
    return res.memory;
  };

  var ImageData_Unmap = function(image_data) {
    // Ignore
  };

  registerInterface("PPB_ImageData;1.0", [
    ImageData_GetNativeImageDataFormat,
    ImageData_IsImageDataFormatSupported,
    ImageData_Create,
    ImageData_IsImageData,
    ImageData_Describe,
    ImageData_Map,
    ImageData_Unmap
  ]);
})();
