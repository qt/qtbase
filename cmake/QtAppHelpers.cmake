# This function creates a CMake target for a Qt internal app.
# Such projects had a load(qt_app) command.
function(qt_internal_add_app target)
    qt_parse_all_arguments(arg
        "qt_internal_add_app"
        "NO_INSTALL;INSTALL_VERSIONED_LINK"
        "${__default_target_info_args}"
        "${__default_private_args}"
        ${ARGN})

    set(output_directory "${QT_BUILD_DIR}/${INSTALL_BINDIR}")

    set(no_install "")
    if(arg_NO_INSTALL)
        set(no_install NO_INSTALL)
    endif()

    qt_internal_add_executable("${target}"
        QT_APP
        DELAY_RC
        DELAY_TARGET_INFO
        OUTPUT_DIRECTORY "${output_directory}"
        ${no_install}
        SOURCES ${arg_SOURCES}
        INCLUDE_DIRECTORIES
            ${arg_INCLUDE_DIRECTORIES}
        DEFINES
            ${arg_DEFINES}
        LIBRARIES ${arg_LIBRARIES} Qt::PlatformAppInternal
        COMPILE_OPTIONS ${arg_COMPILE_OPTIONS}
        LINK_OPTIONS ${arg_LINK_OPTIONS}
        MOC_OPTIONS ${arg_MOC_OPTIONS}
        ENABLE_AUTOGEN_TOOLS ${arg_ENABLE_AUTOGEN_TOOLS}
        DISABLE_AUTOGEN_TOOLS ${arg_DISABLE_AUTOGEN_TOOLS}
        TARGET_VERSION "${arg_TARGET_VERSION}"
        TARGET_PRODUCT "${arg_TARGET_PRODUCT}"
        TARGET_DESCRIPTION "${arg_TARGET_DESCRIPTION}"
        TARGET_COMPANY "${arg_TARGET_COMPANY}"
        TARGET_COPYRIGHT "${arg_TARGET_COPYRIGHT}"
    )
    qt_internal_add_target_aliases("${target}")
    _qt_internal_apply_strict_cpp("${target}")

    # To mimic the default behaviors of qt_app.prf, we by default enable GUI Windows applications,
    # but don't enable macOS bundles.
    # Bundles are enabled in a separate set_target_properties call if an Info.plist file
    # is provided.
    # Similary, the Windows GUI flag is disabled in a separate call
    # if CONFIG += console was encountered during conversion.
    set_target_properties("${target}" PROPERTIES WIN32_EXECUTABLE TRUE)

    # Consider every app as user facing tool.
    set_property(GLOBAL APPEND PROPERTY QT_USER_FACING_TOOL_TARGETS ${target})

    # Install versioned link if requested.
    if(NOT arg_NO_INSTALL AND arg_INSTALL_VERSIONED_LINK)
        qt_internal_install_versioned_link("${INSTALL_BINDIR}" ${target})
    endif()

    qt_add_list_file_finalizer(qt_internal_finalize_app ${target})
endfunction()

function(qt_internal_get_title_case value out_var)
    if(NOT value)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()
    string(SUBSTRING "${value}" 0 1 first_char)
    string(TOUPPER "${first_char}" first_char_upper)
    string(SUBSTRING "${target}" 1 -1 rest_of_value)
    set(title_value "${first_char_upper}${rest_of_value}")
    set(${out_var} "${title_value}" PARENT_SCOPE)
endfunction()

function(qt_internal_update_app_target_info_properties target)
    # First update the delayed properties with any values that might have been set after the
    # qt_internal_add_app() call.
    qt_internal_update_delayed_target_info_properties(${target})

    # Set defaults in case if no values were set.
    get_target_property(target_version ${target} QT_DELAYED_TARGET_VERSION)
    if(NOT target_version)
        set_target_properties(${target} PROPERTIES QT_DELAYED_TARGET_VERSION "${PROJECT_VERSION}")
    endif()

    get_target_property(target_description ${target} QT_DELAYED_TARGET_DESCRIPTION)
    if(NOT target_description)
        qt_internal_get_title_case("${target}" upper_name)
        set_target_properties(${target} PROPERTIES QT_DELAYED_TARGET_DESCRIPTION "Qt ${upper_name}")
    endif()

    # Finally set the final values.
    qt_internal_set_target_info_properties_from_delayed_properties("${target}")
endfunction()

function(qt_internal_finalize_app target)
    qt_internal_update_app_target_info_properties("${target}")

    if(WIN32)
        _qt_internal_generate_win32_rc_file("${target}")
    endif()

    # Rpaths need to be applied in the finalizer, because the MACOSX_BUNDLE property might be
    # set after a qt_internal_add_app call.
    qt_apply_rpaths(TARGET "${target}" INSTALL_PATH "${INSTALL_BINDIR}" RELATIVE_RPATH)
endfunction()
