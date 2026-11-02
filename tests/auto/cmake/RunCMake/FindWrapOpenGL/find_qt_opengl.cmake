# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Looks up Qt6::OpenGL, then checks that WrapOpenGL::WrapOpenGL only links to existing targets
# and that Qt's lookup did not change the project's OPENGL_USE_<COMPONENT> variables.
set(components OPENGL GLX EGL GLES2 GLES3)
foreach(component IN LISTS components)
    if(DEFINED OPENGL_USE_${component})
        set(before_${component} "${OPENGL_USE_${component}}")
    else()
        set(before_${component} "<undefined>")
    endif()
endforeach()

find_package(Qt6 REQUIRED COMPONENTS OpenGL)

get_target_property(wrap_libs WrapOpenGL::WrapOpenGL INTERFACE_LINK_LIBRARIES)
foreach(lib IN LISTS wrap_libs)
    if(lib MATCHES "^OpenGL::" AND NOT TARGET "${lib}")
        message(FATAL_ERROR "WrapOpenGL::WrapOpenGL links to ${lib}, which does not exist.")
    endif()
endforeach()

foreach(component IN LISTS components)
    if(DEFINED OPENGL_USE_${component})
        set(after "${OPENGL_USE_${component}}")
    else()
        set(after "<undefined>")
    endif()
    if(NOT after STREQUAL before_${component})
        message(FATAL_ERROR "OPENGL_USE_${component} changed from '${before_${component}}'"
            " to '${after}' by find_package(Qt6 COMPONENTS OpenGL).")
    endif()
endforeach()
