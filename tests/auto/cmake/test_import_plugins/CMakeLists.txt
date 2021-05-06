cmake_minimum_required(VERSION 3.1)

project(import_plugins_advanced)
enable_testing()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 COMPONENTS REQUIRED MockPlugins1 MockPlugins2)
# MockPlugins3 is automatically find_dependency'd by QMock5Plugin which depends on MockPlugins3.
# QMock5Plugin itself is loaded by QtMockPlugins1Plugins.cmake.

function(create_test_executable TARGET_NAME)
    set(CHECK_FILE ${CMAKE_BINARY_DIR}/${TARGET_NAME}_check.cpp)

    set(EXPECTED_PLUGINS)
    foreach(_p ${ARGN})
        string(APPEND EXPECTED_PLUGINS "    \"${_p}\",\n")
    endforeach()
    configure_file("${CMAKE_SOURCE_DIR}/check.cpp.in" ${CHECK_FILE})

    add_executable(${TARGET_NAME} main.cpp ${CHECK_FILE})
    target_link_libraries(${TARGET_NAME} PRIVATE Qt6::MockPlugins1)
    add_test(test_${TARGET_NAME} ${TARGET_NAME})
endfunction()

# No call to qt_import_plugins() for the default case.
create_test_executable(default
    QMock1Plugin QMock2Plugin

    QMock3Plugin # TODO: Should not be linked based on .pro file, see QTBUG-93501
)

# No call to qt_import_plugins() for the default_link case.
create_test_executable(default_link QMock1Plugin QMock2Plugin

    # TODO: in qmake QMock3Plugin should only be linked if the executable depends on MockPlugins2
    #       module (based on .pro file PLUGIN_EXTENDS). Here it's accidentally linked because
    #       we're missing PLUGIN_EXTENDS information in CMake land. Thus it's considered
    #       a default plugin which is linked regardless of whether MockPlugins2 is linked.
    #       It's possible the qmake behavior is also wrong, because the qmake qt5 test seems to
    #       expect to link the plugin if both MockPlugins1 AND MockPlugins2 are linked, but qt.pf
    #       suggests that MockPlugins1 OR MockPlugins2 is sufficient to link the plugin, not both.
    #       See QTBUG-93501
    QMock3Plugin
)
target_link_libraries(default_link PRIVATE Qt6::MockPlugins2)

create_test_executable(manual QMock1Plugin QMock2Plugin QMock3Plugin QMock4Plugin)
qt_import_plugins(manual
    INCLUDE Qt6::QMock3Plugin Qt6::QMock4Plugin
)

create_test_executable(manual_genex QMock1Plugin QMock2Plugin QMock3Plugin)
qt_import_plugins(manual_genex
    INCLUDE $<1:Qt6::QMock3Plugin> $<0:Qt6::QMock4Plugin>
)

create_test_executable(blacklist QMock1Plugin)
qt_import_plugins(blacklist
    EXCLUDE Qt6::QMock2Plugin Qt6::QMock3Plugin
)

create_test_executable(blacklist_genex QMock1Plugin)
qt_import_plugins(blacklist_genex
    EXCLUDE $<1:Qt6::QMock2Plugin> $<1:Qt6::QMock3Plugin> $<0:Qt6::QMock1Plugin>
)

create_test_executable(override QMock3Plugin QMock4Plugin)
qt_import_plugins(override
    INCLUDE_BY_TYPE mockplugin Qt6::QMock3Plugin Qt6::QMock4Plugin
)

create_test_executable(override_genex QMock3Plugin)
qt_import_plugins(override_genex
    INCLUDE_BY_TYPE mockplugin $<1:Qt6::QMock3Plugin> $<0:Qt6::QMock4Plugin>
)
get_target_property(prop_plugs override_genex QT_PLUGINS)
get_target_property(prop_types override_genex QT_PLUGINS_mockplugin)

create_test_executable(override_mix QMock2Plugin QMock3Plugin)
qt_import_plugins(override_mix
    INCLUDE Qt6::QMock2Plugin
    INCLUDE_BY_TYPE mockplugin Qt6::QMock3Plugin
)

if(NOT WIN32)
    # Compiling an empty static array fails on Windows.
    create_test_executable(none)
    qt_import_plugins(none
        EXCLUDE_BY_TYPE mockplugin
    )
endif()

create_test_executable(none_mix QMock3Plugin QMock4Plugin)
qt_import_plugins(none_mix
    INCLUDE Qt6::QMock3Plugin Qt6::QMock4Plugin
    EXCLUDE_BY_TYPE mockplugin
)

# QMock5Plugin links against the Qt::MockPlugins3 module, which provides the default plugin
# QMock6Plugin which is why it is pulled in.
create_test_executable(recursive QMock5Plugin QMock6Plugin)
qt_import_plugins(recursive
    INCLUDE_BY_TYPE mockplugin Qt6::QMock5Plugin
)
