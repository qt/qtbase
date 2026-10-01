# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Creates an app from main.cpp in the calling directory that is deployed with DEPLOY_OPTIONS,
# and tests the deployed app. LIBRARY is a shared library target that the app links to,
# which has to be installed explicitly.
function(create_test_executable target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "LIBRARY" "DEPLOY_OPTIONS")

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
    target_link_libraries(${target} PRIVATE Qt::Test ${arg_LIBRARY})

    install(TARGETS ${target}
        BUNDLE  DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    if(arg_LIBRARY)
        # Install the library explicitly, because macdeployqt/winddeployqt don't take care of
        # installing non-Qt dependencies.
        if(APPLE)
            install(TARGETS ${arg_LIBRARY}
                LIBRARY DESTINATION "${target}.app/Contents/Frameworks"
            )
        elseif(WIN32)
            # windeployqt doesn't take care of installing non-Qt dependencies.
            install(TARGETS ${arg_LIBRARY}
                RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            )
        elseif(UNIX AND NOT QT6_IS_SHARED_LIBS_BUILD)
            # In a static build, GRD won't run, and we need to install the DSO manually.
            install(TARGETS ${arg_LIBRARY}
                LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            )
        endif()
    endif()

    if(NOT arg_LIBRARY OR NOT UNIX OR APPLE)
        # Explicitly link against Qt::Gui, because otherwise, macdeployqt/windeployqt don't deploy
        # the Gui module. The apps without a library use Gui directly.
        target_link_libraries(${target} PRIVATE Qt::Gui)
    endif()

    qt_generate_deploy_app_script(
        TARGET ${target}
        OUTPUT_SCRIPT deploy_script
        # Don't fail at configure time on unsupported platforms
        NO_UNSUPPORTED_PLATFORM_ERROR
        ${arg_DEPLOY_OPTIONS}
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

    # The deployment tools don't remove files from an earlier install, so start from an
    # empty install prefix. Only do this for the prefix that the test harness creates.
    if(CMAKE_INSTALL_PREFIX MATCHES "_installed$")
        add_test(clean_${target} "${CMAKE_COMMAND}" -E rm -rf "${CMAKE_INSTALL_PREFIX}")
        set_tests_properties(clean_${target} PROPERTIES FIXTURES_SETUP clean_step)
    endif()

    # There's no nice way to get the location of an installed binary, so we need to construct
    # the binary install path by hand, somewhat similar to how it's done in
    # the implementation of qt_deploy_runtime_dependencies.
    # On unsupported deployment platforms, either the install_ test will fail not finding
    # the location of the app (because we do not set a installed_app_location value)
    # or the run_deployed_ test will fail because we didn't deploy the runtime dependencies.
    # When support for additional platforms is added, these locations will have to be augmented.
    add_test(install_${target} "${CMAKE_COMMAND}" --install .)
    set_tests_properties(install_${target} PROPERTIES
        FIXTURES_SETUP deploy_step
        FIXTURES_REQUIRED clean_step
    )
    add_test(NAME run_deployed_${target}
             COMMAND "${installed_app_location}"
             # Make sure that we don't use the default working directory which is
             # CMAKE_CURRENT_BINARY_DIR because on Windows the loader might pick up dlls
             # from the working directory instead of the installed app dir, if the dll is
             # missing in the app dir.
             WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}")
    set_tests_properties(run_deployed_${target} PROPERTIES FIXTURES_REQUIRED deploy_step)
endfunction()
