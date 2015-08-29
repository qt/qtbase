// Copyright (c) 2013 Google Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
  var View_IsView = function(resource) {
    return resources.is(resource, VIEW_RESOURCE);
  };

  var View_GetRect = function(resource, rectptr) {
    var view = resources.resolve(resource, VIEW_RESOURCE);
    if (view === undefined) {
      return 0;
    }
    glue.setRect(view.rect, rectptr);
    return 1;
  };

  var View_IsFullscreen = function(resource) {
    var view = resources.resolve(resource, VIEW_RESOURCE);
    if (view === undefined) {
      return 0;
    }
    return view.fullscreen;
  };

  var View_IsVisible = function(resource) {
    var view = resources.resolve(resource, VIEW_RESOURCE);
    if (view === undefined) {
      // Be conservative.
      return 1;
    }
    return view.visible;
  };

  var View_IsPageVisible = function(resource) {
    var view = resources.resolve(resource, VIEW_RESOURCE);
    if (view === undefined) {
      // Be conservative.
      return 1;
    }
    return view.page_visible;
  };

  var View_GetClipRect = function(resource, rectptr) {
    var view = resources.resolve(resource, VIEW_RESOURCE);
    if (view === undefined) {
      return 0;
    }
    glue.setRect(view.clip_rect, rectptr);
    return 1;
  };

  var View_GetDeviceScale = function(resource) {
    var view = resources.resolve(resource, VIEW_RESOURCE);
    if (view === undefined) {
      return 0;
    }
    return window.devicePixelRatio || 1;
  };

  var View_GetCSSScale = function(resource) {
    var view = resources.resolve(resource, VIEW_RESOURCE);
    if (view === undefined) {
      return 0;
    }
    // This doesn't actually take CSS scaling into account, but it's unclear how
    // to get this information in JavaScript.
    return 1;
  };

  registerInterface("PPB_View;1.0", [
    View_IsView,
    View_GetRect,
    View_IsFullscreen,
    View_IsVisible,
    View_IsPageVisible,
    View_GetClipRect,
  ]);

  registerInterface("PPB_View;1.1", [
    View_IsView,
    View_GetRect,
    View_IsFullscreen,
    View_IsVisible,
    View_IsPageVisible,
    View_GetClipRect,
    View_GetDeviceScale,
    View_GetCSSScale,
  ]);
})();
