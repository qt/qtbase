# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(_qt_internal_handle_ios_launch_screen target)
    # Check if user provided a launch screen path via a variable.
    set(launch_screen "")

    # Check if the project provided a launch screen path via a variable.
    # This variable is currently in Technical Preview.
    if(QT_IOS_LAUNCH_SCREEN)
        set(launch_screen "${QT_IOS_LAUNCH_SCREEN}")
    endif()

    # Check if the project provided a launch screen path via a target property, it takes precedence
    # over the variable.
    # This property is currently in Technical Preview.
    get_target_property(launch_screen_from_prop "${target}" QT_IOS_LAUNCH_SCREEN)
    if(launch_screen_from_prop)
        set(launch_screen "${launch_screen_from_prop}")
    endif()

    # If the project hasn't provided a launch screen file path, use a copy of the template
    # that qmake uses.
    # It needs to be a copy because configure_file can't handle all the escaped double quotes
    # present in the qmake template file.
    set(is_default_launch_screen FALSE)
    if(NOT launch_screen AND NOT QT_NO_SET_DEFAULT_IOS_LAUNCH_SCREEN)
        set(is_default_launch_screen TRUE)
        set(launch_screen
            "${__qt_internal_cmake_ios_support_files_path}/LaunchScreen.storyboard")
    endif()

    # Check that the launch screen exists.
    if(launch_screen)
        if(NOT IS_ABSOLUTE "${launch_screen}")
            message(FATAL_ERROR
                "Provided launch screen value should be an absolute path: '${launch_screen}'")
        endif()

        if(NOT EXISTS "${launch_screen}")
            message(FATAL_ERROR
                "Provided launch screen file does not exist: '${launch_screen}'")
        endif()
    endif()

    if(launch_screen AND NOT QT_NO_ADD_IOS_LAUNCH_SCREEN_TO_BUNDLE)
        get_filename_component(launch_screen_name "${launch_screen}" NAME)

        # Make a copy of the default launch screen template for this target and replace the
        # label inside the template with the target name.
        if(is_default_launch_screen)
            # Configure our default template and place it in the build dir.
            set(launch_screen_in_path "${launch_screen}")

            string(MAKE_C_IDENTIFIER "${target}" target_identifier)
            set(launch_screen_out_dir
                "${CMAKE_CURRENT_BINARY_DIR}/.qt/launch_screen_storyboards/${target_identifier}")

            set(launch_screen_out_path
                "${launch_screen_out_dir}/${launch_screen_name}")

            file(MAKE_DIRECTORY "${launch_screen_out_dir}")

            # Replaces the value in the default template.
            set(QT_IOS_LAUNCH_SCREEN_TEXT "${target}")
            configure_file(
                "${launch_screen_in_path}"
                "${launch_screen_out_path}"
                @ONLY
            )

            set(final_launch_screen_path "${launch_screen_out_path}")
        else()
            set(final_launch_screen_path "${launch_screen}")
        endif()

        # Add the launch screen storyboard file as a source file, otherwise CMake doesn't consider
        # it as a resource file and MACOSX_PACKAGE_LOCATION processing will be skipped.
        target_sources("${target}" PRIVATE "${final_launch_screen_path}")

        # Ensure Xcode compiles the storyboard file and installs the compiled storyboard .nib files
        # into the app bundle.
        # We use target_sources and the MACOSX_PACKAGE_LOCATION source file property for that
        # instead of the RESOURCE target property, becaues the latter could potentially end up
        # needlessly installing the source storyboard file.
        #
        # We can't rely on policy CMP0118 since user project controls it.
        set(scope_args)
        if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
            set(scope_args TARGET_DIRECTORY ${target})
        endif()
        set_source_files_properties("${final_launch_screen_path}" ${scope_args}
            PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

        # Save the launch screen name, so its value is added as an UILaunchStoryboardName entry
        # in the Qt generated Info.plist file.
        # Xcode expects an Info.plist storyboard entry without an extension.
        get_filename_component(launch_screen_base_name "${launch_screen}" NAME_WE)
        set_target_properties("${target}" PROPERTIES
                              _qt_ios_launch_screen_name "${launch_screen_name}"
                              _qt_ios_launch_screen_base_name "${launch_screen_base_name}"
                              _qt_ios_launch_screen_path "${final_launch_screen_path}")
    endif()
endfunction()

function(_qt_internal_find_ios_development_team_id out_var)
    get_property(team_id GLOBAL PROPERTY _qt_internal_ios_development_team_id)
    get_property(team_id_computed GLOBAL PROPERTY _qt_internal_ios_development_team_id_computed)
    if(team_id_computed)
        # Just in case if the value is non-empty but still booly FALSE.
        if(NOT team_id)
            set(team_id "")
        endif()
        set("${out_var}" "${team_id}" PARENT_SCOPE)
        return()
    endif()

    set_property(GLOBAL PROPERTY _qt_internal_ios_development_team_id_computed "TRUE")

    set(home_dir "$ENV{HOME}")
    set(xcode_preferences_path "${home_dir}/Library/Preferences/com.apple.dt.Xcode.plist")

    # Extract the first account name (email) from the user's Xcode preferences
    message(DEBUG "Trying to extract an Xcode development team id from '${xcode_preferences_path}'")
    execute_process(COMMAND "/usr/libexec/PlistBuddy"
                            -x -c "print IDEProvisioningTeams" "${xcode_preferences_path}"
                    OUTPUT_VARIABLE teams_xml
                    ERROR_VARIABLE plist_error)

    # Parsing state.
    set(is_free "")
    set(current_team_id "")
    set(parsing_is_free FALSE)
    set(parsing_team_id FALSE)
    set(first_team_id "")

    # Parse the xml output and return the first encountered non-free team id. If no non-free team id
    # is found, return the first encountered free team id.
    # If no team is found, return an empty string.
    #
    # Example input:
    #<plist version="1.0">
    #<dict>
    #    <key>marty@planet.local</key>
    #    <array>
    #        <dict>
    #            <key>isFreeProvisioningTeam</key>
    #            <false/>
    #            <key>teamID</key>
    #            <string>AAA</string>
    #            ...
    #        </dict>
    #        <dict>
    #            <key>isFreeProvisioningTeam</key>
    #            <true/>
    #            <key>teamID</key>
    #            <string>BBB</string>
    #            ...
    #        </dict>
    #    </array>
    #</dict>
    #</plist>
    if(teams_xml AND NOT plist_error)
        string(REPLACE "\n" ";" teams_xml_lines "${teams_xml}")

        foreach(xml_line ${teams_xml_lines})
            string(STRIP "${xml_line}" xml_line)
            if(xml_line STREQUAL "<dict>")
                # Clean any previously found values when a new team dict is matched.
                set(is_free "")
                set(current_team_id "")

            elseif(xml_line STREQUAL "<key>isFreeProvisioningTeam</key>")
                set(parsing_is_free TRUE)

            elseif(parsing_is_free)
                set(parsing_is_free FALSE)

                if(xml_line MATCHES "true")
                    set(is_free TRUE)
                else()
                    set(is_free FALSE)
                endif()

            elseif(xml_line STREQUAL "<key>teamID</key>")
                set(parsing_team_id TRUE)

            elseif(parsing_team_id)
                set(parsing_team_id FALSE)
                if(xml_line MATCHES "<string>([^<]+)</string>")
                    set(current_team_id "${CMAKE_MATCH_1}")
                else()
                    continue()
                endif()

                string(STRIP "${current_team_id}" current_team_id)

                # If this is the first team id we found so far, remember that, regardless if's free
                # or not.
                if(NOT first_team_id AND current_team_id)
                    set(first_team_id "${current_team_id}")
                endif()

                # Break early if we found a non-free team id and use it, because we prefer
                # a non-free team for signing, just like qmake.
                if(NOT is_free AND current_team_id)
                    set(first_team_id "${current_team_id}")
                    break()
                endif()
            endif()
        endforeach()
    endif()

    if(NOT first_team_id)
        message(DEBUG "Failed to extract an Xcode development team id.")
        set("${out_var}" "" PARENT_SCOPE)
    else()
        message(DEBUG "Successfully extracted the first encountered Xcode development team id.")
        set_property(GLOBAL PROPERTY _qt_internal_ios_development_team_id "${first_team_id}")
        set("${out_var}" "${first_team_id}" PARENT_SCOPE)
    endif()
endfunction()

function(_qt_internal_get_apple_bundle_identifier_prefix out_var)
    get_property(prefix GLOBAL PROPERTY _qt_internal_ios_bundle_identifier_prefix)
    get_property(prefix_computed GLOBAL PROPERTY
                 _qt_internal_ios_bundle_identifier_prefix_computed)
    if(prefix_computed)
        # Just in case if the value is non-empty but still booly FALSE.
        if(NOT prefix)
            set(prefix "")
        endif()
        set("${out_var}" "${prefix}" PARENT_SCOPE)
        return()
    endif()

    set_property(GLOBAL PROPERTY _qt_internal_ios_bundle_identifier_prefix_computed "TRUE")

    set(home_dir "$ENV{HOME}")
    set(xcode_preferences_path "${home_dir}/Library/Preferences/com.apple.dt.Xcode.plist")

    message(DEBUG "Trying to extract the default bundle identifier prefix from Xcode preferences.")
    execute_process(COMMAND "/usr/libexec/PlistBuddy"
                            -c "print IDETemplateOptions:bundleIdentifierPrefix"
                            "${xcode_preferences_path}"
                    OUTPUT_VARIABLE prefix
                    ERROR_VARIABLE prefix_error)
    if(prefix AND NOT prefix_error)
        message(DEBUG "Successfully extracted the default bundle identifier prefix.")
        string(STRIP "${prefix}" prefix)
    else()
        message(DEBUG "Failed to extract the default bundle identifier prefix.")
    endif()

    if(prefix AND NOT prefix_error)
        set_property(GLOBAL PROPERTY _qt_internal_ios_bundle_identifier_prefix "${prefix}")
        set("${out_var}" "${prefix}" PARENT_SCOPE)
    else()
        set("${out_var}" "" PARENT_SCOPE)
    endif()
endfunction()

function(_qt_internal_escape_rfc_1034_identifier value out_var)
    # According to https://datatracker.ietf.org/doc/html/rfc1034#section-3.5
    # we can only use letters, digits, dot (.) and hyphens (-).
    # Underscores are not allowed.
    string(REGEX REPLACE "[^A-Za-z0-9.]" "-" value "${value}")

    set("${out_var}" "${value}" PARENT_SCOPE)
endfunction()

function(_qt_internal_get_default_apple_bundle_identifier target out_var)
    _qt_internal_get_apple_bundle_identifier_prefix(prefix)
    if(NOT prefix)
        set(prefix "com.yourcompany")

        # For a better out-of-the-box experience, try to create a unique prefix by appending
        # the sha1 of the team id, if one is found.
        _qt_internal_find_ios_development_team_id(team_id)
        if(team_id)
            string(SHA1 hash "${team_id}")
            string(SUBSTRING "${hash}" 0 8 infix)
            string(APPEND prefix ".${infix}")
        endif()

        if(CMAKE_GENERATOR STREQUAL "Xcode")
            message(WARNING
                "No organization bundle identifier prefix could be retrieved from Xcode preferences. \
                This can lead to code signing issues due to a non-unique bundle \
                identifier. Please set up an organization prefix by creating a new project within \
                Xcode, or consider providing a custom bundle identifier by specifying the \
                MACOSX_BUNDLE_GUI_IDENTIFIER or XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER property."
                )
        endif()
    endif()

    # Escape the prefix according to rfc 1034, it's important for code-signing. If an invalid
    # identifier is used, calling xcodebuild on the command line says that no provisioning profile
    # could be found, with no additional error message. If one opens the generated project with
    # Xcode and clicks on 'Try again' to get a new profile, it shows a semi-useful error message
    # that the identifier is invalid.
    _qt_internal_escape_rfc_1034_identifier("${prefix}" prefix)

    if(CMAKE_GENERATOR STREQUAL "Xcode")
        set(identifier "${prefix}.$(PRODUCT_NAME:rfc1034identifier)")
    else()
        set(identifier "${prefix}.${target}")
    endif()

    set("${out_var}" "${identifier}" PARENT_SCOPE)
endfunction()

function(_qt_internal_set_placeholder_apple_bundle_version target)
    # If user hasn't provided neither a bundle version nor a bundle short version string for the
    # app, set a placeholder value for both which will add them to the generated Info.plist file.
    # This is required so that the app launches in the simulator (but apparently not for running
    # on-device).
    get_target_property(bundle_version "${target}" MACOSX_BUNDLE_BUNDLE_VERSION)
    get_target_property(bundle_short_version "${target}" MACOSX_BUNDLE_SHORT_VERSION_STRING)

    if(NOT MACOSX_BUNDLE_BUNDLE_VERSION AND
       NOT MACOSX_BUNDLE_SHORT_VERSION_STRING AND
       NOT bundle_version AND
       NOT bundle_short_version AND
       NOT QT_NO_SET_XCODE_BUNDLE_VERSION
    )
        get_target_property(version "${target}" VERSION)
        if(NOT version)
            set(version "${PROJECT_VERSION}")
            if(NOT version)
                set(version "1.0.0")
            endif()
        endif()

        # Use x.y for short version and x.y.z for full version
        # Any versions longer than this will fail App Store
        # submission.
        string(REPLACE "." ";" version_list ${version})
        list(LENGTH version_list version_list_length)
        list(GET version_list 0 version_major)
        set(bundle_short_version "${version_major}")
        if(version_list_length GREATER 1)
            list(GET version_list 1 version_minor)
            string(APPEND bundle_short_version ".${version_minor}")
        endif()
        set(bundle_version "${bundle_short_version}")
        if(version_list_length GREATER 2)
            list(GET version_list 2 version_patch)
            string(APPEND bundle_version ".${version_patch}")
        endif()


        if(NOT CMAKE_XCODE_ATTRIBUTE_MARKETING_VERSION
            AND NOT QT_NO_SET_XCODE_ATTRIBUTE_MARKETING_VERSION
            AND NOT CMAKE_XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION
            AND NOT QT_NO_SET_XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION
            AND CMAKE_GENERATOR STREQUAL "Xcode")
            get_target_property(marketing_version "${target}"
                XCODE_ATTRIBUTE_MARKETING_VERSION)
            get_target_property(current_project_version "${target}"
                XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION)
            if(NOT marketing_version AND NOT current_project_version)
                set_target_properties("${target}"
                    PROPERTIES
                        XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION "${bundle_version}"
                        XCODE_ATTRIBUTE_MARKETING_VERSION "${bundle_short_version}"
                )
                set(bundle_version "$(CURRENT_PROJECT_VERSION)")
                set(bundle_short_version "$(MARKETING_VERSION)")
            endif()
        endif()

        set_target_properties("${target}"
                               PROPERTIES
                               MACOSX_BUNDLE_BUNDLE_VERSION "${bundle_version}"
                               MACOSX_BUNDLE_SHORT_VERSION_STRING "${bundle_short_version}"
                               )
    endif()
endfunction()

function(_qt_internal_set_xcode_development_team_id target)
    # If user hasn't provided a development team id, try to find the first one specified
    # in the Xcode preferences.
    if(NOT CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM AND NOT QT_NO_SET_XCODE_DEVELOPMENT_TEAM_ID)
        get_target_property(existing_team_id "${target}" XCODE_ATTRIBUTE_DEVELOPMENT_TEAM)
        if(NOT existing_team_id)
            _qt_internal_find_ios_development_team_id(team_id)
            set_target_properties("${target}"
                                  PROPERTIES XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${team_id}")
        endif()
    endif()
endfunction()

function(_qt_internal_set_apple_bundle_identifier target)
    # Skip all logic if requested.
    if(QT_NO_SET_XCODE_BUNDLE_IDENTIFIER)
        return()
    endif()

    # There are two fields to consider: the CFBundleIdentifier key (ie., cmake_bundle_identifier)
    # to be written to Info.plist and the PRODUCT_BUNDLE_IDENTIFIER (ie., xcode_bundle_identifier)
    # property to set in the Xcode project. The `cmake_bundle_identifier` set by
    # MACOSX_BUNDLE_GUI_IDENTIFIER applies to both Xcode, and other generators, while
    # `xcode_bundle_identifier` set by XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER is
    # Xcode specific.
    #
    # If Ninja is the generator, we set the value of `MACOSX_BUNDLE_GUI_IDENTIFIER`
    # and don't touch the `XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER`.
    # If Xcode is the generator, we set the value of `XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER`,
    # and additionally, to silence a Xcode's warning, we set the `MACOSX_BUNDLE_GUI_IDENTIFIER` to
    # `${PRODUCT_BUNDLE_IDENTIFIER}` so that Xcode could sort it out.

    get_target_property(existing_cmake_bundle_identifier "${target}"
                        MACOSX_BUNDLE_GUI_IDENTIFIER)
    get_target_property(existing_xcode_bundle_identifier "${target}"
                        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER)

    set(is_cmake_bundle_identifier_given FALSE)
    if(existing_cmake_bundle_identifier)
        set(is_cmake_bundle_identifier_given TRUE)
    elseif(MACOSX_BUNDLE_GUI_IDENTIFIER)
        set(is_cmake_bundle_identifier_given TRUE)
        set(existing_cmake_bundle_identifier ${MACOSX_BUNDLE_GUI_IDENTIFIER})
    endif()

    set(is_xcode_bundle_identifier_given FALSE)
    if(existing_xcode_bundle_identifier)
        set(is_xcode_bundle_identifier_given TRUE)
    elseif(CMAKE_XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER)
        set(is_xcode_bundle_identifier_given TRUE)
        set(existing_xcode_bundle_identifier ${CMAKE_XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER})
    endif()

    if(is_cmake_bundle_identifier_given
        AND is_xcode_bundle_identifier_given
            AND NOT existing_cmake_bundle_identifier STREQUAL existing_xcode_bundle_identifier)
        message(WARNING
            "MACOSX_BUNDLE_GUI_IDENTIFIER and XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "
            "are set to different values. You only need to set one of them. ")
    endif()

    if(NOT is_xcode_bundle_identifier_given
        AND NOT is_cmake_bundle_identifier_given)
        _qt_internal_get_default_apple_bundle_identifier("${target}" bundle_id)
    elseif(is_cmake_bundle_identifier_given)
        set(bundle_id ${existing_cmake_bundle_identifier})
    elseif(is_xcode_bundle_identifier_given)
        set(bundle_id ${existing_xcode_bundle_identifier})
    endif()

    if(CMAKE_GENERATOR STREQUAL "Xcode")
        set_target_properties("${target}"
                              PROPERTIES
                              XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${bundle_id}"
                              MACOSX_BUNDLE_GUI_IDENTIFIER "$(PRODUCT_BUNDLE_IDENTIFIER)")
    else()
        set_target_properties("${target}"
                              PROPERTIES
                              MACOSX_BUNDLE_GUI_IDENTIFIER "${bundle_id}")
    endif()
endfunction()

function(_qt_internal_set_xcode_targeted_device_family target)
    if(NOT CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY
            AND NOT QT_NO_SET_XCODE_TARGETED_DEVICE_FAMILY)
        get_target_property(existing_device_family
            "${target}" XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY)
        if(NOT existing_device_family)
            set(device_family_iphone_and_ipad "1,2")
            set_target_properties("${target}"
                                  PROPERTIES
                                  XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY
                                  "${device_family_iphone_and_ipad}")
        endif()
    endif()
endfunction()

function(_qt_internal_set_xcode_code_sign_style target)
    if(NOT CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE
            AND NOT QT_NO_SET_XCODE_CODE_SIGN_STYLE)
        get_target_property(existing_code_style
            "${target}" XCODE_ATTRIBUTE_CODE_SIGN_STYLE)
        if(NOT existing_code_style)
            set(existing_code_style "Automatic")
            set_target_properties("${target}"
                                  PROPERTIES
                                  XCODE_ATTRIBUTE_CODE_SIGN_STYLE
                                  "${existing_code_style}")
        endif()
    endif()
endfunction()

# Workaround for https://gitlab.kitware.com/cmake/cmake/-/issues/15183
function(_qt_internal_set_xcode_install_path target)
    if(NOT CMAKE_XCODE_ATTRIBUTE_INSTALL_PATH
            AND NOT QT_NO_SET_XCODE_INSTALL_PATH)
        get_target_property(existing_install_path
            "${target}" XCODE_ATTRIBUTE_INSTALL_PATH)
        if(NOT existing_install_path)
            set_target_properties("${target}"
                                  PROPERTIES
                                  XCODE_ATTRIBUTE_INSTALL_PATH
                                  "$(inherited)")
        endif()
    endif()
endfunction()

function(_qt_internal_set_xcode_bundle_display_name target)
    # We want the value of CFBundleDisplayName to be ${PRODUCT_NAME}, but we can't put that
    # into the Info.plist.in template file directly, because the implicit configure_file(Info.plist)
    # done by CMake is not using the @ONLY option, so CMake would treat the assignment as
    # variable expansion. Escaping using backslashes does not help.
    # Work around it by assigning the dollar char to a separate cache var, and expand it, so that
    # the final value in the file will be ${PRODUCT_NAME}, to be evaluated at build time by Xcode.
    set(QT_INTERNAL_DOLLAR_VAR "$" CACHE STRING "")
endfunction()

# Adds ${PRODUCT_NAME} to the Info.plist file, which is then evaluated by Xcode itself.
function(_qt_internal_set_xcode_bundle_name target)
    if(QT_NO_SET_XCODE_BUNDLE_NAME)
        return()
    endif()

    get_target_property(existing_bundle_name "${target}" MACOSX_BUNDLE_BUNDLE_NAME)
    if(NOT MACOSX_BUNDLE_BUNDLE_NAME AND NOT existing_bundle_name)
        if(CMAKE_GENERATOR STREQUAL Xcode)
            set_target_properties("${target}"
                                  PROPERTIES
                                  MACOSX_BUNDLE_BUNDLE_NAME "$(PRODUCT_NAME)")
        else()
            set_target_properties("${target}"
                                  PROPERTIES
                                  MACOSX_BUNDLE_BUNDLE_NAME "${target}")
        endif()
    endif()
endfunction()

function(_qt_internal_set_xcode_bitcode_enablement target)
    if(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE
        OR QT_NO_SET_XCODE_ENABLE_BITCODE)
        return()
    endif()

    get_target_property(existing_bitcode_enablement
        "${target}" XCODE_ATTRIBUTE_ENABLE_BITCODE)
    if(NOT existing_bitcode_enablement MATCHES "-NOTFOUND")
        return()
    endif()

    # Disable bitcode to match Xcode 14's new default
    set_target_properties("${target}"
        PROPERTIES
        XCODE_ATTRIBUTE_ENABLE_BITCODE
        "NO")
endfunction()

function(_qt_internal_generate_ios_info_plist target)
    # If the project already specifies a custom file, we don't override it.
    get_target_property(existing_plist "${target}" MACOSX_BUNDLE_INFO_PLIST)
    if(existing_plist)
        return()
    endif()

    set(info_plist_in "${__qt_internal_cmake_ios_support_files_path}/Info.plist.app.in")

    string(MAKE_C_IDENTIFIER "${target}" target_identifier)
    set(info_plist_out_dir
        "${CMAKE_CURRENT_BINARY_DIR}/.qt/info_plist/${target_identifier}")
    set(info_plist_out "${info_plist_out_dir}/Info.plist")

    # Check if we need to specify a custom launch screen storyboard entry.
    get_target_property(launch_screen_base_name "${target}" _qt_ios_launch_screen_base_name)
    if(launch_screen_base_name)
        set(qt_ios_launch_screen_plist_entry "${launch_screen_base_name}")
    endif()

    # Call configure_file to substitute Qt-specific @FOO@ values, not ${FOO} values.
    #
    # The output file will be another template file to be fed to CMake via the
    # MACOSX_BUNDLE_INFO_PLIST property. CMake will then call configure_file on it to provide
    # content for regular entries like CFBundleName, etc.
    #
    # We require this extra configure_file call so we can create unique Info.plist files for each
    # target in a project, while also providing a way to add Qt specific entries that CMake
    # does not support out of the box (e.g. a launch screen name).
    configure_file(
        "${info_plist_in}"
        "${info_plist_out}"
        @ONLY
    )

    set_target_properties("${target}" PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${info_plist_out}")
endfunction()

function(_qt_internal_set_ios_simulator_arch target)
    if(CMAKE_XCODE_ATTRIBUTE_ARCHS
        OR QT_NO_SET_XCODE_ARCHS)
        return()
    endif()

    get_target_property(existing_archs
        "${target}" XCODE_ATTRIBUTE_ARCHS)
    if(NOT existing_archs MATCHES "-NOTFOUND")
        return()
    endif()

    if(NOT x86_64 IN_LIST QT_OSX_ARCHITECTURES)
        return()
    endif()

    if(CMAKE_OSX_ARCHITECTURES AND NOT x86_64 IN_LIST CMAKE_OSX_ARCHITECTURES)
        return()
    endif()

    set_target_properties("${target}"
        PROPERTIES
        "XCODE_ATTRIBUTE_ARCHS[sdk=iphonesimulator*]"
        "x86_64")
endfunction()

function(_qt_internal_finalize_apple_app target)
    # Shared between macOS and iOS apps

    # Only set the various properties if targeting the Xcode generator, otherwise the various
    # Xcode tokens are embedded as-is instead of being dynamically evaluated.
    # This affects things like the version number or application name as reported by Qt API.
    if(CMAKE_GENERATOR STREQUAL "Xcode")
        _qt_internal_set_xcode_development_team_id("${target}")
        _qt_internal_set_xcode_code_sign_style("${target}")
        _qt_internal_set_xcode_bundle_display_name("${target}")
        _qt_internal_set_xcode_install_path("${target}")
    endif()

    _qt_internal_set_xcode_bundle_name("${target}")
    _qt_internal_set_apple_bundle_identifier("${target}")
    _qt_internal_set_placeholder_apple_bundle_version("${target}")
endfunction()

function(_qt_internal_finalize_ios_app target)
    _qt_internal_finalize_apple_app("${target}")

    _qt_internal_set_xcode_targeted_device_family("${target}")
    _qt_internal_set_xcode_bitcode_enablement("${target}")
    _qt_internal_handle_ios_launch_screen("${target}")
    _qt_internal_generate_ios_info_plist("${target}")
    _qt_internal_set_ios_simulator_arch("${target}")
endfunction()

function(_qt_internal_finalize_macos_app target)
    get_target_property(is_bundle ${target} MACOSX_BUNDLE)
    if(NOT is_bundle)
        return()
    endif()

    _qt_internal_finalize_apple_app("${target}")

    # Make sure the install rpath has at least the minimum needed if the app
    # has any non-static frameworks. We can't rigorously know if the app will
    # have any, even with a static Qt, so always add this. If there are no
    # frameworks, it won't do any harm.
    get_property(install_rpath TARGET ${target} PROPERTY INSTALL_RPATH)
    list(APPEND install_rpath "@executable_path/../Frameworks")
    list(REMOVE_DUPLICATES install_rpath)
    set_property(TARGET ${target} PROPERTY INSTALL_RPATH "${install_rpath}")
endfunction()
