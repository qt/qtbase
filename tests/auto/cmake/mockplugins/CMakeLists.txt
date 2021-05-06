# Generated from qtsvg.pro.

cmake_minimum_required(VERSION 3.15.0)

include(.cmake.conf)
project(QtMockPlugins
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt MockPlugins Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

# Make sure we only use latest private CMake API, aka no compatibility wrappers.
set(QT_NO_INTERNAL_COMPATIBILITY_FUNCTIONS TRUE)

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core)
find_package(Qt6 ${PROJECT_VERSION} QUIET CONFIG OPTIONAL_COMPONENTS Gui Widgets Xml)

qt_build_repo_begin()

add_subdirectory(mockplugins1)
add_subdirectory(mockplugins2)
add_subdirectory(mockplugins3)

add_subdirectory(mock1plugin)
add_subdirectory(mock2plugin)
add_subdirectory(mock3plugin)
add_subdirectory(mock4plugin)
add_subdirectory(mock5plugin)
add_subdirectory(mock6plugin)

qt_build_repo_end()
