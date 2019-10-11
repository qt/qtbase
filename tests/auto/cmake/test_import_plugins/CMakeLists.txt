
cmake_minimum_required(VERSION 3.1)

project(import_plugins_advanced)
enable_testing()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/FindPackageHints.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/FindPackageHints.cmake")
endif()

# Need to find Qt5Core explicitly because the MockPlugins1 and MockPlugins2 config files
# are in a different directory (the source dir) when doing a standalone tests build,
# whereas Core is in the installed directory, and due to NO_DEFAULT_PATH being used
# for the Core dependency call in Qt5MockPlugins, Core would not be found in the source
# dir.
find_package(Qt5 COMPONENTS Core REQUIRED HINTS ${Qt5Tests_PREFIX_PATH})
get_target_property(qt_is_static Qt5::Core TYPE)

# For a similar reason, we need to find the MockPlugins packages not via COMPONENTS argument,
# but directly, because the location of Qt5Config.cmake is in the installed dir, while
# the MockPlugins are in the source dir, and Qt5Config only looks for packages relative
# to its own location.
# The packages are still successfuly found, because the CMAKE_PREFIX_PATH populated by qmake
# contains both the installed Qt dir, and the Qt source dir.
find_package(Qt5MockPlugins1 REQUIRED HINTS ${Qt5Tests_PREFIX_PATH})
find_package(Qt5MockPlugins2 REQUIRED HINTS ${Qt5Tests_PREFIX_PATH})

function(create_test_executable TARGET_NAME)
    set(CHECK_FILE ${CMAKE_BINARY_DIR}/${TARGET_NAME}_check.cpp)

    set(EXPECTED_PLUGINS)
    foreach(_p ${ARGN})
        string(APPEND EXPECTED_PLUGINS "    \"${_p}\",\n")
    endforeach()
    configure_file("${CMAKE_SOURCE_DIR}/check.cpp.in" ${CHECK_FILE})

    add_executable(${TARGET_NAME} main.cpp ${CHECK_FILE})
    target_link_libraries(${TARGET_NAME} Qt5::MockPlugins1)
    add_test(test_${TARGET_NAME} ${TARGET_NAME})
endfunction()

create_test_executable(default QMock1Plugin QMock2Plugin)
# No call to qt5_import_plugins() for the default

# TODO This test is known to fail because CMake currently doesn't have a way to
# implement its own equivalent of the PLUGIN_EXTENDS mechanism at generate-
# time (meaning a library only gets linked if a set of other libraries are
# *also* linked.) CMake 3.14 or beyond may have such a mechanism, but until
# then, this test is expected to fail, because QMock3Plugin is not being
# linked even though MockPlugins2 is present.
create_test_executable(default_link QMock1Plugin QMock2Plugin QMock3Plugin)
target_link_libraries(default_link Qt5::MockPlugins2)
set_property(TEST test_default_link PROPERTY DISABLED 1)
# No call to qt5_import_plugins() for the default

create_test_executable(manual QMock1Plugin QMock2Plugin QMock3Plugin QMock4Plugin)
qt5_import_plugins(manual
    INCLUDE Qt5::QMock3Plugin Qt5::QMock4Plugin
)

create_test_executable(manual_genex QMock1Plugin QMock2Plugin QMock3Plugin)
qt5_import_plugins(manual_genex
    INCLUDE $<1:Qt5::QMock3Plugin> $<0:Qt5::QMock4Plugin>
)

create_test_executable(blacklist QMock1Plugin)
qt5_import_plugins(blacklist
    EXCLUDE Qt5::QMock2Plugin Qt5::QMock3Plugin
)

create_test_executable(blacklist_genex QMock1Plugin)
qt5_import_plugins(blacklist_genex
    EXCLUDE $<1:Qt5::QMock2Plugin> $<1:Qt5::QMock3Plugin> $<0:Qt5::QMock1Plugin>
)

create_test_executable(override QMock3Plugin QMock4Plugin)
qt5_import_plugins(override
    INCLUDE_BY_TYPE mockplugin Qt5::QMock3Plugin Qt5::QMock4Plugin
)

create_test_executable(override_genex QMock3Plugin)
qt5_import_plugins(override_genex
    INCLUDE_BY_TYPE mockplugin $<1:Qt5::QMock3Plugin> $<0:Qt5::QMock4Plugin>
)

create_test_executable(override_mix QMock2Plugin QMock3Plugin)
qt5_import_plugins(override_mix
    INCLUDE Qt5::QMock2Plugin
    INCLUDE_BY_TYPE mockplugin Qt5::QMock3Plugin
)

if(NOT WIN32)
    # Compiling an empty static array fails on Windows.
    create_test_executable(none)
    qt5_import_plugins(none
        EXCLUDE_BY_TYPE mockplugin
    )
endif()

create_test_executable(none_mix QMock3Plugin QMock4Plugin)
qt5_import_plugins(none_mix
    INCLUDE Qt5::QMock3Plugin Qt5::QMock4Plugin
    EXCLUDE_BY_TYPE mockplugin
)

create_test_executable(recursive QMock5Plugin QMock6Plugin)
qt5_import_plugins(recursive
    INCLUDE_BY_TYPE mockplugin Qt5::QMock5Plugin
)
