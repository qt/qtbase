## Force C++ standard, do not fall back, do not use compiler extensions
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

## Position independent code:
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Do not relink dependent libraries when no header has changed:
set(CMAKE_LINK_DEPENDS_NO_SHARED ON)

# Default to hidden visibility for symbols:
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

## Enable testing:
include(CTest)
enable_testing()

## Define some constants to check for certain platforms, etc:
include(QtPlatformSupport)

## add_qt_module and co.:
include(QtBuild)

## Qt Feature support:
include(QtFeature)

## Compiler optimization flags:
include(QtCompilerOptimization)

## Find host tools (if non native):
set(HOST_QT_TOOLS_DIRECTORY "" CACHE PATH "Directory with Qt host tools.")

if (CMAKE_CROSSCOMPILING AND "x${HOST_QT_TOOLS_DIRECTORY}" STREQUAL "x")
    message(FATAL_ERROR "You need to set HOST_QT_TOOLS_DIRECTORY for a cross-complile.")
endif()

## Find syncqt in HOST TOOLS or locally:
if("x${HOST_QT_TOOLS_DIRECTORY}" STREQUAL "x")
    set(QT_SYNCQT "${PROJECT_SOURCE_DIR}/bin/syncqt.pl")
    install(PROGRAMS "${QT_SYNCQT}" DESTINATION "${INSTALL_BINDIR}")
else()
    set(QT_SYNCQT "${HOST_QT_TOOLS_DIRECTORY}/syncqt.pl")
endif()

## Enable support for sanitizers:
include(${CMAKE_CURRENT_LIST_DIR}/3rdparty/extra-cmake-modules/modules/ECMEnableSanitizers.cmake)

option(QT_USE_CCACHE "Enable the use of ccache")
if(QT_USE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_OBJC_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()
