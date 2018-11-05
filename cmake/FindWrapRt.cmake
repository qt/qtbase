include(CheckCXXSourceCompiles)

find_library(LIBRT rt)

set(_libraries "${CMAKE_REQUIRED_LIBRARIES}")
if(LIBRT_FOUND)
    list(APPEND CMAKE_REQUIRED_LIBRARIES "${LIBRT}")
endif()

check_cxx_source_compiles("
#include <unistd.h>
#include <time.h>

int main(int argc, char *argv[]) {
    timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
}" HAVE_GETTIME)

set(CMAKE_REQUIRED_LIBRARIES "${_libraries}")
unset(_libraries)

add_library(WrapRt INTERFACE)
if (LIBRT_FOUND)
    target_link_libraries(WrapRt INTERFACE "${LIBRT}")
endif()

set(WrapRt_FOUND "${HAVE_GETTIME}")

