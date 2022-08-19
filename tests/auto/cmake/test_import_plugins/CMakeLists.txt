# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(import_plugins_advanced)
enable_testing()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 COMPONENTS REQUIRED MockPlugins1 MockPlugins2)
# MockPlugins3 package is automatically found by the find_dependency call in
# QMock5PluginConfig.cmake which depends on MockPlugins3.
# QMock5Plugin itself is loaded by QtMockPlugins1Plugins.cmake, so via QtMockPlugins1Config.cmake.

function(create_test_executable target)
    cmake_parse_arguments(arg "FINALIZER_MODE;QT_WRAPPER" "" "ADD_EXECUTABLE_ARGS" ${ARGN})

    set(target_name_adjusted "${target}")
    if(arg_FINALIZER_MODE)
        set(target_name_adjusted "${target}_finalizer")
    endif()

    set(CHECK_FILE ${CMAKE_BINARY_DIR}/${target_name_adjusted}_check.cpp)

    set(EXPECTED_PLUGINS)
    foreach(_p ${arg_UNPARSED_ARGUMENTS})
        string(APPEND EXPECTED_PLUGINS "    \"${_p}\",\n")
    endforeach()
    configure_file("${CMAKE_SOURCE_DIR}/check.cpp.in" ${CHECK_FILE})

    set(add_executable_args ${target_name_adjusted} main.cpp ${CHECK_FILE}
        ${arg_ADD_EXECUTABLE_ARGS})

    if(arg_QT_WRAPPER)
        qt_add_executable(${add_executable_args})
    else()
        add_executable(${add_executable_args})
    endif()
    target_link_libraries(${target_name_adjusted} PRIVATE Qt6::MockPlugins1)
    add_test(test_${target_name_adjusted} ${target_name_adjusted})

    if(arg_FINALIZER_MODE)
        set(finalizer_mode_state "ENABLE")
    else()
        set(finalizer_mode_state "DISABLE")
    endif()

    qt_set_finalizer_mode(${target_name_adjusted} ${finalizer_mode_state} MODES static_plugins)
    set(target ${target_name_adjusted} PARENT_SCOPE)
endfunction()

# No call to qt_import_plugins() for the default case.
create_test_executable(default
    QMock1Plugin QMock2Plugin

    QMock3Plugin # TODO: Should not be linked based on .pro file, see QTBUG-93501
    ${import_mode}
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
    ${import_mode}
)
target_link_libraries(${target} PRIVATE Qt6::MockPlugins2)

# Check that both regular and finalizer mode plugin importing pulls in the same set of plugins.
# In regular mode, qt_finalize_target won't execute the finalizer plugin importing, because
# we opt out via qt_set_finalizer_mode(target DISABLE MODES static_plugins).
foreach(import_mode "" "FINALIZER_MODE")
    create_test_executable(manual QMock1Plugin QMock2Plugin QMock3Plugin QMock4Plugin
        ${import_mode})
    qt_import_plugins(${target}
        INCLUDE Qt6::QMock3Plugin Qt6::QMock4Plugin
    )
    qt_finalize_target(${target})

    create_test_executable(manual_genex QMock1Plugin QMock2Plugin QMock3Plugin ${import_mode})
    qt_import_plugins(${target}
        INCLUDE $<1:Qt6::QMock3Plugin> $<0:Qt6::QMock4Plugin>
    )
    qt_finalize_target(${target})

    create_test_executable(blacklist QMock1Plugin ${import_mode})
    qt_import_plugins(${target}
        EXCLUDE Qt6::QMock2Plugin Qt6::QMock3Plugin
    )
    qt_finalize_target(${target})

    create_test_executable(blacklist_genex QMock1Plugin ${import_mode})
    qt_import_plugins(${target}
        EXCLUDE $<1:Qt6::QMock2Plugin> $<1:Qt6::QMock3Plugin> $<0:Qt6::QMock1Plugin>
    )
    qt_finalize_target(${target})

    create_test_executable(override QMock3Plugin QMock4Plugin ${import_mode})
    qt_import_plugins(${target}
        INCLUDE_BY_TYPE mockplugin Qt6::QMock3Plugin Qt6::QMock4Plugin
    )
    qt_finalize_target(${target})

    create_test_executable(override_genex QMock3Plugin ${import_mode})
    qt_import_plugins(${target}
        INCLUDE_BY_TYPE mockplugin $<1:Qt6::QMock3Plugin> $<0:Qt6::QMock4Plugin>
    )
    qt_finalize_target(${target})

    create_test_executable(override_mix QMock2Plugin QMock3Plugin ${import_mode})
    qt_import_plugins(${target}
        INCLUDE Qt6::QMock2Plugin
        INCLUDE_BY_TYPE mockplugin Qt6::QMock3Plugin
    )
    qt_finalize_target(${target})

    if(NOT WIN32)
        # Compiling an empty static array fails on Windows.
        create_test_executable(none ${import_mode})
        qt_import_plugins(${target}
            EXCLUDE_BY_TYPE mockplugin
        )
        qt_finalize_target(${target})
    endif()

    create_test_executable(none_mix QMock3Plugin QMock4Plugin ${import_mode})
    qt_import_plugins(${target}
        INCLUDE Qt6::QMock3Plugin Qt6::QMock4Plugin
        EXCLUDE_BY_TYPE mockplugin
    )
    qt_finalize_target(${target})

    # QMock5Plugin links against the Qt::MockPlugins3 module, which provides the default plugin
    # QMock6Plugin which is why it is pulled in.
    create_test_executable(recursive QMock5Plugin QMock6Plugin ${import_mode})
    qt_import_plugins(${target}
        INCLUDE_BY_TYPE mockplugin Qt6::QMock5Plugin
    )
    qt_finalize_target(${target})
endforeach()

# No call to qt_finalize_target() in finalizer mode means nothing will be
# linked.
if(NOT WIN32)
    # Compiling an empty static array fails on Windows.
    create_test_executable(default_finalizer_missing_import_call FINALIZER_MODE)
endif()

# Empty call to qt_import_plugins() in finalizer mode means default plugins are linked.
create_test_executable(default_finalizer
    QMock1Plugin QMock2Plugin
    QMock3Plugin # TODO: Should not be linked based on .pro file, see QTBUG-93501
    FINALIZER_MODE)
qt_import_plugins(${target})
qt_finalize_target(${target})

# Check that plugin importing with the qt_add_executable wrapper and manual finalization mode works.
create_test_executable(default_qt_add_executable_manually_finalized
    QMock1Plugin QMock2Plugin
    QMock3Plugin # TODO: Should not be linked based on .pro file, see QTBUG-93501
    FINALIZER_MODE QT_WRAPPER
    ADD_EXECUTABLE_ARGS MANUAL_FINALIZATION)
qt_finalize_target(${target})

# Check that plugin importing with automatic deferred finalization works when the CMake version is
# high enough.
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
    create_test_executable(default_qt_add_executable_auto_finalized
        QMock1Plugin QMock2Plugin
        QMock3Plugin # TODO: Should not be linked based on .pro file, see QTBUG-93501
        FINALIZER_MODE QT_WRAPPER)
endif()
