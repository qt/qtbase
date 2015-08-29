// Copyright (c) 2013 Google Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

  //Enums copied from ppb_input_event.h
  var PP_InputEvent_Type = {
    UNDEFINED: -1,
    MOUSEDOWN: 0,
    MOUSEUP: 1,
    MOUSEMOVE: 2,
    MOUSEENTER: 3,
    MOUSELEAVE: 4,
    WHEEL: 5,
    RAWKEYDOWN: 6,
    KEYDOWN: 7,
    KEYUP: 8,
    CHAR: 9,
    CONTEXTMENU: 10,
    IME_COMPOSITION_START: 11,
    IME_COMPOSITION_UPDATE: 12,
    IME_COMPOSITION_END: 13,
    IME_TEXT: 14,
    TOUCHSTART: 15,
    TOUCHMOVE: 16,
    TOUCHEND: 17,
    TOUCHCANCEL: 18
  };
  var PPIE_Type = PP_InputEvent_Type;

  var PP_InputEvent_MouseButton = {
    NONE: -1,
    LEFT: 0,
    MIDDLE: 1,
    RIGHT: 2
  };
  var PPIE_MouseButton = PP_InputEvent_MouseButton;

  var PP_InputEvent_Modifier = {
    SHIFTKEY: 1 << 0,
    CONTROLKEY: 1 << 1,
    ALTKEY: 1 << 2,
    METAKEY: 1 << 3,
    ISKEYPAD: 1 << 4,
    ISAUTOREPEAT: 1 << 5,
    LEFTBUTTONDOWN: 1 << 6,
    MIDDLEBUTTONDOWN: 1 << 7,
    RIGHTBUTTONDOWN: 1 << 8,
    CAPSLOCKKEY: 1 << 9,
    NUMLOCKKEY: 1 << 10,
    ISLEFT: 1 << 11,
    ISRIGHT: 1 << 12
  };
  var PPIE_Modifier = PP_InputEvent_Modifier;

  var PP_InputEvent_Class = {
    MOUSE: 1 << 0,
    KEYBOARD: 1 << 1,
    WHEEL: 1 << 2,
    TOUCH: 1 << 3,
    IME: 1 << 4
  };
  var PPIE_Class = PP_InputEvent_Class;

  var mod_masks = {
    "shiftKey": PP_InputEvent_Modifier.SHIFTKEY,
    "ctrlKey": PP_InputEvent_Modifier.CONTROLKEY,
    "altKey": PP_InputEvent_Modifier.ALTKEY,
    "metaKey": PP_InputEvent_Modifier.METAKEY
  };

  var mod_buttons = [
    PPIE_Modifier.LEFTBUTTONDOWN,
    PPIE_Modifier.MIDDLEBUTTONDOWN,
    PPIE_Modifier.RIGHTBUTTONDOWN
  ];

  var eventclassdata = {
    MOUSE: {
      "mousedown": PPIE_Type.MOUSEDOWN,
      "mouseup": PPIE_Type.MOUSEUP,
      "mousemove": PPIE_Type.MOUSEMOVE,
      "mouseenter": PPIE_Type.MOUSEENTER,
      "mouseout": PPIE_Type.MOUSELEAVE,
      "contextmenu": PPIE_Type.CONTEXTMENU
    },

    KEYBOARD: {
      "keydown": PPIE_Type.KEYDOWN,
      "keyup": PPIE_Type.KEYUP,
      "keypress": PPIE_Type.CHAR
    },

    WHEEL: {
      "mousewheel": PPIE_Type.WHEEL,  // Chrome
      "wheel": PPIE_Type.WHEEL        // Firefox
    }
  };

  // TODO reset on focus lost?
  var button_state = [false, false, false];

  var GetEventPos = function(event) {
    // hopefully this will be reasonably cross platform
    var bcr = event.target.getBoundingClientRect();
    return {
      x: event.clientX-bcr.left,
      y: event.clientY-bcr.top
    };
  };

  var GetWheelScroll = function(e) {
    var x = e.deltaX !== undefined ? e.deltaX : -e.wheelDeltaX;
    var y = e.deltaY !== undefined ? e.deltaY : -e.wheelDeltaY;
    // scroll by lines, not pixels/pages
    if (e.deltaMode === 1) {
      x *= 40;
      y *= 40;
    }
    return {x: x, y: y};
  };

  var GetMovement = function(e) {
    return {
      //will be e.movementX once standardized, but for now we need to polyfill
      x: e.webkitMovementX || e.mozMovementX || 0,
      y: e.webkitMovementY || e.mozMovementY || 0
    };
  };

  var RegisterHandlers = function(instance, event_classes, filtering) {
    var resource = resources.resolve(instance, INSTANCE_RESOURCE);
    if (resource === undefined) {
      return;
    }

    var makeCallback = function(event_type) {
      return function(event) {
        var button = event.button;
        if (typeof button !== 'number') {
          button = -1;
        }
        if (button !== -1) {
          if (event_type === PPIE_Type.MOUSEDOWN) {
            button_state[button] = true;
          } else if (event_type === PPIE_Type.MOUSEUP) {
            button_state[button] = false;
          } else {
            // Only MOUSEDOWN and MOUSEUP specify a button, as per the PPAPI
            // documentation.  In practice, however, Chrome also specifies a
            // button for MOUSEMOVE.  If would be possible to emulate this
            // behavior in JavaScript, with the caveat that mouse motion events
            // do not distinguish between left button held and no button held
            // (event.button === 0) so additional bookkeeping would be required.
            // It is somewhat unclear what button will be reported on various
            // platforms, however, (Last pressed?  Lowest enumeration?) because
            // motion is not directly associated with a button, unlike click
            // events.  Sticking with the well-defined documented behavior is
            // the simplest solution, although it may result in portability
            // issues if a program relies on Chrome's behavior.
            button = -1;
          }
        }

        var modifiers = 0;
        for(var key in mod_masks) {
          if (mod_masks.hasOwnProperty(key) && event[key]) {
            modifiers |= mod_masks[key];
          }
        }

        // This departs slightly from the Chrome's PPAPI implementation.
        // Webkit will only have modifiers for one of the mouse buttons being held.  (The one with the lowest enum?)
        // Webkit will only have mouse button modifers for mouse events, too.
        // But really, this wierd an non-orthogonal, so we don't bother emulating it.
        for(var i = 0; i < mod_buttons.length; i++) {
          if (button_state[i]) {
            modifiers |= mod_buttons[i];
          }
        }

        var obj_uid = resources.register(INPUT_EVENT_RESOURCE, {
          // can't use type as attribute name because it is used internally
          ie_type: event_type,
          pos: GetEventPos(event),
          button: button,
          // TODO(grosse): Make sure this actually follows the Pepper API
          time: event.timeStamp,
          modifiers: modifiers,
          movement: GetMovement(event),
          delta: GetWheelScroll(event),
          scrollByPage: event.deltaMode === 2,
          keyCode: event.keyCode
        });

        var rval = _HandleInputEvent(instance, obj_uid);
        if (!filtering || rval) {
          // Don't prevent default on mousedown so we can get focus when clicked
          if (event_type !== PPIE_Type.MOUSEDOWN) {
            event.preventDefault();
          }
          event.stopPropagation();
        }
        return filtering && !rval;
      };
    };

    var elt = resource.element;
    for(var key in eventclassdata) {
      if (eventclassdata.hasOwnProperty(key) && (event_classes & PPIE_Class[key])) {
        var data = eventclassdata[key];

        for(var key2 in data) {
          if (data.hasOwnProperty(key2)) {
            elt.addEventListener(key2, makeCallback(data[key2]), false);
          }
        }
      }
    }
  };

  var isMouseEvent = function(res) {
    var type = res.ie_type;
    return (
        type === PPIE_Type.MOUSEDOWN ||
        type === PPIE_Type.MOUSEUP ||
        type === PPIE_Type.MOUSEMOVE ||
        type === PPIE_Type.MOUSEENTER ||
        type === PPIE_Type.MOUSELEAVE ||
        type === PPIE_Type.CONTEXTMENU
    );
  };

  var isWheelEvent = function(res) {
    var type = res.ie_type;
    return (type === PPIE_Type.WHEEL);
  };

  var isKeyboardEvent = function(res) {
    var type = res.ie_type;
    return (
        type === PPIE_Type.KEYDOWN ||
        type === PPIE_Type.KEYUP ||
        type === PPIE_Type.RAWKEYDOWN ||
        type === PPIE_Type.CHAR
    );
  };

  var InputEvent_RequestInputEvents = function(instance, event_classes) {
    RegisterHandlers(instance, event_classes, false);
  };

  var InputEvent_RequestFilteringInputEvents = function(instance, event_classes) {
    RegisterHandlers(instance, event_classes, true);
  };

  var InputEvent_ClearInputEventRequest = function(instance, event_classes) {
    throw "InputEvent_ClearInputEventRequest not implemented";
  };

  var InputEvent_IsInputEvent = function(resource) {
    return resources.is(resource, INPUT_EVENT_RESOURCE);
  };

  var InputEvent_GetType = function(event) {
    var resource = resources.resolve(event, INPUT_EVENT_RESOURCE);
    if (resource !== undefined) {
      return resource.ie_type;
    } else {
      return PPIE_Type.UNDEFINED;
    }
  };

  var InputEvent_GetTimeStamp = function(event) {
    var resource = resources.resolve(event, INPUT_EVENT_RESOURCE);
    if (resource !== undefined) {
      return resource.time;
    } else {
      return 0.0;
    }
  };


  var InputEvent_GetModifiers = function(event) {
    var resource = resources.resolve(event, INPUT_EVENT_RESOURCE);
    if (resource !== undefined) {
      return resource.modifiers;
    } else {
      return 0;
    }
  };

  registerInterface("PPB_InputEvent;1.0", [
    InputEvent_RequestInputEvents,
    InputEvent_RequestFilteringInputEvents,
    InputEvent_ClearInputEventRequest,
    InputEvent_IsInputEvent,
    InputEvent_GetType,
    InputEvent_GetTimeStamp,
    InputEvent_GetModifiers
  ]);

  var MouseInputEvent_Create = function() {
    throw "MouseInputEvent_Create not implemented";
  };

  var MouseInputEvent_IsMouseInputEvent = function(event) {
    var _event = resources.resolve(event, INPUT_EVENT_RESOURCE);
    if (_event === undefined) {
      return 0;
    }
    return +isMouseEvent(_event);
  };

  var MouseInputEvent_GetButton = function(event) {
    var resource = resources.resolve(event, INPUT_EVENT_RESOURCE);
    if (resource !== undefined) {
      return resource.button;
    } else {
      return PPIE_MouseButton.NONE;
    }
  };

  var MouseInputEvent_GetPosition = function(ptr, event) {
    var point = {x: 0, y: 0};
    var _event = resources.resolve(event, INPUT_EVENT_RESOURCE);
    if (_event !== undefined && isMouseEvent(_event)) {
      point = _event.pos;
    }
    glue.setPoint(point, ptr);
  };

  var MouseInputEvent_ClickCount = function(event) {
    // TODO(grosse): Find way to implement this
    return 0;
  };

  var MouseInputEvent_Movement = function(ptr, event) {
    var point = {x: 0, y: 0};
    var _event = resources.resolve(event, INPUT_EVENT_RESOURCE);
    if (_event !== undefined && isMouseEvent(_event)) {
      point = _event.movement;
    }
    glue.setPoint(point, ptr);
  };

  registerInterface("PPB_MouseInputEvent;1.1", [
    MouseInputEvent_Create,
    MouseInputEvent_IsMouseInputEvent,
    MouseInputEvent_GetButton,
    MouseInputEvent_GetPosition,
    MouseInputEvent_ClickCount,
    MouseInputEvent_Movement
  ]);

  var WheelInputEvent_Create = function() {
    throw "WheelInputEvent_Create not implemented";
  };

  var WheelInputEvent_IsWheelInputEvent = function(event) {
    var _event = resources.resolve(event, INPUT_EVENT_RESOURCE);
    if (_event === undefined) {
      return 0;
    }
    return +isWheelEvent(_event);
  };

  var WheelInputEvent_GetDelta = function(ptr, event) {
    var point = {x: 0, y: 0};
    var _event = resources.resolve(event, INPUT_EVENT_RESOURCE);
    if (_event !== undefined && isWheelEvent(_event)) {
      point = _event.delta;
    }
    glue.setFloatPoint(point, ptr);
  };

  var deltaToTick = function(value) {
    return (value > 0) ? 1 : ((value < 0) ? -1 : 0);
  };

  var WheelInputEvent_GetTicks = function(ptr, event) {
    // TODO(ncbray): get tick directly from event object?
    var point = {x: 0, y: 0};
    var _event = resources.resolve(event, INPUT_EVENT_RESOURCE);
    if (_event !== undefined && isWheelEvent(_event)) {
      point.x = deltaToTick(_event.delta.x);
      point.y = deltaToTick(_event.delta.y);
    }
    glue.setFloatPoint(point, ptr);
  };

  var WheelInputEvent_GetScrollByPage = function(event) {
    var res = resources.resolve(event, INPUT_EVENT_RESOURCE);
    if (res === undefined) {
      return 0;
    }
    return +res.scrollByPage;
  };

  registerInterface("PPB_WheelInputEvent;1.0", [
    WheelInputEvent_Create,
    WheelInputEvent_IsWheelInputEvent,
    WheelInputEvent_GetDelta,
    WheelInputEvent_GetTicks,
    WheelInputEvent_GetScrollByPage
  ]);

  var KeyboardInputEvent_Create = function() {
    throw "KeyboardInputEvent_Create not implemented";
  };

  var KeyboardInputEvent_IsKeyboardInputEvent = function(event) {
    var _event = resources.resolve(event, INPUT_EVENT_RESOURCE);
    if (_event === undefined) {
      return 0;
    }
    return +isKeyboardEvent(_event);
  };

  var KeyboardInputEvent_GetKeyCode = function(event) {
    var res = resources.resolve(event, INPUT_EVENT_RESOURCE);
    if (res === undefined) {
      return 0;
    }
    return res.keyCode;
  };

  var KeyboardInputEvent_GetCharacterText = function(ptr, event) {
    // TODO(grosse): Find way to implement this
    glue.jsToMemoryVar(undefined, ptr);
  };

  registerInterface("PPB_KeyboardInputEvent;1.0", [
    KeyboardInputEvent_Create,
    KeyboardInputEvent_IsKeyboardInputEvent,
    KeyboardInputEvent_GetKeyCode,
    KeyboardInputEvent_GetCharacterText
  ]);

})();
