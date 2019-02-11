include(CheckCXXSourceCompiles)
include(CMakePushCheckState)

find_library(EGL_LIBRARY NAMES EGL)
find_path(EGL_INCLUDE_DIR NAMES "EGL/egl.h" DOC "The EGL include path")

cmake_push_check_state(RESET)
list(APPEND CMAKE_REQUIRED_LIBRARIES "${EGL_LIBRARY}")
list(APPEND CMAKE_REQUIRED_INCLUDES "${EGL_INCLUDE_DIR}")

check_cxx_source_compiles("
#include <EGL/egl.h>

int main(int argc, char *argv[]) {
    EGLint x = 0; EGLDisplay dpy = 0; EGLContext ctx = 0;
    eglDestroyContext(dpy, ctx);
}" HAVE_EGL)

cmake_pop_check_state()


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EGL DEFAULT_MSG EGL_INCLUDE_DIR EGL_LIBRARY HAVE_EGL)

mark_as_advanced(EGL_INCLUDE_DIR EGL_LIBRARY HAVE_EGL)

if(EGL_FOUND AND NOT TARGET EGL::EGL)
    add_library(EGL::EGL UNKNOWN IMPORTED)
    set_target_properties(EGL::EGL PROPERTIES
        IMPORTED_LOCATION "${EGL_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${EGL_INCLUDE_DIR}")
endif()
