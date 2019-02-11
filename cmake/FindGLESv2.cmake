include(CheckCXXSourceCompiles)

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


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLESv2 DEFAULT_MSG GLESv2_INCLUDE_DIR GLESv2_LIBRARY HAVE_GLESv2)

mark_as_advanced(GLESv2_INCLUDE_DIR GLESv2_LIBRARY HAVE_GLESv2)

if(GLESv2_FOUND AND NOT TARGET GLESv2::GLESv2)
    add_library(GLESv2::GLESv2 UNKNOWN IMPORTED)
    set_target_properties(GLESv2::GLESv2 PROPERTIES
        IMPORTED_LOCATION "${GLESv2_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${GLESv2_INCLUDE_DIR}")
endif()
