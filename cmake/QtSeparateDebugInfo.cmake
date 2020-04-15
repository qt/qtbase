include(CMakeFindBinUtils)

if(CMAKE_VERSION VERSION_LESS 3.17.0)
set(CMAKE_CURRENT_FUNCTION_LIST_DIR ${CMAKE_CURRENT_LIST_DIR})
endif()

# Enable separate debug information for the given target
function(qt_enable_separate_debug_info target installDestination)
    unset(commands)
    if(APPLE)
        find_program(DSYMUTIL_PROGRAM dsymutil)
        set(copy_bin ${DSYMUTIL_PROGRAM})
        set(strip_bin ${CMAKE_STRIP})
        set(debug_info_suffix dSYM)
        set(copy_bin_out_arg --flat -o)
        set(strip_args -S)
    else()
        set(copy_bin ${CMAKE_OBJCOPY})
        set(strip_bin ${CMAKE_OBJCOPY})
        if(QNX)
            set(debug_info_suffix sym)
            set(debug_info_keep --keep-file-symbols)
            set(strip_args "--strip-debug -R.ident")
        else()
            set(debug_info_suffix debug)
            set(debug_info_keep --only-keep-debug)
            set(strip_args --strip-debug)
        endif()
    endif()
    if(APPLE)
        get_target_property(is_framework ${target} FRAMEWORK)
        if(is_framework)
            set(debug_info_bundle_dir "$<TARGET_BUNDLE_DIR:${target}>.${debug_info_suffix}")
            set(BUNDLE_ID Qt${target})
        else()
            set(debug_info_bundle_dir "$<TARGET_FILE:${target}>.${debug_info_suffix}")
            set(BUNDLE_ID ${target})
        endif()
        set(debug_info_contents_dir "${debug_info_bundle_dir}/Contents")
        set(debug_info_target_dir "${debug_info_contents_dir}/Resources/DWARF")
        configure_file(
            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/QtSeparateDebugInfo.Info.plist.in"
            "Info.dSYM.plist"
            )
        list(APPEND commands
            COMMAND ${CMAKE_COMMAND} -E make_directory ${debug_info_target_dir}
            COMMAND ${CMAKE_COMMAND} -E copy "Info.dSYM.plist" "${debug_info_contents_dir}/Info.plist"
            )
        set(debug_info_target "${debug_info_target_dir}/$<TARGET_FILE_BASE_NAME:${target}>")
        qt_install(DIRECTORY ${debug_info_bundle_dir} DESTINATION ${installDestination})
    else()
        set(debug_info_target "$<TARGET_FILE_DIR:${target}>/$<TARGET_FILE_BASE_NAME:${target}>.${debug_info_suffix}")
        qt_install(FILES ${debug_info_target} DESTINATION ${installDestination})
    endif()
    list(APPEND commands
        COMMAND ${copy_bin} ${debug_info_keep} $<TARGET_FILE:${target}>
                ${copy_bin_out_arg} ${debug_info_target}
        COMMAND ${strip_bin} ${strip_args} $<TARGET_FILE:${target}>
        )
    if(NOT APPLE)
        list(APPEND commands
            COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=${debug_info_target} $<TARGET_FILE:${target}>
            )
    endif()
    if(NOT CMAKE_HOST_WIN32)
        list(APPEND commands
            COMMAND chmod -x ${debug_info_target}
            )
    endif()
    add_custom_command(
        TARGET ${target}
        POST_BUILD
        ${commands}
        )
endfunction()
