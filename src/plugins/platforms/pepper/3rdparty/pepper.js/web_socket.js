// Copyright (c) 2014 Google Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
/*

Queue.js

A function to represent a queue

Created by Stephen Morley - http://code.stephenmorley.org/ - and released under
the terms of the CC0 1.0 Universal legal code:

http://creativecommons.org/publicdomain/zero/1.0/legalcode

*/

  /* Creates a new queue. A queue is a first-in-first-out (FIFO) data structure -
   * items are added to the end of the queue and removed from the front.
   */
  function Queue(){

      // initialise the queue and offset
      var queue  = [];
      var offset = 0;

      // Returns the length of the queue.
      this.getLength = function(){
          return (queue.length - offset);
      };

      // Returns true if the queue is empty, and false otherwise.
      this.isEmpty = function(){
          return (queue.length == 0);
      };

      /* Enqueues the specified item. The parameter is:
       *
       * item - the item to enqueue
       */
      this.enqueue = function(item){
          queue.push(item);
      };

      /* Dequeues an item and returns it. If the queue is empty, the value
       * 'undefined' is returned.
       */
      this.dequeue = function(){

          // if the queue is empty, return immediately
          if (queue.length == 0) return undefined;

          // store the item at the front of the queue
          var item = queue[offset];

          // increment the offset and remove the free space if necessary
          if (++ offset * 2 >= queue.length){
              queue  = queue.slice(offset);
              offset = 0;
          }

          // return the dequeued item
          return item;

      };

  };

  var WebSocket_Create = function(instance) {
    return resources.register(WEB_SOCKET_RESOURCE, {
      destroy: function() {
      }
    });
  };

  var WebSocket_IsWebSocket = function(resource) {
    return resources.is(resource, WEB_SOCKET_RESOURCE);
  };

  var WebSocket_Connect = function(socketResource, url, protocols, protocolCount, cCallback) {
    if (protocolCount > 1) {
        // TODO(danielrh) currently don't support protocol arrays
        throw "Multiple specified protocol Support not implemented";
        return ppapi.PP_ERROR_FAILED;
    }
    var protocol = [];
    if (protocolCount == 1) {
        protocol = [glue.memoryToJSVar(protocols)];
    }
    var socket = resources.resolve(socketResource, WEB_SOCKET_RESOURCE);
    if (socket === undefined) {
      return ppapi.PP_ERROR_BADRESOURCE;
    }
    var callback = glue.getCompletionCallback(cCallback);
    var req = new WebSocket(glue.memoryToJSVar(url), protocol);
    req.binaryType = "arraybuffer";
    socket.websocket = req;
    socket.closeWasClean = false;
    socket.closeCode = 0;
    socket.closeReason = "";
    socket.closeCallback = null;
    socket.onmessageCallback = null;
    socket.onmessageWritableVar = null;
    socket.receiveQueue = new Queue();

    req.onclose = function (evt) {
        socket.closeWasClean = evt.wasClean;
        socket.closeCode = evt.closeCode;
        socket.closeReason = evt.closeReason;
        if (socket.closeCallback !== null && !socket.dead) {
            socket.closeCallback(ppapi.PP_OK);
            socket.closeCallback = null;
        }
    };
    req.onmessage = function(evt) {
        if (socket.onmessageCallback !== null) {
            var memoryDestination = socket.onmessageWritableVar;
            var messageCallback = socket.onmessageCallback;
            socket.onmessageWritableVar = null;
            socket.onmessageCallback = null; //set it null now in case callback sets it again
            glue.jsToMemoryVar(evt.data, memoryDestination);
            messageCallback(ppapi.PP_OK);
        } else {
            socket.receiveQueue.enqueue(evt.data);
        }
    };
    req.onopen = function() {
      if (socket.dead) {
        return;
      }
      callback(ppapi.PP_OK);
    };
    // Called on network errors and CORS failiures.
    // Note that we do not explicitly distinguish CORS failiures because this information is not exposed to JavaScript.
    req.onerror = function(e) {
      if (socket.dead) {
        return;
      }
      callback(ppapi.PP_ERROR_FAILED);
    };

    return ppapi.PP_OK_COMPLETIONPENDING;
  };

  var hasSocketClassConnected = function(socket) {
      if (socket.websocket) {
          return true;
      } else {
          return false;
      }
  };

  var WebSocket_Close = function(socketResource, codeUint16, reasonResource, callback) {
    var socket = resources.resolve(socketResource, WEB_SOCKET_RESOURCE);
    if (socket === undefined) {
      return ppapi.PP_ERROR_BADRESOURCE;
    }
    if (!hasSocketClassConnected(socket)) {
        return ppapi.PP_ERROR_FAILED;
    }
    var reason = glue.memoryToJSVar(reasonResource);
    if (reason.length > 123) {
        return ppapi.PP_ERROR_BADARGUMENT;
    }
    if (codeUint16 != 1000 && !(codeUint16 >= 3000 && codeUint16 < 4999)) {
        return ppapi.PP_ERROR_NOACCESS;
    }
    if (socket.closeCallback !== null) {
        return ppapi.PP_ERROR_INPROGRESS;
    }
    socket.closeCallback = glue.getCompletionCallback(callback);
    socket.websocket.close(codeUint16, reason);
    return ppapi.PP_OK_COMPLETIONPENDING;
  };

  var WebSocket_ReceiveMessage = function(socketResource, messagePtr, cCallback) {
    var socket = resources.resolve(socketResource, WEB_SOCKET_RESOURCE);
    if (socket === undefined) {
      return ppapi.PP_ERROR_BADRESOURCE;
    }
    if (!hasSocketClassConnected(socket)) {
        return ppapi.PP_ERROR_FAILED;
    }
    if (socket.receiveQueue.isEmpty()) {
        socket.onmessageWritableVar = messagePtr;
        socket.onmessageCallback = glue.getCompletionCallback(cCallback);
        return ppapi.PP_OK_COMPLETIONPENDING;
    } else {
        glue.jsToMemoryVar(socket.receiveQueue.dequeue(), messagePtr);
        return ppapi.PP_OK;
    }
  };

  var WebSocket_SendMessage = function(socketResource, message) {
    var socket = resources.resolve(socketResource, WEB_SOCKET_RESOURCE);
    if (socket === undefined) {
      return ppapi.PP_ERROR_BADRESOURCE;
    }
    if (!hasSocketClassConnected(socket)) {
        return ppapi.PP_ERROR_FAILED;
    }
    if (socket.websocket.readyState == 0) {
        return ppapi.PP_ERROR_FAILED;
    }
    socket.websocket.send(glue.memoryToJSVar(message));
    return ppapi.PP_OK;
  };

  var WebSocket_GetBufferedAmount = function(socketResource) {
    var socket = resources.resolve(socketResource, WEB_SOCKET_RESOURCE);
    if (socket === undefined) {
      return 0;
    }
    if (!hasSocketClassConnected(socket)) {
        return 0;
    }
    return socket.websocket.bufferedAmount;
  };

  var WebSocket_GetCloseCode = function(socketResource) {
    var socket = resources.resolve(socketResource, WEB_SOCKET_RESOURCE);
    if (socket === undefined) {
      return 0;
    }
    return socket.closeCode;
  };

  var returnString = function(result, string){
    var memory = 0;
    var len = string.length;
    memory = _malloc(len + 1);
    for (var i = 0; i < len; i++) {
      HEAPU8[memory + i] = string.charCodeAt(i);
    }
    // Null terminate the string because why not?
    HEAPU8[memory + len] = 0;
    setValue(result, ppapi.PP_VARTYPE_STRING, 'i32');
    setValue(result + 8, resources.registerString(string, memory, len), 'i32');
  };

  var WebSocket_GetCloseReason = function(result, socketResource) {
    var socket = resources.resolve(socket, WEB_SOCKET_RESOURCE);
    if (socket === undefined) {
      glue.jsToMemoryVar(null, result);
      return;
    }
    returnString(result, socket.closeReason);
    return;
  };

  var WebSocket_GetCloseWasClean = function(socketResource) {
    var socket = resources.resolve(socketResource, WEB_SOCKET_RESOURCE);
    if (socket === undefined) {
      return 0;
    }
    return socket.closeWasClean;
  };

  var WebSocket_GetExtensions = function(result, socketResource) {
    var socket = resources.resolve(socketResource, WEB_SOCKET_RESOURCE);
    if (socket === undefined) {
      glue.jsToMemoryVar(null, result);
    }
    returnString(result, '');
    return;
  };

  var WebSocket_GetProtocol = function(result, socketResource) {
    var socket = resources.resolve(socketResource, WEB_SOCKET_RESOURCE);
    if (socket === undefined) {
      glue.jsToMemoryVar(null, result);
    }
    if (!hasSocketClassConnected(socket)) {
        returnString (result, '');
        return;
    }
    returnString(result, socket.websocket.protocol);
    return;
  };

  var WebSocket_GetReadyState = function(socketResource) {
    var socket = resources.resolve(socketResource, WEB_SOCKET_RESOURCE);
    if (socket === undefined) {
      return ppapi.PP_VARTYPE_UNDEFINED;
    }
    if (!hasSocketClassConnected(socket)) {
      return ppapi.PP_WEBSOCKETREADYSTATE_INVALID;
    }
    return socket.websocket.readyState;
  };

  var WebSocket_GetURL = function(result, socketResource) {
    var socket = resources.resolve(socketResource, WEB_SOCKET_RESOURCE);
    if (socket === undefined) {
        glue.jsToMemoryVar(null, result);
        return;
    }
    if (!hasSocketClassConnected(socket)) {
        returnString(result, '');
        return;
    }
    returnString(result, socket.websocket.url);
    return;
  };

  registerInterface("PPB_WebSocket;1.0", [
    WebSocket_Create,
    WebSocket_IsWebSocket,
    WebSocket_Connect,
    WebSocket_Close,
    WebSocket_ReceiveMessage,
    WebSocket_SendMessage,
    WebSocket_GetBufferedAmount,
    WebSocket_GetCloseCode,
    WebSocket_GetCloseReason,
    WebSocket_GetCloseWasClean,
    WebSocket_GetExtensions,
    WebSocket_GetProtocol,
    WebSocket_GetReadyState,
    WebSocket_GetURL
  ]);

})();
