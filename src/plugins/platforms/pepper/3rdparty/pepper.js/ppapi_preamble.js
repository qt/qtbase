// Copyright (c) 2013 Google Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(ncbray): re-enable once Emscripten stops including code with octal values.
//"use strict";

var clamp = function(value, min, max) {
  if (value < min) {
    return min;
  } else if (value > max) {
    return max;
  } else {
    return value;
  }
}

// Polyfill for Safari.
if (window.performance === undefined) {
  window.performance = {};
}
if (window.performance.now === undefined) {
  var nowStart = Date.now();
  window.performance.now = function() {
    return Date.now() - nowStart;
  }
}

// Polyfill for IE 10.
if (ArrayBuffer.prototype.slice === undefined) {
  // From https://developer.mozilla.org/en-US/docs/Web/API/ArrayBuffer:
  // Returns a new ArrayBuffer whose contents are a copy of this ArrayBuffer's
  // bytes from begin, inclusive, up to end, exclusive. If either begin or end
  // is negative, it refers to an index from the end of the array, as opposed
  // to from the beginning.
  ArrayBuffer.prototype.slice = function(begin, end) {
    if (begin < 0) {
      begin = this.byteLength + begin;
    }
    begin = clamp(begin, 0, this.byteLength);

    if (end === undefined) {
      end = this.byteLength;
    } else if (end < 0) {
      end = this.byteLength + end;
    }
    end = clamp(end, 0, this.byteLength);

    var length = end - begin;
    if (length < 0) {
      length = 0;
    }

    var src = new Int8Array(this, begin, length);
    var dst = new Int8Array(length);
    dst.set(src);
    return dst.buffer;
  };
}

var getFullscreenElement = function() {
  return document.fullscreenElement || document.webkitFullscreenElement || document.mozFullScreenElement || null;
}

// Encoding types as numbers instead of string saves ~2kB when minified because closure will inline these constants.
var STRING_RESOURCE = 0;
var ARRAY_BUFFER_RESOURCE = 1;

var INPUT_EVENT_RESOURCE = 2;

var FILE_SYSTEM_RESOURCE = 3;
var FILE_REF_RESOURCE = 4;
var FILE_IO_RESOURCE = 5;

var URL_LOADER_RESOURCE = 6;
var URL_REQUEST_INFO_RESOURCE = 7;
var URL_RESPONSE_INFO_RESOURCE = 8;

var AUDIO_CONFIG_RESOURCE = 9;
var AUDIO_RESOURCE = 10;

var GRAPHICS_2D_RESOURCE = 11;
var IMAGE_DATA_RESOURCE = 12;

var PROGRAM_RESOURCE = 13;
var SHADER_RESOURCE = 14;
var BUFFER_RESOURCE = 15;
var TEXTURE_RESOURCE = 16;
var UNIFORM_LOCATION_RESOURCE = 17;

var INSTANCE_RESOURCE = 18;
var VIEW_RESOURCE = 19;
var GRAPHICS_3D_RESOURCE = 20;

var ARRAY_RESOURCE = 21;
var DICTIONARY_RESOURCE = 22;
var WEB_SOCKET_RESOURCE = 23;

var ResourceManager = function() {
  this.lut = {};
  this.uid = 1;
  this.num_resources = 0;
}

ResourceManager.prototype.checkType = function(type) {
  if (typeof type !== "number") {
    throw "resource type must be a number";
  }
}

ResourceManager.prototype.register = function(type, res) {
  this.checkType(type);
  while (this.uid in this.lut || this.uid === 0) {
    this.uid = (this.uid + 1) & 0xffffffff;
  }
  res.type = type;
  res.uid = this.uid;
  res.refcount = 1;
  this.lut[res.uid] = res;
  this.num_resources += 1;
  this.dead = false;
  return this.uid;
}

ResourceManager.prototype.registerString = function(value, memory, len) {
  return this.register(STRING_RESOURCE, {
      value: value,
      memory: memory,
      len: len,
      destroy: function() {
        _free(this.memory);
      }
  });
}

ResourceManager.prototype.registerArray = function(value) {
  return this.register(ARRAY_RESOURCE, {
      value: value,
      setLength: function(length) {
        while(this.value.length > length) {
          glue.structRelease(this.value.pop());
        }
        while(this.value.length < length) {
          this.value.push({type: ppapi.PP_VARTYPE_UNDEFINED, value: 0});
        }
      },
      destroy: function() {
        var wrapped = this.value;
        this.value = [];
        for (var i = 0; i < wrapped.length; i++) {
          glue.structRelease(wrapped[i]);
        }
      }
  });
}

ResourceManager.prototype.registerDictionary = function(value) {
  return this.register(DICTIONARY_RESOURCE, {
      value: value,
      remove: function(key) {
        if (key in this.value) {
          var e = this.value[key];
          delete this.value[key];
          glue.structRelease(e);
        }
      },
      destroy: function() {
        var wrapped = this.value;
        this.value = {};
        for (var key in wrapped) {
          glue.structRelease(wrapped[key]);
        }
      }
  });
}

ResourceManager.prototype.registerArrayBuffer = function(memory, len) {
  return this.register(ARRAY_BUFFER_RESOURCE, {
      memory: memory,
      len: len,
      destroy: function() {
        _free(this.memory);
      }
  });
}

var uidInfo = function(uid, type) {
  return "(" + uid + " as " + type + ")";
}

ResourceManager.prototype.resolve = function(uid, type, speculative) {
  if (typeof uid !== "number") {
    throw "resources.resolve uid must be an int";
  }
  this.checkType(type);
  if (uid === 0) {
    if (!speculative) {
      console.error("Attempted to resolve an invalid resource ID " + uidInfo(uid, type));
    }
    return undefined;
  }
  var res = this.lut[uid];
  if (res === undefined) {
    console.error("Attempted to resolve a non-existant resource ID " + uidInfo(uid, type));
    return undefined;
  }
  if (res.type !== type) {
    if (!speculative) {
      console.error("Expected resource " + uidInfo(uid, type) + ", but it was " + uidInfo(uid, res.type));
    }
    return undefined;
  }
  return res;
}

ResourceManager.prototype.is = function(uid, type) {
  return this.resolve(uid, type, true) !== undefined;
}

ResourceManager.prototype.addRef = function(uid) {
  var res = this.lut[uid];
  if (res === undefined) {
    throw "Resource does not exist: " + uid;
  }
  res.refcount += 1;
}

ResourceManager.prototype.release = function(uid) {
  var res = this.lut[uid];
  if (res === undefined) {
    throw "Resource does not exist: " + uid;
  }

  res.refcount -= 1;
  if (res.refcount <= 0) {
    if (res.destroy) {
      res.destroy();
    }

    res.dead = true;
    delete this.lut[res.uid];
    this.num_resources -= 1;
  }
}

ResourceManager.prototype.getNumResources = function() {
  return this.num_resources;
}

ResourceManager.prototype.getResourceTypeHistogram = function() {
  var types = {};
  for (var uid in this.lut) {
    var t = this.lut[uid].type;
      types[t] = (types[t] || 0) + 1;
  }
  return types;
}

var resources = new ResourceManager();
var interfaces = {};
var declaredInterfaces = [];

var registerInterface = function(name, functions, supported) {
  // Defer creating the interface until Emscripten's runtime is available.
  declaredInterfaces.push({name: name, functions: functions, supported: supported});
};

var createInterface = function(name, functions) {
  var trace = function(f, i) {
    return function() {
      console.log(">>>", name, i, arguments);
      var result = f.apply(f, arguments);
      console.log("<<<", name, i, result);
      return result;
    }
  };

  var getFuncPtr = function(f) {
    // Memoize - a single function may appear in multiple versions of an interface.
    if (f.func_ptr === undefined) {
      f.func_ptr = Runtime.addFunction(f, 1);
    }
    return f.func_ptr;
  };

  // allocate(...) is bugged for non-i8 allocations, so do it manually
  // TODO(ncbray): static alloc?
  var ptr = allocate(functions.length * 4, 'i8', ALLOC_NORMAL);
  for (var i in functions) {
    var f = functions[i];
    if (false) {
      f = trace(f, i);
    }
    setValue(ptr + i * 4, getFuncPtr(f), 'i32');
  }
  interfaces[name] = ptr;
};

var Module = {
  "noInitialRun": true,
  "noExitRuntime": true,
  "preInit": function() {
    for (var i = 0; i < declaredInterfaces.length; i++) {
      var inf = declaredInterfaces[i];
      if (inf.supported === undefined || inf.supported()) {
        createInterface(inf.name, inf.functions);
      } else {
        interfaces[inf.name] = 0;
      }
    }
    declaredInterfaces = [];
  }
};

var CreateInstance = function(width, height, shadow_instance) {
  if (shadow_instance === undefined) {
    shadow_instance = document.createElement("span");
    shadow_instance.setAttribute("name", "nacl_module");
    shadow_instance.setAttribute("id", "nacl_module");
  }

  shadow_instance.setAttribute("width", width);
  shadow_instance.setAttribute("height", height);
  shadow_instance.className = "ppapiJsEmbed";

  shadow_instance.style.display = "inline-block";
  shadow_instance.style.width = width + "px";
  shadow_instance.style.height = height + "px";
  shadow_instance.style.overflow = "hidden";

  // Called from external code.
  shadow_instance["postMessage"] = function(message) {
    var instance = this.instance;
    // Fill out the PP_Var structure
    var var_ptr = _malloc(16);
    glue.jsToMemoryVar(message, var_ptr);

    // Post messages are resolved asynchronously.
    glue.defer(function() {
      _DoPostMessage(instance, var_ptr);
      // Note: the callee releases the var so we don't need to.
      // This is different than most interfaces.
      _free(var_ptr);
    });
  };

  // Not compatible with CSP.
  var style = document.createElement("style");
  style.type = "text/css";
  style.innerHTML = ".ppapiJsEmbed {border: 0px; margin: 0px; padding: 0px; outline: none;}";
  style.innerHTML += " .ppapiJsCanvas {outline: none; image-rendering: optimizeSpeed; image-rendering: -moz-crisp-edges; image-rendering: -o-crisp-edges; image-rendering: -webkit-optimize-contrast; image-rendering: optimize-contrast; -ms-interpolation-mode: nearest-neighbor;}";
  // Bug-ish.  Each variation needs to be specified seperately.
  // TODO(ncbray): set width and height to screen.width and screen.height?  This would better match PPAPI's behavior, but it could also cause unanticipated problems.
  var fullscreenCSS = "{position: fixed; top: 0; left: 0; bottom: 0; right: 0; width: 100% !important; height: 100% !important; box-sizing: border-box; object-fit: contain; background-color: black;}";
  style.innerHTML += " .ppapiJsEmbed:-webkit-full-screen " + fullscreenCSS;
  style.innerHTML += " .ppapiJsEmbed:-moz-full-screen " + fullscreenCSS;
  style.innerHTML += " .ppapiJsEmbed:-ms-fullscreen " + fullscreenCSS;
  style.innerHTML += " .ppapiJsEmbed:full-screen " + fullscreenCSS;

  document.getElementsByTagName("head")[0].appendChild(style);

  var last_update = "";
  var updateView = function() {
    // NOTE: this will give the wrong value if the canvas has any margin,
    // border, or padding.  Some browsers may also give non-integer values and
    // rounding is prefered to truncation.
    var bounds = shadow_instance.getBoundingClientRect();
    var rect = {
      point: {
        x: Math.round(bounds.left),
        y: Math.round(bounds.top)
      },
      size: {
        width: Math.round(bounds.right - bounds.left),
        height: Math.round(bounds.bottom - bounds.top)
      }
    };

    // Clip the bounds to the viewport.
    var clipX = clamp(bounds.left, 0, window.innerWidth);
    var clipY = clamp(bounds.top, 0, window.innerHeight);
    var clipWidth = clamp(bounds.right, 0, window.innerWidth) - clipX;
    var clipHeight = clamp(bounds.bottom, 0, window.innerHeight) - clipY;

    // Translate into the coordinate space of the canvas.
    clipX -= bounds.left;
    clipY -= bounds.top;

    // Handle a zero-sized clip region.
    var visible = clipWidth > 0 && clipHeight > 0;
    if (!visible) {
      // clipX and clipY may be outside the canvas if width or height are zero.
      // The PPAPI spec requires we return (0, 0, 0, 0)
      clipX = 0;
      clipY = 0;
      clipWidth = 0;
      clipHeight = 0;
    }
    var event = {
      rect: rect,
      fullscreen: getFullscreenElement() === shadow_instance,
      visible: visible,
      page_visible: 1,
      clip_rect: {
        point: {
          x: clipX,
          y: clipY
        },
        size: {
          width: clipWidth,
          height: clipHeight
        }
      }
    };
    var s = JSON.stringify(event);
    if (s !== last_update) {
      last_update = s;
      var view = resources.register(VIEW_RESOURCE, event);
      _DoChangeView(instance, view);
      resources.release(view);
    }
  };

  var makeFocusCallback = function(hasFocus){
    return function(event) {
      _DoChangeFocus(shadow_instance.instance, hasFocus);
      return true;
    };
  };

  var instance = resources.register(INSTANCE_RESOURCE, {
    element: shadow_instance,
    device: null,
    createCanvas: function(width, height, opaque) {
      var canvas = document.createElement('canvas');
      canvas.className = "ppapiJsCanvas";
      canvas.width = width;
      canvas.height = height;
      canvas.style.border = "0px";
      canvas.style.padding = "0px";
      canvas.style.margin = "0px";
      canvas.style.backgroundColor = opaque ? "black" : "transparent";
      return canvas;
    },
    bind: function(device) {
      this.unbind();
      this.device = device;
      resources.addRef(this.device.uid);
      this.device.notifyBound(this);
    },
    unbind: function() {
      if (this.device) {
        this.device.notifyUnbound(this);
        resources.release(this.device.uid);
        this.device = null;
      }
    },
    destroy: function() {
      this.unbind();
    }
  });

  // Allows shadow_instance.postMessage to work.
  // This is only a UID so there is no circular reference.
  shadow_instance.instance = instance;

  shadow_instance.onselectstart = function(evt) {
      evt.preventDefault();
      return false;
  };
  shadow_instance.setAttribute('tabindex', '0'); // make it focusable
  shadow_instance.addEventListener('focus', makeFocusCallback(true));
  shadow_instance.addEventListener('blur', makeFocusCallback(false));

  // Called from external code.
  shadow_instance["finishLoading"] = function() {
    // Turn the element's attributes into PPAPI's arguments.
    // TODO(ncbray): filter out style attribute?
    var argc = shadow_instance.attributes.length;
    var argn = allocate(argc * 4, 'i8', ALLOC_NORMAL);
    var argv = allocate(argc * 4, 'i8', ALLOC_NORMAL);
    for (var i = 0; i < argc; i++) {
      var attribute = shadow_instance.attributes[i];
      setValue(argn + i * 4, allocate(intArrayFromString(attribute.name), 'i8', ALLOC_NORMAL), 'i32');
      setValue(argv + i * 4, allocate(intArrayFromString(attribute.value), 'i8', ALLOC_NORMAL), 'i32');
    }

    _NativeCreateInstance(instance, argc, argn, argv);

    // Clean up the arguments.
    for (var i = 0; i < argc; i++) {
      _free(getValue(argn + i * 4, 'i32'));
      _free(getValue(argv + i * 4, 'i32'));
    }
    _free(argn);
    _free(argv);

    updateView();

    var sendProgressEvent = function(name) {
      var evt = document.createEvent('Event');
      evt.initEvent(name, true, true);  // bubbles, cancelable
      shadow_instance.dispatchEvent(evt);
    }

    // Fake the load sequence.
    // Note that .readyState will always be "complete" in IE, and
    // this assignment will do nothing.
    shadow_instance.readyState = 4;
    sendProgressEvent('load');
    sendProgressEvent('loadend');
  };

  window.addEventListener('DOMContentLoaded', updateView);
  window.addEventListener('load', updateView);
  window.addEventListener('scroll', updateView);
  window.addEventListener('resize', updateView);

  // TODO(ncbray): element resize.

  var target = shadow_instance;
  if (target.requestFullscreen) {
    document.addEventListener('fullscreenchange', updateView);
  } else if (target.mozRequestFullScreen) {
    document.addEventListener('mozfullscreenchange', updateView);
  } else if (target.webkitRequestFullscreen) {
    document.addEventListener('webkitfullscreenchange', updateView);
  } else if (target.msRequestFullscreen) {
    document.addEventListener('MSFullscreenChange', updateView);
  }

  // TODO handle removal events.
  return shadow_instance;
}

// Entry point
window["CreateInstance"] = CreateInstance;

var glue = {};

glue.k2_32 = 0x100000000;

glue.ToI64 = function(low, high){
  var val = low + (high * glue.k2_32);
  if (((val - low) / glue.k2_32) !== high || (val % glue.k2_32) !== low) {
    throw "Value " + String([low, high]) + " cannot be represented as a Javascript number";
  }
  return val;
};

glue.decodeUTF8 = function(ptr, len) {
  var chars = [];
  var i = 0;
  var val;
  var n;
  var b;

  // If no len is provided, assume null termination
  while (len === undefined || i < len) {
    b = HEAPU8[ptr + i];
    if (len === undefined && b === 0) {
      break;
    }

    i += 1;
    if (b < 0x80) {
      val = b;
      n = 0;
    } else if ((b & 0xE0) === 0xC0) {
      val = b & 0x1f;
      n = 1;
    } else if ((b & 0xF0) === 0xE0) {
      val = b & 0x0f;
      n = 2;
    } else if ((b & 0xF8) === 0xF0) {
      val = b & 0x07;
      n = 3;
    } else if ((b & 0xFC) === 0xF8) {
      val = b & 0x03;
      n = 4;
    } else if ((b & 0xFE) === 0xFC) {
      val = b & 0x01;
      n = 5;
    } else {
      return null;
    }
    if (i + n > len) {
      return null;
    }
    while (n > 0) {
      b = HEAPU8[ptr + i];
      if ((b & 0xC0) !== 0x80) {
        return null;
      }
      val = (val << 6) | (b & 0x3f);
      i += 1;
      n -= 1;
    }
    chars.push(String.fromCharCode(val));
  }
  return chars.join("");
};

glue.getPoint = function(ptr) {
  return {
    x: getValue(ptr, 'i32'),
    y: getValue(ptr + 4, 'i32')
  };
};

glue.setPoint = function(obj, ptr) {
  setValue(ptr, obj.x, 'i32');
  setValue(ptr + 4, obj.y, 'i32');
};

// No need for getFloatPoint, yet.

glue.setFloatPoint = function(obj, ptr) {
  setValue(ptr, obj.x, 'float');
  setValue(ptr + 4, obj.y, 'float');
};

glue.getSize = function(ptr) {
  return {
    width: getValue(ptr, 'i32'),
    height: getValue(ptr + 4, 'i32')
  };
};

glue.setSize = function(obj, ptr) {
  setValue(ptr, obj.width, 'i32');
  setValue(ptr + 4, obj.height, 'i32');
};

glue.getRect = function(ptr) {
  return {
    point: glue.getPoint(ptr),
    size: glue.getSize(ptr + 8)
  };
};

glue.setRect = function(rect, ptr) {
  glue.setPoint(rect.point, ptr);
  glue.setSize(rect.size, ptr + 8);
};

glue.getVarType = function(ptr) {
  // ptr->type is offset 0.
  return getValue(ptr, 'i32');
};

glue.isRefCountedVarType = function(type) {
  return type >= 5;
};

glue.getVarUID = function(ptr) {
  // ptr->value is offset 8.
  return getValue(ptr + 8, 'i32');
};

glue.memoryToStructVar = function(ptr) {
  var type = glue.getVarType(ptr);
  var value;
  if (type == ppapi.PP_VARTYPE_DOUBLE) {
    value = getValue(ptr + 8, 'double');
  } else {
    value = getValue(ptr + 8, 'i32');
  }
  return {type: type, value: value};
};

glue.structAddRef = function(e) {
  if (glue.isRefCountedVarType(e.type)) {
    resources.addRef(e.value);
  }
};

glue.structRelease = function(e) {
  if (glue.isRefCountedVarType(e.type)) {
    resources.release(e.value);
  }
};

glue.memoryToJSVar = function(ptr) {
  return glue.structToJSVar(glue.memoryToStructVar(ptr));
};

glue.structToJSVar = function(e) {
  var type = e.type;
  var value = e.value;
  if (type == ppapi.PP_VARTYPE_UNDEFINED) {
    return undefined;
  } else if (type == ppapi.PP_VARTYPE_NULL) {
    return null;
  } else if (type == ppapi.PP_VARTYPE_BOOL) {
    return 0 != value;
  } else if (type == ppapi.PP_VARTYPE_INT32) {
    return value | 0;
  } else if (type == ppapi.PP_VARTYPE_DOUBLE) {
    return value;
  } else if (type == ppapi.PP_VARTYPE_STRING) {
    return resources.resolve(value, STRING_RESOURCE).value;
  } else if (type == ppapi.PP_VARTYPE_ARRAY) {
    // TODO(ncbray): deduplicate DAG nodes.
    var wrapped = resources.resolve(value, ARRAY_RESOURCE).value;
    var unwrapped = [];
    for (var i = 0; i < wrapped.length; i++) {
      unwrapped.push(glue.structToJSVar(wrapped[i]));
    }
    return unwrapped;
  } else if (type == ppapi.PP_VARTYPE_DICTIONARY) {
    // TODO(ncbray): deduplicate DAG nodes.
    var wrapped = resources.resolve(value, DICTIONARY_RESOURCE).value;
    var unwrapped = {};
    for (var key in wrapped) {
      unwrapped[key] = glue.structToJSVar(wrapped[key]);
    }
    return unwrapped;
  } else if (type == ppapi.PP_VARTYPE_ARRAY_BUFFER) {
    var wrapped = resources.resolve(value, ARRAY_BUFFER_RESOURCE);

    // Note: "buffer" is an implementation detail of Emscripten and is likely
    // not a stable interface.
    var unwrapped = buffer.slice(wrapped.memory, wrapped.memory + wrapped.len);
    return unwrapped;
  } else {
    throw "Var type conversion not implemented: " + type;
  }
};

glue.jsToStructVar = function(obj) {
  var type = 0;
  var value = 0;

  var typen = (typeof obj);
  if (typen === 'string') {
    type = ppapi.PP_VARTYPE_STRING;
    var arr = intArrayFromString(obj);
    var memory = allocate(arr, 'i8', ALLOC_NORMAL);
    // Length is adjusted for null terminator.
    value = resources.registerString(obj, memory, arr.length-1);
  } else if (typen === 'number') {
    // Note this will always create a double, even when the value can be
    // represented as an int32.
    type = ppapi.PP_VARTYPE_DOUBLE;
    value = obj;
  } else if (typen === 'boolean') {
    type = ppapi.PP_VARTYPE_BOOL;
    value = obj ? 1 : 0;
  } else if (typen === 'undefined') {
    type = ppapi.PP_VARTYPE_UNDEFINED;
  } else if (obj === null) {
    type = ppapi.PP_VARTYPE_NULL;
  } else if (obj instanceof Array) {
    // TODO(ncbray): deduplicate DAG nodes.
    type = ppapi.PP_VARTYPE_ARRAY;
    var wrapped = [];
    value = resources.registerArray(wrapped);
    for (var i = 0; i < obj.length; i++) {
      wrapped.push(glue.jsToStructVar(obj[i]));
    }
  } else if (obj instanceof ArrayBuffer) {
    type = ppapi.PP_VARTYPE_ARRAY_BUFFER;
    var memory = _malloc(obj.byteLength);
    // Note: "buffer" is an implementation detail of Emscripten and is likely
    // not a stable interface.
    var memory_view = new Int8Array(buffer, memory, obj.byteLength);
    memory_view.set(new Int8Array(obj));
    value = resources.registerArrayBuffer(memory, obj.byteLength);
  } else if (typen === 'object') {
    // Note that this need to go last because many things are "objects", such as
    // ArrayBuffers.  Is there a good way to distinguish between plain-old
    // objects and other types?

    // TODO(ncbray): deduplicate DAG nodes.
    type = ppapi.PP_VARTYPE_DICTIONARY;
    var wrapped = {};
    value = resources.registerDictionary(wrapped);
    for (var key in obj) {
      wrapped[key] = glue.jsToStructVar(obj[key]);
    }
  } else {
    throw "Var type conversion not implemented: " + typen;
  }
  return {type: type, value: value};
};

glue.structToMemoryVar = function(e, ptr) {
  setValue(ptr, e.type, 'i32');
  if (e.type === ppapi.PP_VARTYPE_DOUBLE) {
    setValue(ptr + 8, e.value, 'double');
  } else {
    // Note: PPAPI defines var IDs as 64-bit values, but in practice we're only
    // using 32 bits, so we can handle UIDs in this catch all.
    setValue(ptr + 8, e.value, 'i32');
    setValue(ptr + 12, 0, 'i32'); // Paranoia.
  }
};

glue.jsToMemoryVar = function(obj, ptr) {
  glue.structToMemoryVar(glue.jsToStructVar(obj), ptr);
};

glue.setIntVar = function(obj, ptr) {
  var typen = (typeof obj);
  var valptr = ptr + 8;
  if (typen === 'number') {
    setValue(ptr, ppapi.PP_VARTYPE_INT32, 'i32');
    setValue(valptr, obj, 'i32');
  } else {
    throw "Not an integer: " + typen;
  }
};

glue.getCompletionCallback = function(ptr) {
  var func = getValue(ptr, 'i32');
  var user_data = getValue(ptr + 4, 'i32');
  return function(result) {
    if (typeof result !== 'number') {
      throw "Invalid argument to callback: " + result;
    }
    Runtime.dynCall('vii', func, [user_data, result]);
  };
};

glue.defer = function(callback) {
  setTimeout(callback, 0);
};


var ppapi = (function() {
  var ppapi = {
    /**
     * This value is returned by a function on successful synchronous completion
     * or is passed as a result to a PP_CompletionCallback_Func on successful
     * asynchronous completion.
     */
    PP_OK: 0,
    /**
     * This value is returned by a function that accepts a PP_CompletionCallback
     * and cannot complete synchronously. This code indicates that the given
     * callback will be asynchronously notified of the final result once it is
     * available.
     */
    PP_OK_COMPLETIONPENDING: -1,
    /**This value indicates failure for unspecified reasons. */
    PP_ERROR_FAILED: -2,
    /**
     * This value indicates failure due to an asynchronous operation being
     * interrupted. The most common cause of this error code is destroying a
     * resource that still has a callback pending. All callbacks are guaranteed
     * to execute, so any callbacks pending on a destroyed resource will be
     * issued with PP_ERROR_ABORTED.
     *
     * If you get an aborted notification that you aren't expecting, check to
     * make sure that the resource you're using is still in scope. A common
     * mistake is to create a resource on the stack, which will destroy the
     * resource as soon as the function returns.
     */
    PP_ERROR_ABORTED: -3,
    /** This value indicates failure due to an invalid argument. */
    PP_ERROR_BADARGUMENT: -4,
    /** This value indicates failure due to an invalid PP_Resource. */
    PP_ERROR_BADRESOURCE: -5,
    /** This value indicates failure due to an unavailable PPAPI interface. */
    PP_ERROR_NOINTERFACE: -6,
    /** This value indicates failure due to insufficient privileges. */
    PP_ERROR_NOACCESS: -7,
    /** This value indicates failure due to insufficient memory. */
    PP_ERROR_NOMEMORY: -8,
    /** This value indicates failure due to insufficient storage space. */
    PP_ERROR_NOSPACE: -9,
    /** This value indicates failure due to insufficient storage quota. */
    PP_ERROR_NOQUOTA: -10,
    /**
     * This value indicates failure due to an action already being in
     * progress.
     */
    PP_ERROR_INPROGRESS: -11,
    /** This value indicates failure due to a file that does not exist. */
    /**
     * The requested command is not supported by the browser.
     */
    PP_ERROR_NOTSUPPORTED: -12,
    /**
     * Returned if you try to use a null completion callback to "block until
     * complete" on the main thread. Blocking the main thread is not permitted
     * to keep the browser responsive (otherwise, you may not be able to handle
     * input events, and there are reentrancy and deadlock issues).
     *
     * The goal is to provide blocking calls from background threads, but PPAPI
     * calls on background threads are not currently supported. Until this
     * support is complete, you must either do asynchronous operations on the
     * main thread, or provide an adaptor for a blocking background thread to
     * execute the operaitions on the main thread.
     */
    PP_ERROR_BLOCKS_MAIN_THREAD: -13,
    PP_ERROR_FILENOTFOUND: -20,
    /** This value indicates failure due to a file that already exists. */
    PP_ERROR_FILEEXISTS: -21,
    /** This value indicates failure due to a file that is too big. */
    PP_ERROR_FILETOOBIG: -22,
    /**
     * This value indicates failure due to a file having been modified
     * unexpectedly.
     */
    PP_ERROR_FILECHANGED: -23,
    /** This value indicates failure due to a time limit being exceeded. */
    PP_ERROR_TIMEDOUT: -30,
    /**
     * This value indicates that the user cancelled rather than providing
     * expected input.
     */
    PP_ERROR_USERCANCEL: -40,
    /**
     * This value indicates failure due to lack of a user gesture such as a
     * mouse click or key input event. Examples of actions requiring a user
     * gesture are showing the file chooser dialog and going into fullscreen
     * mode.
     */
    PP_ERROR_NO_USER_GESTURE: -41,
    /**
     * This value indicates that the graphics context was lost due to a
     * power management event.
     */
    PP_ERROR_CONTEXT_LOST: -50,
    /**
     * Indicates an attempt to make a PPAPI call on a thread without previously
     * registering a message loop via PPB_MessageLoop.AttachToCurrentThread.
     * Without this registration step, no PPAPI calls are supported.
     */
    PP_ERROR_NO_MESSAGE_LOOP: -51,
    /**
     * Indicates that the requested operation is not permitted on the current
     * thread.
     */
    PP_ERROR_WRONG_THREAD: -52,

    PP_GRAPHICS3DATTRIB_ALPHA_SIZE: 0x3021,
    PP_GRAPHICS3DATTRIB_BLUE_SIZE: 0x3022,
    PP_GRAPHICS3DATTRIB_GREEN_SIZE: 0x3023,
    PP_GRAPHICS3DATTRIB_RED_SIZE: 0x3024,
    PP_GRAPHICS3DATTRIB_DEPTH_SIZE: 0x3025,
    PP_GRAPHICS3DATTRIB_STENCIL_SIZE: 0x3026,
    PP_GRAPHICS3DATTRIB_SAMPLES: 0x3031,
    PP_GRAPHICS3DATTRIB_SAMPLE_BUFFERS: 0x3032,
    PP_GRAPHICS3DATTRIB_NONE: 0x3038,
    PP_GRAPHICS3DATTRIB_HEIGHT: 0x3056,
    PP_GRAPHICS3DATTRIB_WIDTH: 0x3057,
    PP_GRAPHICS3DATTRIB_SWAP_BEHAVIOR: 0x3093,
    PP_GRAPHICS3DATTRIB_BUFFER_PRESERVED: 0x3094,
    PP_GRAPHICS3DATTRIB_BUFFER_DESTROYED: 0x3095,
    PP_GRAPHICS3DATTRIB_GPU_PREFERENCE: 0x11000,
    PP_GRAPHICS3DATTRIB_GPU_PREFERENCE_LOW_POWER: 0x11001,
    PP_GRAPHICS3DATTRIB_GPU_PREFERENCE_PERFORMANCE: 0x11002,

    PP_VARTYPE_UNDEFINED: 0,
    PP_VARTYPE_NULL: 1,
    PP_VARTYPE_BOOL: 2,
    PP_VARTYPE_INT32: 3,
    PP_VARTYPE_DOUBLE: 4,
    PP_VARTYPE_STRING: 5,
    PP_VARTYPE_ARRAY: 7,
    PP_VARTYPE_DICTIONARY: 8,
    PP_VARTYPE_ARRAY_BUFFER: 9
  };

  return ppapi;
})();


// Called from generated code.
var _GetBrowserInterface = function(interface_name) {
  var name = Pointer_stringify(interface_name);
  if (!(name in interfaces)) {
    console.error('Requested unknown interface: ' + name);
    return 0;
  }
  var inf = interfaces[name]|0;
  if (inf === 0) {
    // The interface exists, but it is not available for this particular browser.
    console.error('Requested unavailable interface: ' + name);
  }
  return inf;
};
