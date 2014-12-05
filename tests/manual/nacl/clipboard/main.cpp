/* Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/input_event.h"
#include <stdio.h>

class MyInstance : public pp::Instance {
 public:
  explicit MyInstance(PP_Instance instance) : pp::Instance(instance) {;
      puts("Hello World from MyInstance");
  }
  virtual ~MyInstance() {}
  
  virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]) {
    RequestFilteringInputEvents(PP_INPUTEVENT_CLASS_KEYBOARD);
    return true;
  }
  
  virtual bool HandleInputEvent (const pp::InputEvent &event) { 
    puts("event");
    return false; // discard all events
  }
};

class MyModule : public pp::Module {
 public:
  MyModule() : pp::Module() {}
  virtual ~MyModule() {}
  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    return new MyInstance(instance);
  }
};

namespace pp {
  Module* CreateModule() {
  return new MyModule();
}

}  // namespace pp

