/* Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/** @file hello_world.c
 * This example demonstrates loading, running and scripting a very simple
 * NaCl module.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ppapi/c/pp_errors.h"
#include "ppapi/c/pp_module.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/ppb.h"
#include "ppapi/c/ppb_instance.h"
#include "ppapi/c/ppb_messaging.h"
#include "ppapi/c/ppb_var.h"
#include "ppapi/c/ppp.h"
#include "ppapi/c/ppp_instance.h"
#include "ppapi/c/ppp_messaging.h"

//
// Prototype OpenGL function resolving
//

// A C++ inlining optimization. Seems to require ../command_buffer/client/gles2_lib.h,
// which is nowhere to be found.
// #define GLES2_INLINE_OPTIMIZATION


// A CPP bindings macro. Not defined.
//#undef GLES2_USE_CPP_BINDINGS
// #define GLES2_USE_CPP_BINDINGS

#include <GLES2/gl2.h>

void callGl()
{
    // Qt uses a clever resolving system to resolve the OpenGL functions,
    // which conflicts with Pepper's clever macro system for exporting them.

    // We can now call the glClearColor as long as we keep the macro in place
    //#undef glClearColor;
    glClearColor(1.0, 1.0, 1.0, 1.0);

    // This is the actual symbol name exported in -lppapi_gles2. We can call it
    // directly, but are then relying on the name staying the same.
    GLES2ClearColor(1.0, 1.0, 1.0, 1.0);
    
    // C++ version
    //gles2::GetGLContext()->glClearColor(1.0, 1.0, 1.0, 1.0);
}

// A redirection approach:

// Header
GL_APICALL void GL_APIENTRY qtClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);

// Source
// #include <GLES2/gl2.h> to get the glClearColor macro
GL_APICALL void GL_APIENTRY qtClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    glClearColor(1.0, 1.0, 1.0, 1.0);
}

// Resolve header
// clean up macro pollution:
#undef glClearColor;
class GlFunctions
{
public:
    static void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
};

void GlFunctions::glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    qtClearColor(red, green, blue, alpha);
}

void useResolvedGlFunctions()
{
    GlFunctions::glClearColor(1.0, 1.0, 1.0, 1.0);
}

static PPB_Messaging* ppb_messaging_interface = NULL;
static PPB_Var* ppb_var_interface = NULL;


static PP_Bool Instance_DidCreate(PP_Instance instance,
                                  uint32_t argc,
                                  const char* argn[],
                                  const char* argv[]) {
  return PP_TRUE;
}


static void Instance_DidDestroy(PP_Instance instance) {
}

static void Instance_DidChangeView(PP_Instance instance,
                                   PP_Resource view_resource) {
}

static void Instance_DidChangeFocus(PP_Instance instance,
                                    PP_Bool has_focus) {
}

static PP_Bool Instance_HandleDocumentLoad(PP_Instance instance,
                                           PP_Resource url_loader) {
  /* NaCl modules do not need to handle the document load function. */
  return PP_FALSE;
}


PP_EXPORT int32_t PPP_InitializeModule(PP_Module a_module_id,
                                       PPB_GetInterface get_browser) {
  ppb_messaging_interface =
      (PPB_Messaging*)(get_browser(PPB_MESSAGING_INTERFACE));
  ppb_var_interface = (PPB_Var*)(get_browser(PPB_VAR_INTERFACE));
  return PP_OK;
}


PP_EXPORT const void* PPP_GetInterface(const char* interface_name) {
  if (strcmp(interface_name, PPP_INSTANCE_INTERFACE) == 0) {
    static PPP_Instance instance_interface = {
      &Instance_DidCreate,
      &Instance_DidDestroy,
      &Instance_DidChangeView,
      &Instance_DidChangeFocus,
      &Instance_HandleDocumentLoad,
    };
    return &instance_interface;
  }
  return NULL;
}

PP_EXPORT void PPP_ShutdownModule() {
}

