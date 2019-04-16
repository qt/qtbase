if(QT_FEATURE_evdev)
    add_subdirectory(evdevmouse)
    add_subdirectory(evdevtouch)
    add_subdirectory(evdevkeyboard)

    if(QT_FEATURE_tabletevent)
        add_subdirectory(evdevtablet)
    endif()
endif()

if(QT_FEATURE_tslib)
    add_subdirectory(tslib)
endif()

if(QT_FEATURE_tuiotouch)
    add_subdirectory(tuiotouch)
endif()

if(QT_FEATURE_libinput)
    add_subdirectory(libinput)
endif()

if(FREEBSD)
#   TODO  add_subdirectory(bsdkeyboard)
#   TODO  add_subdirectory(bsdmouse)
endif()
