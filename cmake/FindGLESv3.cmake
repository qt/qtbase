# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include(CheckCXXSourceCompiles)

# Many embedded drivers expose ES 3.x via libGLESv2.
find_library(GLESv3_LIBRARY NAMES GLESv3 OpenGLES GLESv2)
find_path(GLESv3_INCLUDE_DIR
    NAMES "GLES3/gl3.h" "OpenGLES/ES3/gl.h"
    DOC "The OpenGLES 3 include path"
)
find_package(EGL)
set(_libraries "${CMAKE_REQUIRED_LIBRARIES}")
if(GLESv3_LIBRARY)
    list(APPEND CMAKE_REQUIRED_LIBRARIES "${GLESv3_LIBRARY}")
endif ()
if(EGL_LIBRARY)
    list(APPEND CMAKE_REQUIRED_LIBRARIES "${EGL_LIBRARY}")
endif()
set(_includes "${CMAKE_REQUIRED_INCLUDES}")
list(APPEND CMAKE_REQUIRED_INCLUDES "${GLESv3_INCLUDE_DIR}")

find_package(PlatformGraphics QUIET)
if(TARGET PlatformGraphics::PlatformGraphics)
    platform_graphics_extend_check_cxx_source_required_variables()
endif()
check_cxx_source_compiles("
#ifdef __APPLE__
#  include <OpenGLES/ES3/gl.h>
#else
#  define GL_GLEXT_PROTOTYPES
#  include <GLES3/gl3.h>
#endif

int main(void)
{
    /* BEGIN TEST: */
static GLfloat f[6];
glGetStringi(GL_EXTENSIONS, 0);
glReadBuffer(GL_COLOR_ATTACHMENT1);
glUniformMatrix2x3fv(0, 0, GL_FALSE, f);
glMapBufferRange(GL_ARRAY_BUFFER, 0, 0, GL_MAP_READ_BIT);
    /* END TEST: */
    return 0;
}" HAVE_GLESv3)

set(CMAKE_REQUIRED_LIBRARY "${_libraries}")
unset(_libraries)
set(CMAKE_REQUIRED_INCLUDES "${_includes}")
unset(_includes)
set(package_args GLESv3_INCLUDE_DIR GLESv3_LIBRARY HAVE_GLESv3)

# Framework handling partially inspired by FindGLUT.cmake.
if(GLESv3_LIBRARY MATCHES "/([^/]+)\\.framework$")
    # TODO: Might need to handle non .tbd suffixes, but didn't find an
    # example like that.
    # TODO: Might need to handle INTERFACE_INCLUDE_DIRECTORIES differently.
    set(_library_imported_location "${GLESv3_LIBRARY}/${CMAKE_MATCH_1}.tbd")
    if(NOT EXISTS "${_library_imported_location}")
        set(_library_imported_location "")
    endif()
else()
    set(_library_imported_location "${GLESv3_LIBRARY}")
endif()
set(GLESv3_LIBRARY "${_library_imported_location}")

list(APPEND package_args HAVE_GLESv3)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLESv3 DEFAULT_MSG ${package_args})

mark_as_advanced(${package_args})

if(GLESv3_FOUND AND NOT TARGET GLESv3::GLESv3)
    if(IOS)
        add_library(GLESv3::GLESv3 INTERFACE IMPORTED)
        # For simulator_and_device builds we can't specify the full library path, because
        # it's specific to either the device or the simulator. Resort to passing a link
        # flag instead.
        target_link_libraries(GLESv3::GLESv3 INTERFACE "-framework OpenGLES")
    else()
        add_library(GLESv3::GLESv3 UNKNOWN IMPORTED)
        set_target_properties(GLESv3::GLESv3 PROPERTIES
            IMPORTED_LOCATION "${GLESv3_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${GLESv3_INCLUDE_DIR}")

        if(TARGET EGL::EGL)
            target_link_libraries(GLESv3::GLESv3 INTERFACE "EGL::EGL")
        elseif(EGL_LIBRARY)
            target_link_libraries(GLESv3::GLESv3 INTERFACE "${EGL_LIBRARY}")
        endif()
    endif()
endif()
