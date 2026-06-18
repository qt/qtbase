# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(deployment_api)
enable_testing()

find_package(Qt6 COMPONENTS REQUIRED Widgets Test)

qt6_standard_project_setup()

function(create_test_executable target)
    cmake_parse_arguments(arg "" "" "" ${ARGN})

    if(CMAKE_VERSION VERSION_LESS "3.19")
        qt_add_executable(${target} MANUAL_FINALIZATION main.cpp)
    else()
        qt_add_executable(${target} main.cpp)
    endif()

    set_target_properties(${target} PROPERTIES
        # We explicitly don't set WIN32_EXECUTABLE to ensure we see errors from stderr when
        # something fails and not having to use DebugView.

        MACOSX_BUNDLE TRUE
    )
    target_link_libraries(${target} PRIVATE Qt::Widgets Qt::Test)

    install(TARGETS ${target}
        BUNDLE  DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    qt_generate_deploy_app_script(
        TARGET ${target}
        OUTPUT_SCRIPT deploy_script
        # Don't fail at configure time on unsupported platforms
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})

    if(CMAKE_VERSION VERSION_LESS "3.19")
        qt_finalize_target(${target})
    endif()

    if(APPLE AND NOT IOS)
        set(installed_app_location "${CMAKE_INSTALL_PREFIX}/${target}.app/Contents/MacOS/${target}")
    elseif(WIN32)
        set(installed_app_location "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/${target}.exe")
    elseif(UNIX AND NOT APPLE AND NOT ANDROID AND NOT CMAKE_CROSSCOMPILING)
        set(installed_app_location "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/${target}")
    endif()

    # There's no nice way to get the location of an installed binary, so we need to construct
    # the binary install path by hand, somewhat similar to how it's done in
    # the implementation of qt_deploy_runtime_dependencies.
    # On unsupported deployment platforms, either the install_ test will fail not finding
    # the location of the app (because we do not set a installed_app_location value)
    # or the run_deployed_ test will fail because we didn't deploy the runtime dependencies.
    # When support for additional platforms is added, these locations will have to be augmented.
    add_test(install_${target} "${CMAKE_COMMAND}" --install .)
    set_tests_properties(install_${target} PROPERTIES FIXTURES_SETUP deploy_step)
    add_test(NAME run_deployed_${target}
             COMMAND "${installed_app_location}"
             # Make sure that we don't use the default working directory which is
             # CMAKE_CURRENT_BINARY_DIR because on Windows the loader might pick up dlls
             # from the working directory instead of the installed app dir, if the dll is
             # missing in the app dir.
             WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}")
    set_tests_properties(run_deployed_${target} PROPERTIES FIXTURES_REQUIRED deploy_step)

    # Also test deployment with a relative install prefix given at install time
    # (QTBUG-147578). The prefix is resolved relative to the working directory
    # of the cmake --install invocation, so install into a subdirectory of the
    # build directory, which is also the test's working directory.
    set(rel_prefix "relative_install_prefix")
    set(rel_prefix_abs "${CMAKE_CURRENT_BINARY_DIR}/${rel_prefix}")
    if(APPLE AND NOT IOS)
        set(rel_installed_app_location "${rel_prefix_abs}/${target}.app/Contents/MacOS/${target}")
    elseif(WIN32)
        set(rel_installed_app_location "${rel_prefix_abs}/${CMAKE_INSTALL_BINDIR}/${target}.exe")
    elseif(UNIX AND NOT APPLE AND NOT ANDROID AND NOT CMAKE_CROSSCOMPILING)
        set(rel_installed_app_location "${rel_prefix_abs}/${CMAKE_INSTALL_BINDIR}/${target}")
    endif()

    add_test(NAME install_rel_${target}
             COMMAND "${CMAKE_COMMAND}" --install . --prefix "${rel_prefix}"
             WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    set_tests_properties(install_rel_${target} PROPERTIES FIXTURES_SETUP deploy_rel_step)
    add_test(NAME run_deployed_rel_${target}
             COMMAND "${rel_installed_app_location}"
             WORKING_DIRECTORY "${rel_prefix_abs}")
    set_tests_properties(run_deployed_rel_${target} PROPERTIES FIXTURES_REQUIRED deploy_rel_step)
endfunction()

create_test_executable(App)

