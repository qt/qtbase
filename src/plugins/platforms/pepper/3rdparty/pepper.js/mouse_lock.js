// Copyright (c) 2013 Google Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

  var MouseLock_LockMouse = function(instance, callback) {
    var res = resources.resolve(instance, INSTANCE_RESOURCE);
    if (res === undefined) {
      return;
    }
    // TODO why does targeting the enclosing element cause difficulty?
    var target = res.element;
    var cb_func = glue.getCompletionCallback(callback);

    var makeCallback = function(return_code) {
      return function(event) {
        cb_func(return_code);
        return true;
      };
    };

    if('webkitRequestPointerLock' in target) {
      // TODO(grosse): Figure out how to handle the callbacks properly
      target.addEventListener('webkitpointerlockchange', makeCallback(ppapi.PP_OK));
      target.addEventListener('webkitpointerlockerror', makeCallback(ppapi.PP_ERROR_FAILED));
      target.webkitRequestPointerLock();
    } else {
      // Note: This may not work as Firefox currently requires fullscreen before requesting pointer lock
      target.addEventListener('mozpointerlockchange', makeCallback(ppapi.PP_OK));
      target.addEventListener('mozpointerlockerror', makeCallback(ppapi.PP_ERROR_FAILED));
      target.mozRequestPointerLock();
    }
  };

  var MouseLock_UnlockMouse = function(instance) {
    throw "MouseLock_UnlockMouse not implemented";
  };


  registerInterface("PPB_MouseLock;1.0", [
    MouseLock_LockMouse,
    MouseLock_UnlockMouse,
  ], function() {
    var b = document.body;
    return b.webkitRequestPointerLock || b.mozRequestPointerLock;
  });

  var FullScreen_IsFullscreen = function(instance) {
    var res = resources.resolve(instance, INSTANCE_RESOURCE);
    if (res === undefined) {
      return 0;
    }
    var element = document.fullscreenElement || document.webkitFullscreenElement || document.mozFullScreenElement || document.msFullscreenElement;
    return element === res.element ? 1 : 0;
  };

  var FullScreen_SetFullscreen = function(instance, fullscreen) {
    var res = resources.resolve(instance, INSTANCE_RESOURCE);
    if (res == undefined) {
      return 0;
    }
    var element = res.element;

    if (fullscreen) {
      if (element.requestFullscreen) {
        element.requestFullscreen();
      } else if (element.mozRequestFullScreen) {
        element.mozRequestFullScreen();
      } else if (element.webkitRequestFullscreen) {
        element.webkitRequestFullscreen();
      } else if (element.msRequestFullscreen) {
        element.msRequestFullscreen();
      }
    } else {
      if (document.cancelFullscreen) {
        document.cancelFullscreen();
      } else if (document.mozCancelFullScreen) {
        document.mozCancelFullScreen();
      } else if (document.webkitCancelFullScreen) {
        document.webkitCancelFullScreen();
      } else if (document.msExitFullscreen) {
        document.msExitFullscreen();
      }
    }

    return 1;
  };


  var FullScreen_GetScreenSize = function() {
    throw "FullScreen_GetScreenSize not implemented";
  };

  registerInterface("PPB_Fullscreen;1.0", [
    FullScreen_IsFullscreen,
    FullScreen_SetFullscreen,
    FullScreen_GetScreenSize
  ], function() {
    var b = document.body;
    return b.requestFullscreen || b.mozRequestFullScreen || b.webkitRequestFullscreen || b.msRequestFullscreen;
  });

})();
