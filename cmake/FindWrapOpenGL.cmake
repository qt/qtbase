# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET WrapOpenGL::WrapOpenGL)
    set(WrapOpenGL_FOUND ON)
    return()
endif()

set(WrapOpenGL_FOUND OFF)

# FindOpenGL sets OPENGL_USE_<COMPONENT> as normal variables in the calling scope so a project's
# find_package(OpenGL COMPONENTS ...) changes what Qt's find_package(OpenGL) lookup requires.
# Hide the normal variables during Qt's lookup and restore them afterwards.
# Cache values (e.g. from the command line) are treated as intentional and remain set.
set(__qt_wrap_opengl_components "")
if(NOT QT_NO_OVERRIDE_OPENGL_USE_VARIABLES)
    set(__qt_wrap_opengl_components OPENGL GLX EGL GLES2 GLES3)
endif()
foreach(__qt_wrap_opengl_component IN LISTS __qt_wrap_opengl_components)
    set(__qt_wrap_opengl_var OPENGL_USE_${__qt_wrap_opengl_component})
    if(DEFINED ${__qt_wrap_opengl_var}
            AND (NOT DEFINED CACHE{${__qt_wrap_opengl_var}}
                 OR NOT "${${__qt_wrap_opengl_var}}" STREQUAL "$CACHE{${__qt_wrap_opengl_var}}"))
        set(__qt_wrap_opengl_saved_${__qt_wrap_opengl_component} "${${__qt_wrap_opengl_var}}")
        set(__qt_wrap_opengl_had_${__qt_wrap_opengl_component} TRUE)
    else()
        set(__qt_wrap_opengl_had_${__qt_wrap_opengl_component} FALSE)
    endif()
    unset(${__qt_wrap_opengl_var})
endforeach()

find_package(OpenGL ${WrapOpenGL_FIND_VERSION})

if (OpenGL_FOUND)
    set(WrapOpenGL_FOUND ON)

    add_library(WrapOpenGL::WrapOpenGL INTERFACE IMPORTED)
    if(APPLE)
        # CMake 3.27 and older:
        # On Darwin platforms FindOpenGL sets IMPORTED_LOCATION to the absolute path of the library
        # within the framework. This ends up as an absolute path link flag, which we don't want,
        # because that makes our .prl files un-relocatable.
        # Extract the framework path instead, and use that in INTERFACE_LINK_LIBRARIES,
        # which CMake ends up transforming into a relocatable -framework flag.
        # See https://gitlab.kitware.com/cmake/cmake/-/issues/20871 for details.
        #
        # CMake 3.28 and above:
        # IMPORTED_LOCATION is the absolute path the the OpenGL.framework folder.
        get_target_property(__opengl_fw_lib_path OpenGL::GL IMPORTED_LOCATION)
        if(__opengl_fw_lib_path AND NOT __opengl_fw_lib_path MATCHES "/([^/]+)\\.framework$")
            get_filename_component(__opengl_fw_path "${__opengl_fw_lib_path}" DIRECTORY)
        endif()

        if(NOT __opengl_fw_path)
            # Just a safety measure in case if no OpenGL::GL target exists.
            set(__opengl_fw_path "-framework OpenGL")
        endif()

        target_link_libraries(WrapOpenGL::WrapOpenGL INTERFACE ${__opengl_fw_path})
    else()
        target_link_libraries(WrapOpenGL::WrapOpenGL INTERFACE OpenGL::GL)
    endif()
elseif(UNIX AND NOT APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "Integrity")
    # Requesting only the OpenGL component ensures CMake does not mark the package as
    # not found if neither GLX nor libGL are available. This allows finding OpenGL
    # on an X11-less Linux system.
    find_package(OpenGL ${WrapOpenGL_FIND_VERSION} COMPONENTS OpenGL)
    if (OpenGL_FOUND)
        set(WrapOpenGL_FOUND ON)
        add_library(WrapOpenGL::WrapOpenGL INTERFACE IMPORTED)
        target_link_libraries(WrapOpenGL::WrapOpenGL INTERFACE OpenGL::OpenGL)
    endif()
endif()

foreach(__qt_wrap_opengl_component IN LISTS __qt_wrap_opengl_components)
    set(__qt_wrap_opengl_var OPENGL_USE_${__qt_wrap_opengl_component})
    if(__qt_wrap_opengl_had_${__qt_wrap_opengl_component})
        set(${__qt_wrap_opengl_var} "${__qt_wrap_opengl_saved_${__qt_wrap_opengl_component}}")
    else()
        unset(${__qt_wrap_opengl_var})
    endif()
    unset(__qt_wrap_opengl_saved_${__qt_wrap_opengl_component})
    unset(__qt_wrap_opengl_had_${__qt_wrap_opengl_component})
endforeach()
unset(__qt_wrap_opengl_component)
unset(__qt_wrap_opengl_components)
unset(__qt_wrap_opengl_var)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapOpenGL DEFAULT_MSG WrapOpenGL_FOUND)
