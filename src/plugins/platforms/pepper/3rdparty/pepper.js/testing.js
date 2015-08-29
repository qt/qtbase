// Copyright (c) 2013 Google Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

  var Testing_Dev_ReadImageData = function(device_context_2d, image, top_left) {
    throw "Testing_Dev_ReadImageData not implemented";
  };

  var Testing_Dev_RunMessageLoop = function(instance) {
    throw "Testing_Dev_RunMessageLoop not implemented";
  };

  var Testing_Dev_QuitMessageLoop = function(instance) {
    throw "Testing_Dev_QuitMessageLoop not implemented";
  };

  var Testing_Dev_GetLiveObjectsForInstance = function(instance) {
    return resources.getNumResources();
  };

  var Testing_Dev_IsOutOfProcess = function() {
    throw "Testing_Dev_IsOutOfProcess not implemented";
  };

  var Testing_Dev_SimulateInputEvent = function(instance, input_event) {
    throw "Testing_Dev_SimulateInputEvent not implemented";
  };

  var Testing_Dev_GetDocumentURL = function(instance, components) {
    throw "Testing_Dev_GetDocumentURL not implemented";
  };

  var Testing_Dev_GetLiveVars = function(live_vars, array_size) {
    // TODO(ncbray) split vars from resources so that they can be enumerated.
    throw "Testing_Dev_GetLiveVars not implemented";
  };

  var Testing_Dev_SetMinimumArrayBufferSizeForShmem = function(instance, threshold) {
    throw "Testing_Dev_SetMinimumArrayBufferSizeForShmem not implemented";
  };

  registerInterface("PPB_Testing(Dev);0.92", [
    Testing_Dev_ReadImageData,
    Testing_Dev_RunMessageLoop,
    Testing_Dev_QuitMessageLoop,
    Testing_Dev_GetLiveObjectsForInstance,
    Testing_Dev_IsOutOfProcess,
    Testing_Dev_SimulateInputEvent,
    Testing_Dev_GetDocumentURL,
    Testing_Dev_GetLiveVars,
    Testing_Dev_SetMinimumArrayBufferSizeForShmem,
  ]);
})();
