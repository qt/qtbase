
if(QT_FEATURE_evdev)
    add_subdirectory(evdevmouse)
#   TODO  add_subdirectory(evdevtouch)
#   TODO  add_subdirectory(evdevkeyboard)

    if(QT_FEATURE_tabletevent)
#      TODO   add_subdirectory(evdevtablet)
    endif()
endif()

if(QT_FEATURE_tslib)
#   TODO  add_subdirectory(tslib)
endif()

if(QT_FEATURE_tuiotouch)
#   TODO  add_subdirectory(tuiotouch)
endif()

if(QT_FEATURE_libinput)
#   TODO  add_subdirectory(libinput)
endif()

if(FREEBSD)
#   TODO  add_subdirectory(bsdkeyboard)
#   TODO  add_subdirectory(bsdmouse)
endif()
