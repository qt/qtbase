include(CheckCXXSourceCompiles)

# No library linkage is necessary to use GLESv2 with Emscripten. The headers are also
# system headers, so we don't need to search for them.
if(EMSCRIPTEN)
    set(HAVE_GLESv2 ON)
else()
    find_library(GLESv2_LIBRARY NAMES GLESv2)
    find_path(GLESv2_INCLUDE_DIR NAMES "GLES2/gl2.h" DOC "The OpenGLES 2 include path")
    set(_libraries "${CMAKE_REQUIRED_LIBRARIES}")
    list(APPEND CMAKE_REQUIRED_LIBRARIES "${GLESv2_LIBRARY}")
    set(_includes "${CMAKE_REQUIRED_INCLUDES}")
    list(APPEND CMAKE_REQUIRED_INCLUDES "${GLESv2_INCLUDE_DIR}")

    check_cxx_source_compiles("
#ifdef __APPLE__
#  include <OpenGLES/ES2/gl.h>
#else
#  define GL_GLEXT_PROTOTYPES
#  include <GLES2/gl2.h>
#endif

int main(int argc, char *argv[]) {
    glUniform1f(1, GLfloat(1.0));
    glClear(GL_COLOR_BUFFER_BIT);
}" HAVE_GLESv2)

    set(CMAKE_REQUIRED_LIBRARY "${_libraries}")
    unset(_libraries)
    set(CMAKE_REQUIRED_INCLUDES "${_includes}")
    unset(_includes)
    set(package_args GLESv2_INCLUDE_DIR GLESv2_LIBRARY HAVE_GLESv2)
endif()

list(APPEND package_args HAVE_GLESv2)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLESv2 DEFAULT_MSG ${package_args})

mark_as_advanced(${package_args})

if(GLESv2_FOUND AND NOT TARGET GLESv2::GLESv2)
    if(EMSCRIPTEN)
        add_library(GLESv2::GLESv2 INTERFACE IMPORTED)
    else()
        add_library(GLESv2::GLESv2 UNKNOWN IMPORTED)
        set_target_properties(GLESv2::GLESv2 PROPERTIES
            IMPORTED_LOCATION "${GLESv2_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${GLESv2_INCLUDE_DIR}")
    endif()
endif()
