// Copyright (c) 2013 Google Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

  // Canonicalize the URL using the DOM.
  var resolveURL = function(url) {
    var a = document.createElement('a');
    a.href = url;
    return a.href;
  };

  var updatePendingRead = function(loader) {
    if (loader.pendingReadCallback) {
      var full_read_possible = loader.data.byteLength >= loader.index + loader.pendingReadSize
      if (loader.done || full_read_possible){
        var cb = loader.pendingReadCallback;
        loader.pendingReadCallback = null;
        var readSize = full_read_possible ? loader.pendingReadSize : loader.data.byteLength - loader.index;
        var index = loader.index;
        loader.index += readSize;
        cb(readSize, new Uint8Array(loader.data, index, readSize));
      }
    }
  };

  var URLLoader_Create = function(instance) {
    return resources.register(URL_LOADER_RESOURCE, {
      destroy: function() {
        if (this.response_info !== undefined) {
          resources.release(this.response_info.uid);
        }
      }
    });
  };

  var URLLoader_IsURLLoader = function(resource) {
    return resources.is(resource, URL_LOADER_RESOURCE);
  };

  var URLLoader_Open = function(loader, request, callback) {
    loader = resources.resolve(loader, URL_LOADER_RESOURCE);
    if (loader === undefined) {
      return ppapi.PP_ERROR_BADRESOURCE;
    }
    request = resources.resolve(request, URL_REQUEST_INFO_RESOURCE);
    if (request === undefined) {
      return ppapi.PP_ERROR_BADRESOURCE;
    }
    // TODO(ncbray): what to do if no URL is specified?
    callback = glue.getCompletionCallback(callback);

    loader.data = '';
    loader.index = 0;
    loader.done = false;
    loader.pendingReadCallback = null;
    loader.pendingReadSize = 0;
    loader.progress_bytes = 0;
    loader.progress_total = -1;

    loader.response_info = {
      url: resolveURL(request.url),
      status: 0,
      file_ref: 0,
      destroy: function() {
        if (this.file_ref !== 0) {
          resources.release(this.file_ref);
          this.file_ref = 0;
        }
      }
    };
    resources.register(URL_RESPONSE_INFO_RESOURCE, loader.response_info);

    var req = new XMLHttpRequest();

    req.onprogress = function(evt) {
      if (loader.dead) {
        return;
      }
      loader.progress_bytes = evt.loaded;
      loader.progress_total = evt.total;
    }
    // Called as long as the request completes, even if it's a 404 or similar.
    req.onload = function() {
      if (loader.dead) {
        return;
      }
      loader.response_info.status = this.status;

      // Even a 404 is "OK".
      // This could be done before onload, but because we can't stream data there isn't a huge reason to do it.
      callback(ppapi.PP_OK);

      loader.data = this.response;
      loader.done = true;
      updatePendingRead(loader);
    };
    // Called on network errors and CORS failiures.
    // Note that we do not explicitly distinguish CORS failiures because this information is not exposed to JavaScript.
    req.onerror = function(e) {
      if (loader.dead) {
        return;
      }
      callback(ppapi.PP_ERROR_FAILED);
    };
    req.onabort = function() {
      if (loader.dead) {
        return;
      }
      callback(ppapi.PP_ERROR_ABORTED);
    };

    req.open(request.method || "GET", request.url);
    req.responseType = "arraybuffer";
    req.send();

    return ppapi.PP_OK_COMPLETIONPENDING;
  };

  var URLLoader_FollowRedirect = function() {
    throw "URLLoader_FollowRedirect not implemented";
  };
  var URLLoader_GetUploadProgress = function() {
    throw "URLLoader_GetUploadProgress not implemented";
  };

  var URLLoader_GetDownloadProgress = function(loader, bytes_ptr, total_ptr) {
    var l = resources.resolve(loader, URL_LOADER_RESOURCE);
    if (l === undefined) {
      return 0;
    }
    setValue(bytes_ptr, l.progress_bytes, 'i64');
    setValue(total_ptr, l.progress_total, 'i64');
    return 1;
  };

  var URLLoader_GetResponseInfo = function(loader) {
    var l = resources.resolve(loader, URL_LOADER_RESOURCE);
    if (l === undefined || !l.response_info) {
      return 0;
    }
    var uid = l.response_info.uid;
    // Returned resources have an implicit addRef.
    resources.addRef(uid);
    return uid;
  };

  var URLLoader_ReadResponseBody = function(loader, buffer_ptr, read_size, callback) {
    var loader = resources.resolve(loader, URL_LOADER_RESOURCE);
    if (loader === undefined) {
      return ppapi.PP_ERROR_BADRESOURCE;
    }
    var c = glue.getCompletionCallback(callback);

    loader.pendingReadCallback = function(status, data) {
      HEAP8.set(data, buffer_ptr);
      c(status);
    };
    loader.pendingReadSize = read_size;
    glue.defer(function() {
      updatePendingRead(loader);
    });
    return ppapi.PP_OK_COMPLETIONPENDING;

  };

  var URLLoader_FinishStreamingToFile = function(loader, callback) {
    var loader = resources.resolve(loader, URL_LOADER_RESOURCE);
    if (loader === undefined) {
      return ppapi.PP_ERROR_BADRESOURCE;
    }
    // TODO check STREAM_TO_FILE flag.
    var c = glue.getCompletionCallback(callback);
    // HACK to get the test running but failing.
    glue.defer(function() {
      c(ppapi.PP_OK);
    });
    return ppapi.PP_OK_COMPLETIONPENDING;
  };
  var URLLoader_Close = function() {
    throw "URLLoader_Close not implemented";
  };

  registerInterface("PPB_URLLoader;1.0", [
    URLLoader_Create,
    URLLoader_IsURLLoader,
    URLLoader_Open,
    URLLoader_FollowRedirect,
    URLLoader_GetUploadProgress,
    URLLoader_GetDownloadProgress,
    URLLoader_GetResponseInfo,
    URLLoader_ReadResponseBody,
    URLLoader_FinishStreamingToFile,
    URLLoader_Close
  ]);


  var URLRequestInfo_Create = function(instance) {
    if (resources.resolve(instance, INSTANCE_RESOURCE) === undefined) {
      return 0;
    }
    return resources.register(URL_REQUEST_INFO_RESOURCE, {
      method: "GET",
      headers: "",
      stream_to_file: false,
      follow_redirects: true,
      record_download_progress: false,
      record_upload_progress: false,
      allow_cross_origin_requests: false,
      allow_credentials: false,
      body: null
    });
  };

  var URLRequestInfo_IsURLRequestInfo = function(resource) {
    return resources.is(resource, URL_REQUEST_INFO_RESOURCE);
  };

  var URLRequestInfo_SetProperty = function(request, property, value) {
    var r = resources.resolve(request, URL_REQUEST_INFO_RESOURCE);
    if (r === undefined) {
      return 0;
    }

    // Need to use the ppapi var type to distinguish between ints and floats.
    var var_type = glue.getVarType(value);
    var js_obj = glue.memoryToJSVar(value);

    if (property === 0) {
      if (var_type !== ppapi.PP_VARTYPE_STRING) {
        return 0;
      }
      r.url = js_obj;
    } else if (property === 1) {
      if (var_type !== ppapi.PP_VARTYPE_STRING) {
        return 0;
      }
      // PPAPI does not filter invalid methods at this level.
      r.method = js_obj;
    } else if (property === 2) {
      if (var_type !== ppapi.PP_VARTYPE_STRING) {
        return 0;
      }
      r.headers = js_obj;
    } else if (property === 3) {
      if (var_type !== ppapi.PP_VARTYPE_BOOL) {
        return 0;
      }
      r.stream_to_file = js_obj;
    } else if (property === 4) {
      if (var_type !== ppapi.PP_VARTYPE_BOOL) {
        return 0;
      }
      r.follow_redirects = js_obj;
    } else if (property === 5) {
      if (var_type !== ppapi.PP_VARTYPE_BOOL) {
        return 0;
      }
      r.record_download_progress = js_obj;
    } else if (property === 6) {
      if (var_type !== ppapi.PP_VARTYPE_BOOL) {
        return 0;
      }
      r.record_upload_progress = js_obj;
    } else if (property === 7) {
      if (var_type !== ppapi.PP_VARTYPE_STRING && var_type !== ppapi.PP_VARTYPE_UNDEFINED) {
        return 0;
      }
      r.custom_referrer_url = js_obj;
    } else if (property === 8) {
      if (var_type !== ppapi.PP_VARTYPE_BOOL) {
        return 0;
      }
      r.allow_cross_origin_requests = js_obj;
    } else if (property === 9) {
      if (var_type !== ppapi.PP_VARTYPE_BOOL) {
        return 0;
      }
      r.allow_credentials = js_obj;
    } else if (property === 10) {
      if (var_type !== ppapi.PP_VARTYPE_STRING && var_type !== ppapi.PP_VARTYPE_UNDEFINED) {
        return 0;
      }
      r.custom_content_transfer_encoding = js_obj;
    } else if (property === 11) {
      // TODO(ncbray): require integer, disallow double.
      if (var_type !== ppapi.PP_VARTYPE_INT32) {
        return 0;
      }
      r.prefetch_buffer_upper_threshold = js_obj;
    } else if (property === 12) {
      // TODO(ncbray): require integer, disallow double.
      if (var_type !== ppapi.PP_VARTYPE_INT32) {
        return 0;
      }
      r.prefetch_buffer_lower_threshold = js_obj;
    } else if (property === 13) {
      if (var_type !== ppapi.PP_VARTYPE_STRING && var_type !== ppapi.PP_VARTYPE_UNDEFINED) {
        return 0;
      }
      r.custom_user_agent = js_obj;
    } else {
      console.error("URLRequestInfo_SetProperty got unknown property " + property);
      return 0;
    }
    return 1;
  };

  var URLRequestInfo_AppendDataToBody = function(request, data, len) {
    var r = resources.resolve(request, URL_REQUEST_INFO_RESOURCE);
    if (r === undefined) {
      return 0;
    }
    // TODO(ncbray): actually copy and send the data.  Note the data may not be UTF8.
    return 1;
  };

  var URLRequestInfo_AppendFileToBody = function(request, file_ref, start_offset, number_of_bytes, expect_last_time_modified) {
    throw "URLRequestInfo_AppendFileToBody not implemented";
  };

  registerInterface("PPB_URLRequestInfo;1.0", [
    URLRequestInfo_Create,
    URLRequestInfo_IsURLRequestInfo,
    URLRequestInfo_SetProperty,
    URLRequestInfo_AppendDataToBody,
    URLRequestInfo_AppendFileToBody
  ]);


  var URLResponseInfo_IsURLResponseInfo = function(res) {
    return resources.is(res, URL_RESPONSE_INFO_RESOURCE);
  };

  var URLResponseInfo_GetProperty = function(var_ptr, res, property) {
    var r = resources.resolve(res, URL_RESPONSE_INFO_RESOURCE);
    if (r === undefined) {
      return 0;
    }
    if (property == 0) {
      glue.jsToMemoryVar(r.url, var_ptr);
    } else if (property == 3) {
      glue.setIntVar(r.status, var_ptr);
    } else {
      throw "URLResponseInfo_GetProperty not implemented: " + property;
    }
    return 1;
  };

  var URLResponseInfo_GetBodyAsFileRef = function(res) {
    var r = resources.resolve(res, URL_RESPONSE_INFO_RESOURCE);
    if (r === undefined) {
      return 0;
    }
    var uid = r.file_ref;
    if (uid === 0) {
      return 0;
    }
    resource.addRef(uid);
    return uid;
  };


  registerInterface("PPB_URLResponseInfo;1.0", [
    URLResponseInfo_IsURLResponseInfo,
    URLResponseInfo_GetProperty,
    URLResponseInfo_GetBodyAsFileRef
  ]);

})();
