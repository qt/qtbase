// Copyright (c) 2013 Google Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

  var PP_FILESYSTEMTYPE_INVALID = 0;
  var PP_FILESYSTEMTYPE_EXTERNAL = 1;
  var PP_FILESYSTEMTYPE_LOCALPERSISTENT = 2;
  var PP_FILESYSTEMTYPE_LOCALTEMPORARY = 3;

  var fsTypeMap = {};
  fsTypeMap[PP_FILESYSTEMTYPE_LOCALPERSISTENT] = window.PERSISTENT;
  fsTypeMap[PP_FILESYSTEMTYPE_LOCALTEMPORARY] = window.TEMPORARY;

  var FileSystem_Create = function(instance, type) {
    // Creating a filesystem is asynchronous, so just store args for later
    return resources.register(FILE_SYSTEM_RESOURCE, {fs_type: type, fs: null});
  };

  var FileSystem_IsFileSystem = function(res) {
    return resources.is(res, FILE_SYSTEM_RESOURCE);
  };

  // Note that int64s are passed as two arguments, with high word second
  var FileSystem_Open = function(file_system, size_low, size_high, callback_ptr) {
    var res = resources.resolve(file_system, FILE_SYSTEM_RESOURCE);
    if (res === undefined) {
      return ppapi.PP_ERROR_BADRESOURCE;
    }
    var callback = glue.getCompletionCallback(callback_ptr);

    var type = fsTypeMap[res.fs_type];
    if (type === undefined) {
      return ppapi.PP_ERROR_FAILED;
    }

    var requestFS = window.requestFileSystem || window.webkitRequestFileSystem;
    requestFS(type, glue.ToI64(size_low, size_high), function(fs) {
      res.fs = fs;
      callback(ppapi.PP_OK);
    }, function(error) {
      console.log('Error!', error);
      callback(ppapi.PP_ERROR_FAILED);
    });

    return ppapi.PP_OK_COMPLETIONPENDING;
  };

  var FileSystem_GetType = function() {
    throw "FileSystem_GetType not implemented";
  };


  registerInterface("PPB_FileSystem;1.0", [
      FileSystem_Create,
      FileSystem_IsFileSystem,
      FileSystem_Open,
      FileSystem_GetType
  ]);


  var FileRef_Create = function(file_system, path_ptr) {
    var path = glue.decodeUTF8(path_ptr);
    if (path === null) {
      // Not UTF8
      return 0;
    }
    resources.addRef(file_system);
    return resources.register(FILE_REF_RESOURCE, {
        path: path,
        file_system: file_system,
        destroy: function () {
          resources.release(file_system);
        }
      });
  };

  var FileRef_IsFileRef = function(res) {
    return resources.is(res, FILE_REF_RESOURCE);
  };

  var FileRef_GetFileSystemType = function() {
    throw "FileRef_GetFileSystemType not implemented";
  };

  var FileRef_GetName = function() {
    throw "FileRef_GetName not implemented";
  };

  var FileRef_GetPath = function() {
    throw "FileRef_GetPath not implemented";
  };

  var FileRef_GetParent = function() {
    throw "FileRef_GetParent not implemented";
  };

  var FileRef_MakeDirectory = function() {
    throw "FileRef_MakeDirectory not implemented";
  };

  var FileRef_Touch = function() {
    throw "FileRef_Touch not implemented";
  };

  var FileRef_Delete = function(file_ref, callback_ptr) {
    var callback = glue.getCompletionCallback(callback_ptr);
    var ref = resources.resolve(file_ref, FILE_REF_RESOURCE);
    if (ref === undefined) {
      return ppapi.PP_ERROR_BADRESOURCE;
    }
    var file_system = resources.resolve(ref.file_system, FILE_SYSTEM_RESOURCE);
    if (file_system  === undefined) {
      // This is an internal error.
      return ppapi.PP_ERROR_FAILED;
    }

    var error_handler = function(error) {
      var code = error.code;
      if (code === FileError.NOT_FOUND_ERR) {
        callback(ppapi.PP_ERROR_FILENOTFOUND);
      } else {
        callback(ppapi.PP_ERROR_FAILED);
      }
    };

    file_system.fs.root.getFile(ref.path, {}, function(entry) {
      entry.remove(function() {
        callback(ppapi.PP_OK);
      }, function(error) {
        console.log('Unhandled fileref error!', error);
        throw 'Unhandled fileref error!' + error;
      });
    }, error_handler);

    return ppapi.PP_OK_COMPLETIONPENDING;
  };

  var FileRef_Rename = function() {
    throw "FileRef_Rename not implemented";
  };


  registerInterface("PPB_FileRef;1.0", [
      FileRef_Create,
      FileRef_IsFileRef,
      FileRef_GetFileSystemType,
      FileRef_GetName,
      FileRef_GetPath,
      FileRef_GetParent,
      FileRef_MakeDirectory,
      FileRef_Touch,
      FileRef_Delete,
      FileRef_Rename
  ]);


  var PP_FLAGS_WRITE = 1 << 1;
  var PP_FLAGS_CREATE = 1 << 2;
  var PP_FLAGS_TRUNCATE = 1 << 3;
  var PP_FLAGS_EXCLUSIVE = 1 << 4;

  var PP_FILETYPE_REGULAR = 0;
  var PP_FILETYPE_DIRECTORY = 1;
  var PP_FILETYPE_OTHER = 2;

  var DummyError = function(error) {
    console.log('Unhandled fileio error!', error);
    throw 'Unhandled fileio error: ' + error;
  };

  var FileIO_Create = function(instance) {
    if (!resources.is(instance, INSTANCE_RESOURCE)) {
      return 0;
    }
    return resources.register(FILE_IO_RESOURCE, {
        closed: false,
        entry: null,
        fs_type: 0,
        flags: 0
    });
  };

  var FileIO_IsFileIO = function(res) {
    return resources.is(res, FILE_IO_RESOURCE);
  };

  var FileIO_Open = function(file_io, file_ref, flags, callback_ptr) {
    var io = resources.resolve(file_io, FILE_IO_RESOURCE);
    if (io === undefined) {
      return ppapi.PP_ERROR_BADRESOURCE;
    }
    var ref = resources.resolve(file_ref, FILE_REF_RESOURCE);
    if (ref === undefined) {
      return ppapi.PP_ERROR_BADRESOURCE;
    }
    var file_system = resources.resolve(ref.file_system, FILE_SYSTEM_RESOURCE);
    if (file_system  === undefined) {
      // This is an internal error.
      return ppapi.PP_ERROR_FAILED;
    }
    var callback = glue.getCompletionCallback(callback_ptr);

    if (io.closed || io.entry !== null) {
      return ppapi.PP_ERROR_FAILED;
    }

    var js_flags = {
      create: (flags & PP_FLAGS_CREATE) !== 0,
      exclusive: (flags & PP_FLAGS_EXCLUSIVE) !== 0
    };

    file_system.fs.root.getFile(ref.path, js_flags, function(entry) {
      if (io.dead || file_system.dead) {
        return callback(ppapi.PP_ERROR_ABORTED);
      }

      io.entry = entry;
      io.fs_type = file_system.fs_type;

      if (flags & PP_FLAGS_TRUNCATE) {
        entry.createWriter(function(writer) {
          writer.onwrite = function(event) {
            callback(ppapi.PP_OK);
          }
          writer.onerror = DummyError;
          writer.truncate(0);
        }, DummyError);
      } else {
        callback(ppapi.PP_OK);
      }
    }, function(error) {
      var code = error.code;
      if (code === FileError.NOT_FOUND_ERR) {
        callback(ppapi.PP_ERROR_FILENOTFOUND)
      } else {
        callback(ppapi.PP_ERROR_FAILED)
      }
    });
    return ppapi.PP_OK_COMPLETIONPENDING;
  };

  var AccessFile = function(file_io, callback_ptr, body) {
    var io = resources.resolve(file_io, FILE_IO_RESOURCE);
    if (io === undefined) {
      return ppapi.PP_ERROR_BADRESOURCE;
    }
    var callback = glue.getCompletionCallback(callback_ptr);
    if (io.closed) {
      return callback(ppapi.PP_ERROR_ABORTED);
    }

    body(io, io.entry, callback);
    return ppapi.PP_OK_COMPLETIONPENDING;
  }

  // This setter is only used by the file APIs, so it is implemented here rather
  // than with the other getters and setters.
  var setFileInfo = function(obj, ptr) {
    setValue(ptr, obj.size_low, 'i32');
    setValue(ptr + 4, obj.size_high, 'i32');
    setValue(ptr + 8, obj.type, 'i32');
    setValue(ptr + 12, obj.system_type, 'i32');
    setValue(ptr + 16, obj.creation_time, 'double');
    setValue(ptr + 24, obj.last_access_time, 'double');
    setValue(ptr + 32, obj.last_modified_time, 'double');
  };

  var FileIO_Query = function(file_io, info_ptr, callback_ptr) {
    return AccessFile(file_io, callback_ptr, function(io, entry, callback) {
        entry.getMetadata(function(metadata) {

          if (io.dead) {
            return callback(ppapi.PP_ERROR_ABORTED);
          }

          var info = {
              size_low: metadata.size % glue.k2_32,
              size_high: (metadata.size / glue.k2_32) | 0,
              type: PP_FILETYPE_REGULAR,
              system_type: io.fs_type,
              creation_time: 0.0,
              last_access_time: 0.0,
              last_modified_time: metadata.modificationTime ? metadata.modificationTime.valueOf() / 1000 : 0.0
          };

          setFileInfo(info, info_ptr);

          callback(ppapi.PP_OK);
        }, DummyError);
    });
  };

  var FileIO_Touch = function() {
    throw "FileIO_Touch not implemented";
  };

  var FileIO_Read = function(file_io, offset_low, offset_high, output_ptr, bytes_to_read, callback_ptr) {
    return AccessFile(file_io, callback_ptr, function(io, entry, callback) {
        entry.file(function(file) {
          var reader = new FileReader();
          reader.onload = function(event) {

            var offset = glue.ToI64(offset_low, offset_high);
            var buffer = reader.result.slice(offset, offset + bytes_to_read);
            HEAP8.set(new Int8Array(buffer), output_ptr);
            callback(buffer.byteLength);
          };
          reader.onerror = DummyError;
          reader.readAsArrayBuffer(file);
        }, DummyError);
    });
  };

  var FileIO_Write = function(file_io, offset_low, offset_high, input_ptr, bytes_to_read, callback_ptr) {
    return AccessFile(file_io, callback_ptr, function(io, entry, callback) {
        entry.createWriter(function(writer) {
          var buffer = HEAP8.subarray(input_ptr, input_ptr + bytes_to_read);
          var offset = glue.ToI64(offset_low, offset_high);

          writer.seek(offset);
          // TODO(ncbray): listen to onwrite.  The polyfill for firefox currently doesn't fire onwrite.
          writer.onwrite = function(event) {
            callback(buffer.byteLength);
          }
          writer.onerror = DummyError;
          writer.write(new Blob([buffer]));
        }, DummyError);
    });
  };

  var FileIO_SetLength = function() {
    throw "FileIO_SetLength not implemented";
  };

  var FileIO_Flush = function(file_io, callback_ptr) {
    // Basically a NOP in the current implementation
    var io = resources.resolve(file_io, FILE_IO_RESOURCE);
    if (io === undefined) {
      return ppapi.PP_ERROR_BADRESOURCE;
    }
    var callback = glue.getCompletionCallback(callback_ptr);
    callback(io.closed ? ppapi.PP_ERROR_ABORTED : ppapi.PP_OK);
  };

  var FileIO_Close = function() {
    throw "FileIO_Close not implemented";
  };

  var FileIO_ReadToArray = function() {
    throw "FileIO_ReadToArray not implemented";
  };


  registerInterface("PPB_FileIO;1.1", [
      FileIO_Create,
      FileIO_IsFileIO,
      FileIO_Open,
      FileIO_Query,
      FileIO_Touch,
      FileIO_Read,
      FileIO_Write,
      FileIO_SetLength,
      FileIO_Flush,
      FileIO_Close,
      FileIO_ReadToArray
  ]);

  registerInterface("PPB_FileIO;1.0", [
      FileIO_Create,
      FileIO_IsFileIO,
      FileIO_Open,
      FileIO_Query,
      FileIO_Touch,
      FileIO_Read,
      FileIO_Write,
      FileIO_SetLength,
      FileIO_Flush,
      FileIO_Close
  ]);

})();
