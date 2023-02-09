# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(QT_FEATURE_evdev)
    add_subdirectory(evdevmouse)
    add_subdirectory(evdevtouch)
    add_subdirectory(evdevkeyboard)
endif()
if(QT_FEATURE_evdev AND QT_FEATURE_tabletevent)
    add_subdirectory(evdevtablet)
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
    # add_subdirectory(bsdkeyboard) # special case TODO
    # add_subdirectory(bsdmouse) # special case TODO
endif()
