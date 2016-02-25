// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Javascript module pattern:
//   see http://en.wikipedia.org/wiki/Unobtrusive_JavaScript#Namespaces
// In essence, we define an anonymous function which is immediately called and
// returns a new object. The new object contains only the exported definitions;
// all other definitions in the anonymous function are inaccessible to external
// code.
var common = (function () {

  var addListener = function(elt, event_name, callback) {
    if (elt.addEventListener) {
      elt.addEventListener(event_name, callback, false);
    } else {
      elt.attachEvent('on' + event_name, callback);
    }
  };

  var getImageDataBuffer = function(imageData) {
    var buffer = imageData.data.buffer;
    // IE support
    if(buffer === undefined) {
      buffer = new ArrayBuffer(imageData.data.length);
      view = new Uint8Array(buffer);
      for (var i = 0; i < imageData.data.length; i++) {
        view[i] = imageData.data[i];
      }
    }
    return buffer;
  }

  nacl.createInstance = function(config) {
    var variant = config.module;
    var e;
    var width = config.width;
    var height = config.height;
    var type = variant.type;
    if (type == "pnacl") {
      e = nacl.createEmbedInstance(variant.url, nacl.pnaclMimeType, width, height);
    } else if (type == "emscripten") {
      e = nacl.createEmscriptenInstance(variant.url, width, height);
    } else if (type == "nacl") {
      e = nacl.createEmbedInstance(variant.url, nacl.naclMimeType, width, height);
    } else if (type == "host") {
      e = nacl.createEmbedInstance("bogusURL", nacl.mimetype, width, height);
    } else {
      throw new Error("Unknown variant type " + type);
    }
    if (config.init) {
      config.init(e);
    }
    if (config.progress) {
      e.addEventListener('progress', config.progress);
    }
    if (config.load) {
      e.addEventListener('load', config.load);
    }
    if (config.error) {
      e.addEventListener('error', config.error);
    }
    config.insert.appendChild(e);
    e.load();
    return e;
  };

  var loadStart;

  /**
   * Create the Native Client <embed> element as a child of the DOM element
   * named "listener".
   *
   * @param {string} name The name of the example.
   * @param {string} tool The name of the toolchain, e.g. "glibc", "newlib" etc.
   * @param {string} path Directory name where .nmf file can be found.
   * @param {number} width The width to create the plugin.
   * @param {number} height The height to create the plugin.
   * @param {Object} optional dictionary of args to send to DidCreateInstance
   */
  function createNaClModule(name, tool, path, width, height, args) {
    loadStart = new Date();

    var isRelease = path.toLowerCase().indexOf('release') != -1;

    var progress = document.createElement('progress');
    progress.style.width = '480px';

    var modules = {
        "pnacl": {
          type: "pnacl",
          url: path + '/' + name + '.nmf',
        },
        "nacl": {
          type: "nacl",
          url: path + '/' + name + '.nmf',
        },
        "emscripten": {
          type: "emscripten",
          url: path + '/' + name + '.js',
        },
        "host": {
          type: "host",
          mimetype: 'application/x-ppapi-' + (isRelease ? 'release' : 'debug'),
        }
    };

    var moduleEl = nacl.createInstance({
      module: modules[tool],
      width: width,
      height: height,
      insert: document.getElementById('listener'),
      init: function(e) {
        e.setAttribute('name', 'nacl_module');
        e.setAttribute('id', 'nacl_module');
        e.setAttribute('path', path);
        // Add any optional arguments
        if (args) {
          for (var key in args) {
            e.setAttribute(key, args[key])
          }
        }
      },
      progress: function(evt) {
        var loadPercent = -1.0;
        progress.max = 100;
        if (evt.lengthComputable && evt.total > 0) {
          loadPercent = evt.loaded / evt.total * 100.0;
        }
        progress.value = loadPercent;
      },
      load: function(evt) {
        progress.value = 100;
        document.getElementById('listener').removeChild(progress);
      },
      error: function(evt) {
        progress.value = 100;
        document.getElementById('listener').removeChild(progress);
      },
    });

    if (tool == 'pnacl' && !nacl.hasPNaCl()) {
      updateStatus('PNaCl requires Chrome 31 or newer.');
    } else if (tool == 'nacl' && !nacl.hasNaCl()) {
      updateStatus('NaCl requires Chrome.');
    } else {
      document.getElementById('listener').appendChild(progress);
    }

    // Host plugins don't send a moduleDidLoad message. We'll fake it
    // here.
    var isHost = tool == 'win' || tool == 'linux' || tool == 'mac' || tool == 'host';
    if (isHost) {
      window.setTimeout(function () {
        var evt = document.createEvent('Event');
        evt.initEvent('load', true, true);  // bubbles, cancelable
        moduleEl.dispatchEvent(evt);
      }, 100);  // 100 ms
    }
    return moduleEl;
  }

  /**
   * Add the default "load" and "message" event listeners to the element with
   * id "listener".
   *
   * The "load" event is sent when the module is successfully loaded. The
   * "message" event is sent when the naclModule posts a message using
   * PPB_Messaging.PostMessage() (in C) or pp::Instance().PostMessage() (in
   * C++).
   */
  function attachDefaultListeners() {
    var listenerDiv = document.getElementById('listener');
    listenerDiv.addEventListener('load', moduleDidLoad, true);
    listenerDiv.addEventListener('message', handleMessage, true);
    listenerDiv.addEventListener('crash', handleCrash, true);
    if (typeof window.attachListeners !== 'undefined') {
      window.attachListeners();
    }
  }


  /**
   * Called when the Browser can not communicate with the Module
   *
   * This event listener is registered in attachDefaultListeners above.
   */
  function handleCrash(event) {
    updateStatus('module crashed')
    if (typeof window.handleCrash !== 'undefined') {
      window.handleCrash(common.naclModule.lastError);
    }
  }

  /**
   * Called when the NaCl module is loaded.
   *
   * This event listener is registered in attachDefaultListeners above.
   */
  function moduleDidLoad() {
    common.naclModule = document.getElementById('nacl_module');
    updateStatus('loaded');
    console.log("Create instance: " + (new Date()-loadStart) + " ms");

    if (typeof window.moduleDidLoad !== 'undefined') {
      window.moduleDidLoad();
    }
  }

  /**
   * Hide the NaCl module's embed element.
   *
   * We don't want to hide by default; if we do, it is harder to determine that
   * a plugin failed to load. Instead, call this function inside the example's
   * "moduleDidLoad" function.
   *
   */
  function hideModule() {
    // Setting common.naclModule.style.display = "None" doesn't work; the
    // module will no longer be able to receive postMessages.
    common.naclModule.style.height = "0";
  }

  /**
   * Return true when |s| starts with the string |prefix|.
   *
   * @param {string} s The string to search.
   * @param {string} prefix The prefix to search for in |s|.
   */
  function startsWith(s, prefix) {
    // indexOf would search the entire string, lastIndexOf(p, 0) only checks at
    // the first index. See: http://stackoverflow.com/a/4579228
    return s.lastIndexOf(prefix, 0) === 0;
  }

  /** Maximum length of logMessageArray. */
  var kMaxLogMessageLength = 20;

  /** An array of messages to display in the element with id "log". */
  var logMessageArray = [];

  /**
   * Add a message to an element with id "log".
   *
   * This function is used by the default "log:" message handler.
   *
   * @param {string} message The message to log.
   */
  function logMessage(message) {
    logMessageArray.push(message);
    if (logMessageArray.length > kMaxLogMessageLength)
      logMessageArray.shift();

    document.getElementById('log').textContent = logMessageArray.join('');
    console.log(message)
  }

  /**
   */
  var defaultMessageTypes = {
    'alert': alert,
    'log': logMessage
  };

  /**
   * Called when the NaCl module sends a message to JavaScript (via
   * PPB_Messaging.PostMessage())
   *
   * This event listener is registered in createNaClModule above.
   *
   * @param {Event} message_event A message event. message_event.data contains
   *     the data sent from the NaCl module.
   */
  function handleMessage(message_event) {
    if (typeof message_event.data === 'string') {
      for (var type in defaultMessageTypes) {
        if (defaultMessageTypes.hasOwnProperty(type)) {
          if (startsWith(message_event.data, type + ':')) {
            func = defaultMessageTypes[type];
            func(message_event.data.slice(type.length + 1));
            return;
          }
        }
      }
    }

    if (typeof window.handleMessage !== 'undefined') {
      window.handleMessage(message_event);
    }
  }

  /**
   * Called when the DOM content has loaded; i.e. the page's document is fully
   * parsed. At this point, we can safely query any elements in the document via
   * document.querySelector, document.getElementById, etc.
   *
   * @param {string} name The name of the example.
   * @param {string} tool The name of the toolchain, e.g. "glibc", "newlib" etc.
   * @param {string} path Directory name where .nmf file can be found.
   * @param {number} width The width to create the plugin.
   * @param {number} height The height to create the plugin.
   */
  function domContentLoaded(name, tool, path, width, height) {
    // If the page loads before the Native Client module loads, then set the
    // status message indicating that the module is still loading.  Otherwise,
    // do not change the status message.
    updateStatus('page loaded');
    if (common.naclModule == null) {
      updateStatus('creating ' + tool + ' embed')

      // We use a non-zero sized embed to give Chrome space to place the bad
      // plug-in graphic, if there is a problem.
      width = typeof width !== 'undefined' ? width : 200;
      height = typeof height !== 'undefined' ? height : 200;
      attachDefaultListeners();
      createNaClModule(name, tool, path, width, height);
    } else {
      // It's possible that the Native Client module onload event fired
      // before the page's onload event.  In this case, the status message
      // will reflect 'SUCCESS', but won't be displayed.  This call will
      // display the current message.
      updateStatus('waiting');
    }
  }

  /** Saved text to display in the element with id 'statusField'. */
  var statusText = 'NO-STATUSES';

  /**
   * Set the global status message. If the element with id 'statusField'
   * exists, then set its HTML to the status message as well.
   *
   * @param {string} opt_message The message to set. If null or undefined, then
   *     set element 'statusField' to the message from the last call to
   *     updateStatus.
   */
  function updateStatus(opt_message) {
    if (opt_message) {
      statusText = opt_message;
    }
    var statusField = document.getElementById('statusField');
    if (statusField) {
      statusField.innerHTML = statusText;
    }
  }

  // The symbols to export.
  return {
    /** A reference to the NaCl module, once it is loaded. */
    naclModule: null,

    addListener: addListener,
    getImageDataBuffer: getImageDataBuffer,
    attachDefaultListeners: attachDefaultListeners,
    domContentLoaded: domContentLoaded,
    createNaClModule: createNaClModule,
    hideModule: hideModule,
    logMessage: logMessage,
    updateStatus: updateStatus
  };

}());

// Listen for the DOM content to be loaded. This event is fired when parsing of
// the page's document has finished.
//common.addListener(document, 'DOMContentLoaded', function() {
window.onload = function() {
  var body = document.querySelector('body');

  var loadFunction = common.domContentLoaded;
  // The data-* attributes on the body can be referenced via body.dataset.
  if (body.dataset && body.dataset.customLoad && typeof window.domContentLoaded !== 'undefined') {
    loadFunction = window.domContentLoaded;
  }

  // From https://developer.mozilla.org/en-US/docs/DOM/window.location
  var searchVars = {};
  if (window.location.search.length > 1) {
    var pairs = window.location.search.substr(1).split("&");
    for (var key_ix = 0; key_ix < pairs.length; key_ix++) {
      var keyValue = pairs[key_ix].split("=");
      searchVars[unescape(keyValue[0])] =
        keyValue.length > 1 ? unescape(keyValue[1]) : "";
    }
  }
  if (loadFunction) {
    var name = body.getAttribute("data-name");
    var tc = body.getAttribute("data-tc");
    var path = body.getAttribute("data-path");
    var width = body.getAttribute("data-width") || undefined;
    var height = body.getAttribute("data-height") || undefined;

    var toolchains = (body.getAttribute("data-tools") || tc || "emscripten newlib pnacl").split(' ');
    var configs = (body.getAttribute("data-configs") || "Debug Release").split(' ');

    var tc = toolchains.indexOf(searchVars.tc) !== -1 ?
        searchVars.tc : toolchains[0];
    var config = configs.indexOf(searchVars.config) !== -1 ?
      searchVars.config : configs[0];
    path = path.replace('{tc}', tc).replace('{config}', config);

    // The SDK uses the pnacl toolchain to compile nexes in Debug mode.
    if (tc == 'pnacl' && config == 'Debug') {
      tc = 'nacl';
    }
    if (tc == 'newlib') {
      tc = 'nacl';
    }
    if (tc == 'win' || tc == 'linux' || tc == 'mac') {
      tc = 'host';
    }
    loadFunction(name, tc, path, width, height);
  }
};
//});
