// Copyright (c) 2013 Google Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ppapi/c/pp_errors.h"

#include "ppapi/c/ppb_var.h"
#include "ppapi/c/ppp.h"
#include "ppapi/c/ppp_input_event.h"
#include "ppapi/c/ppp_instance.h"
#include "ppapi/c/ppp_messaging.h"

extern "C" {
  const void* GetBrowserInterface(const char* interface_name);

  void DoPostMessage(PP_Instance instance, const PP_Var* var) {
    const PPP_Messaging* messaging_interface = (const PPP_Messaging*)PPP_GetInterface(PPP_MESSAGING_INTERFACE);
    if (!messaging_interface) {
      return;
    }
    messaging_interface->HandleMessage(instance, *var);
    // It appears that the callee own the var, so no need to release it?
  }

  void DoChangeView(PP_Instance instance, PP_Resource resource) {
    const PPP_Instance* instance_interface = (const PPP_Instance*)PPP_GetInterface(PPP_INSTANCE_INTERFACE);
    if (instance_interface == NULL) {
      printf("STUB: Failed to get instance interface.\n");
      return;
    }
    instance_interface->DidChangeView(instance, resource);
  }

  void DoChangeFocus(PP_Instance instance, PP_Bool hasFocus) {
    const PPP_Instance* instance_interface = (const PPP_Instance*)PPP_GetInterface(PPP_INSTANCE_INTERFACE);
    if (instance_interface == NULL) {
      printf("STUB: Failed to get instance interface.\n");
      return;
    }
    instance_interface->DidChangeFocus(instance, hasFocus);
  }

  void Shutdown(PP_Instance instance) {
    // Work around a bug in Emscripten that prevent malloc from being included unless it is referenced from native code.
    // This appears to be live to the compiler, but instance will never be 0.
    // TODO(ncbray): fix the bug and remove this hack.
    if (instance == 0) {
      free(malloc(1));
    }
    const PPP_Instance* instance_interface = (const PPP_Instance*)PPP_GetInterface(PPP_INSTANCE_INTERFACE);
    if (instance_interface) {
      instance_interface->DidDestroy(instance);
    }
    PPP_ShutdownModule();
  }

  void NativeCreateInstance(PP_Instance instance, uint32_t argc, const char *argn[], const char *argv[]) {
    int32_t status = PPP_InitializeModule(1, &GetBrowserInterface);
    if (status != PP_OK) {
      printf("STUB: Failed to initialize module.\n");
      return;
    }

    const PPP_Instance* instance_interface = (const PPP_Instance*)PPP_GetInterface(PPP_INSTANCE_INTERFACE);
    if (instance_interface == NULL) {
      printf("STUB: Failed to get instance interface.\n");
      return;
    }

    status = instance_interface->DidCreate(instance, argc, argn, argv);
    if (status != PP_TRUE) {
      printf("STUB: Failed to create instance.\n");
      Shutdown(instance);
      return;
    }
  }

  PP_Bool HandleInputEvent(PP_Instance instance, PP_Resource input_event) {
    const PPP_InputEvent* event_interface = (const PPP_InputEvent*)PPP_GetInterface(PPP_INPUT_EVENT_INTERFACE);
    if (event_interface == NULL) {
      printf("STUB: Failed to get input event interface.\n");
      return PP_FALSE;
    }
    return event_interface->HandleInputEvent(instance, input_event);
  }

}
