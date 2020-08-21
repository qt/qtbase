function(qt_internal_add_linker_version_script target)
    qt_parse_all_arguments(arg "qt_internal_add_linker" "INTERNAL" "" "PRIVATE_HEADERS" ${ARGN})

    if (TEST_ld_version_script)
        if (arg_INTERNAL)
            set(contents "Qt_${PROJECT_VERSION_MAJOR}_PRIVATE_API { *; };")
        else()
            set(contents "Qt_${PROJECT_VERSION_MAJOR}_PRIVATE_API {\n    qt_private_api_tag*;\n")
            foreach(ph ${arg_PRIVATE_HEADERS})
                string(APPEND contents "    @FILE:${ph}@\n")
            endforeach()
            string(APPEND contents "};\n")
            set(current "Qt_${PROJECT_VERSION_MAJOR}")
            if (QT_NAMESPACE STREQUAL "")
                set(tag_symbol "qt_version_tag")
            else()
                set(tag_symbol "qt_version_tag_${QT_NAMESPACE}")
            endif()
            string(APPEND contents "${current} { *; };\n")

            foreach(minor_version RANGE ${PROJECT_VERSION_MINOR})
                set(previous "${current}")
                set(current "Qt_${PROJECT_VERSION_MAJOR}.${minor_version}")
                if (minor_version EQUAL ${PROJECT_VERSION_MINOR})
                   string(APPEND contents "${current} { ${tag_symbol}; } ${previous};\n")
                else()
                   string(APPEND contents "${current} {} ${previous};\n")
                endif()
            endforeach()

            set(infile "${CMAKE_CURRENT_BINARY_DIR}/${target}.version.in")
            set(outfile "${CMAKE_CURRENT_BINARY_DIR}/${target}.version")

            file(GENERATE OUTPUT "${infile}" CONTENT "${contents}")

            qt_ensure_perl()

            add_custom_command(TARGET "${target}" PRE_LINK
                COMMAND "${HOST_PERL}" "${QT_MKSPECS_DIR}/features/data/unix/findclasslist.pl" < "${infile}" > "${outfile}"
                BYPRODUCTS "${outfile}" DEPENDS "${infile}"
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                COMMENT "Generating version linker script"
            )
            target_link_options("${target}" PRIVATE "-Wl,--version-script,${outfile}")
        endif()
    endif()
endfunction()

function(qt_internal_add_link_flags_no_undefined target)
    if (NOT QT_BUILD_SHARED_LIBS)
        return()
    endif()
    if (GCC OR CLANG)
        set(previous_CMAKE_REQUIRED_LINK_OPTIONS ${CMAKE_REQUIRED_LINK_OPTIONS})

        set(CMAKE_REQUIRED_LINK_OPTIONS "-Wl,-undefined,error")
        check_cxx_source_compiles("int main() {}" HAVE_DASH_UNDEFINED_SYMBOLS)
        if(HAVE_DASH_UNDEFINED_SYMBOLS)
            set(no_undefined_flag "-Wl,-undefined,error")
        endif()

        set(CMAKE_REQUIRED_LINK_OPTIONS "-Wl,--no-undefined")
        check_cxx_source_compiles("int main() {}" HAVE_DASH_DASH_NO_UNDEFINED)
        if(HAVE_DASH_DASH_NO_UNDEFINED)
            set(no_undefined_flag "-Wl,--no-undefined")
        endif()

        set(CMAKE_REQUIRED_LINK_OPTIONS ${previous_CMAKE_REQUIRED_LINK_OPTIONS})

        if (NOT HAVE_DASH_UNDEFINED_SYMBOLS AND NOT HAVE_DASH_DASH_NO_UNDEFINED)
            message(FATAL_ERROR "Platform linker doesn't support erroring upon encountering undefined symbols. Target:\"${target}\".")
        endif()
        target_link_options("${target}" PRIVATE "${no_undefined_flag}")
    endif()
endfunction()

function(qt_internal_apply_gc_binaries_conditional target visibility)
    # Should only be applied when the feature is enabled, aka for static builds.
    if(NOT QT_FEATURE_gc_binaries)
        return()
    endif()
    qt_internal_apply_gc_binaries("${target}" "${visibility}")
endfunction()

function(qt_internal_apply_gc_binaries target visibility)
    set(possible_visibilities PRIVATE INTERFACE PUBLIC)
    list(FIND possible_visibilities "${visibility}" known_visibility)
    if (known_visibility EQUAL "-1")
        message(FATAL_ERROR "Visibitily setting must be one of PRIVATE, INTERFACE or PUBLIC.")
    endif()

    if ((GCC OR CLANG) AND NOT EMSCRIPTEN AND NOT UIKIT)
        if(APPLE)
            set(gc_sections_flag "-Wl,-dead_strip")
        elseif(SOLARIS)
            set(gc_sections_flag "-Wl,-z,ignore")
        elseif(LINUX OR BSD OR WIN32 OR ANDROID)
            set(gc_sections_flag "-Wl,--gc-sections")
        endif()
    endif()
    if(gc_sections_flag)
        target_link_options("${target}" ${visibility} "${gc_sections_flag}")
    endif()

    if((GCC OR CLANG OR ICC) AND NOT EMSCRIPTEN AND NOT UIKIT)
        set(split_sections_flags "-ffunction-sections" "-fdata-sections")
    endif()
    if(split_sections_flags)
        target_compile_options("${target}" ${visibility} ${split_sections_flags})
    endif()
endfunction()

function(qt_internal_apply_intel_cet target visibility)
    if(NOT QT_FEATURE_intelcet)
        return()
    endif()

    set(possible_visibilities PRIVATE INTERFACE PUBLIC)
    list(FIND possible_visibilities "${visibility}" known_visibility)
    if (known_visibility EQUAL "-1")
        message(FATAL_ERROR "Visibitily setting must be one of PRIVATE, INTERFACE or PUBLIC.")
    endif()

    if(GCC)
        set(flags "-mshstk")
    endif()
    if(flags)
        target_compile_options("${target}" ${visibility} "${flags}")
    endif()
endfunction()

function(qt_internal_library_deprecation_level result)
    if(WIN32)
        # On Windows, due to the way DLLs work, we need to export all functions,
        # including the inlines
        list(APPEND deprecations "QT_DISABLE_DEPRECATED_BEFORE=0x040800")
    else()
        # On other platforms, Qt's own compilation goes needs to compile the Qt 5.0 API
        list(APPEND deprecations "QT_DISABLE_DEPRECATED_BEFORE=0x050000")
    endif()
    list(APPEND deprecations "QT_DEPRECATED_WARNINGS_SINCE=0x060000")
    set("${result}" deprecations PARENT_SCOPE)
endfunction()

# Sets the exceptions flags for the given target
function(qt_internal_set_no_exceptions_flags target)
    target_compile_definitions("${target}" PRIVATE "QT_NO_EXCEPTIONS")
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        target_compile_options("${target}" PRIVATE "/wd4530" "/wd4577")
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options("${target}" PRIVATE "-fno-exceptions")
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
        target_compile_options("${target}" PRIVATE "-fno-exceptions")
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        target_compile_options("${target}" PRIVATE "-fno-exceptions")
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
        target_compile_options("${target}" PRIVATE "-fno-exceptions")
    endif()
endfunction()

function(qt_skip_warnings_are_errors target)
    get_target_property(target_type "${target}" TYPE)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()
    set_target_properties("${target}" PROPERTIES QT_SKIP_WARNINGS_ARE_ERRORS ON)
endfunction()

function(qt_skip_warnings_are_errors_when_repo_unclean target)
    if(QT_REPO_NOT_WARNINGS_CLEAN)
        qt_skip_warnings_are_errors("${target}")
    endif()
endfunction()

function(qt_disable_warnings target)
    get_target_property(target_type "${target}" TYPE)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()
    set_target_properties("${target}" PROPERTIES QT_COMPILE_OPTIONS_DISABLE_WARNINGS ON)
endfunction()

function(qt_set_symbol_visibility_preset target value)
    get_target_property(target_type "${target}" TYPE)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()

    set_target_properties("${target}" PROPERTIES C_VISIBILITY_PRESET "${value}")
    set_target_properties("${target}" PROPERTIES CXX_VISIBILITY_PRESET "${value}")
    set_target_properties("${target}" PROPERTIES OBJC_VISIBILITY_PRESET "${value}")
    set_target_properties("${target}" PROPERTIES OBJCXX_VISIBILITY_PRESET "${value}")
endfunction()

function(qt_set_symbol_visibility_hidden target)
    qt_set_symbol_visibility_preset("${target}" "hidden")
endfunction()

function(qt_set_language_standards)
    ## Use the latest standard the compiler supports (same as qt_common.prf)
    if (QT_FEATURE_cxx2a)
        set(CMAKE_CXX_STANDARD 20 PARENT_SCOPE)
    else()
        set(CMAKE_CXX_STANDARD 17 PARENT_SCOPE)
    endif()

    if (c_std_11 IN_LIST CMAKE_C_COMPILE_FEATURES)
        set(CMAKE_C_STANDARD 11 PARENT_SCOPE)
    elseif (c_std_99 IN_LIST CMAKE_C_COMPILE_FEATURES)
        set(CMAKE_C_STANDARD 99 PARENT_SCOPE)
    endif()
endfunction()

function(qt_set_language_standards_interface_compile_features target)
    # Regardless of which C++ standard is used to build Qt itself, require C++17 when building
    # Qt applications using CMake (because the Qt header files use C++17 features).
    set(cpp_feature "cxx_std_17")
    target_compile_features("${target}" INTERFACE ${cpp_feature})
endfunction()

function(qt_enable_msvc_cplusplus_define target visibility)
    # For MSVC we need to explicitly pass -Zc:__cplusplus to get correct __cplusplus.
    # Check qt_config_compile_test for more info.
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND MSVC_VERSION GREATER_EQUAL 1913)
        target_compile_options("${target}" ${visibility} "-Zc:__cplusplus")
    endif()
endfunction()

function(qt_enable_utf8_sources target)
    set(utf8_flags "")
    if(MSVC)
        list(APPEND utf8_flags "-utf-8")
    elseif(WIN32 AND ICC)
        list(APPEND utf8_flags "-Qoption,cpp,--unicode_source_kind,UTF-8")
    endif()

    if(utf8_flags)
        # Allow opting out by specifying the QT_NO_UTF8_SOURCE target property.
        set(genex_condition "$<NOT:$<BOOL:$<TARGET_PROPERTY:QT_NO_UTF8_SOURCE>>>")
        set(utf8_flags "$<${genex_condition}:${utf8_flags}>")
        target_compile_options("${target}" INTERFACE "${utf8_flags}")
    endif()
endfunction()

function(qt_internal_get_all_possible_optimization_flag_values out_var)
    set(flag_values "")
    set(vars QT_CFLAGS_OPTIMIZE QT_CFLAGS_OPTIMIZE_FULL
             QT_CFLAGS_OPTIMIZE_DEBUG QT_CFLAGS_OPTIMIZE_SIZE)
    foreach(optimize_var ${vars})
        set(value "${${optimize_var}}")
        if(value)
            list(APPEND flag_values "${value}")
        endif()
    endforeach()

    # Additional flag values which might not be used in qmake mkspecs, but might be set by CMake,
    # aka flags that are recognized by the compile which we might want to remove.
    if(QT_CFLAGS_OPTIMIZE_VALID_VALUES)
        list(APPEND flag_values ${QT_CFLAGS_OPTIMIZE_VALID_VALUES})
    endif()

    set("${out_var}" "${flag_values}" PARENT_SCOPE)
endfunction()

function(qt_internal_print_optimization_flags_values languages configs target_link_types)
    foreach(lang ${languages})
        set(flag_var_name "CMAKE_${lang}_FLAGS")
        message(STATUS "${flag_var_name}: ${${flag_var_name}}")

        foreach(config ${configs})
            set(flag_var_name "CMAKE_${lang}_FLAGS_${config}")
            message(STATUS "${flag_var_name}: ${${flag_var_name}}")
        endforeach()
    endforeach()

    foreach(t ${target_link_types})
        set(flag_var_name "CMAKE_${t}_LINKER_FLAGS")
        message(STATUS "${flag_var_name}: ${${flag_var_name}}")

        foreach(config ${configs})
            set(flag_var_name "CMAKE_${t}_LINKER_FLAGS_${config}")
            message(STATUS "${flag_var_name}: ${${flag_var_name}}")
        endforeach()
    endforeach()
endfunction()

# This function finds the optimization flags set by the default CMake modules or toolchains, and
# replaces them with ones that Qt qmake builds expect for all the default CMAKE_BUILD_TYPE
# configurations.
# This normalizes things like using -O2 for both Release and RelWithDebInfo, among other flags.
# See QTBUG-85992 for details.
function(qt_internal_set_up_config_optimizations_like_in_qmake)
    # Allow opt out.
    if(QT_USE_DEFAULT_CMAKE_OPTIMIZATION_FLAGS)
        return()
    endif()

    # Limit flag modification to c-like code. We don't want to accidentally add incompatible
    # flags to MSVC's RC or Swift.
    set(languages_to_process C CXX OBJC OBJCXX)
    get_property(globally_enabled_languages GLOBAL PROPERTY ENABLED_LANGUAGES)
    set(enabled_languages "")
    foreach(lang ${languages_to_process})
        if(lang IN_LIST globally_enabled_languages)
            list(APPEND enabled_languages "${lang}")
        endif()
    endforeach()

    set(configs RELEASE RELWITHDEBINFO MINSIZEREL DEBUG)
    set(target_link_types EXE SHARED MODULE STATIC)

    # Opt into additional non-standard configs for flag removal only.
    if(QT_ADDITIONAL_OPTIMIZATION_FLAG_CONFIGS)
        list(APPEND configs ${QT_ADDITIONAL_OPTIMIZATION_FLAG_CONFIGS})
    endif()

    # You can set QT_DEBUG_OPTIMIZATION_FLAGS to see the before and after results.
    if(QT_DEBUG_OPTIMIZATION_FLAGS)
        message(STATUS "")
        message(STATUS "DEBUG: Original CMake optimization flags.\n")
        qt_internal_print_optimization_flags_values("${enabled_languages}" "${configs}"
                                                    "${target_link_types}")
    endif()

    # Remove known optimization flags.
    qt_internal_get_all_possible_optimization_flag_values(flag_values)
    foreach(lang ${enabled_languages})
        foreach(config ${configs})
            set(flag_var_name "CMAKE_${lang}_FLAGS_${config}")
            foreach(flag_value ${flag_values})
                # Remove any existing optimization flags, they will be re-added later on.
                string(REPLACE "${flag_value}" "" "${flag_var_name}" "${${flag_var_name}}")
                string(STRIP "${${flag_var_name}}" "${flag_var_name}")
            endforeach()
        endforeach()
    endforeach()

    # Re-add optimization flags as per qmake mkspecs.
    foreach(lang ${enabled_languages})
        foreach(config ${configs})
            set(flag_var_name "CMAKE_${lang}_FLAGS_${config}")

            # Release and RelWithDebInfo should get the same base optimization flags.
            if(config STREQUAL "RELEASE" AND QT_CFLAGS_OPTIMIZE)
                string(APPEND "${flag_var_name}" " ${QT_CFLAGS_OPTIMIZE}")
            elseif(config STREQUAL "RELWITHDEBINFO" AND QT_CFLAGS_OPTIMIZE)
                string(APPEND "${flag_var_name}" " ${QT_CFLAGS_OPTIMIZE}")

            # MinSizeRel should get the optimize size flag if available, otherwise the regular
            # release flag.
            elseif(config STREQUAL "MINSIZEREL")
                if(QT_CFLAGS_OPTIMIZE_SIZE)
                    string(APPEND "${flag_var_name}" " ${QT_CFLAGS_OPTIMIZE_SIZE}")
                else()
                    string(APPEND "${flag_var_name}" " ${QT_CFLAGS_OPTIMIZE}")
                endif()
            endif()

            # Debug should get the OPTIMIZE_DEBUG flag if the respective feature is ON.
            if(config STREQUAL "DEBUG" AND QT_FEATURE_optimize_debug)
                string(APPEND "${flag_var_name}" " ${QT_CFLAGS_OPTIMIZE_DEBUG}")
            endif()

            set(configs_for_optimize_size RELEASE RELWITHDEBINFO)
            if(QT_FEATURE_optimize_size AND config IN_LIST configs_for_optimize_size)
                string(APPEND "${flag_var_name}" " ${QT_CFLAGS_OPTIMIZE_SIZE}")
            endif()

            # Assign value to the cache entry.
            get_property(help_text CACHE "${flag_var_name}" PROPERTY HELPSTRING)
            set("${flag_var_name}" "${${flag_var_name}}" CACHE STRING "${help_text}" FORCE)
        endforeach()
    endforeach()

    if(MSVC)
        # Handle MSVC /INCREMENTAL flag which should not be enabled for Release configurations.
        # First remove them from all configs, and re-add INCREMENTAL for Debug only.
        set(flag_values "/INCREMENTAL:YES" "/INCREMENTAL:NO" "/INCREMENTAL")
        foreach(config ${configs})
            foreach(t ${target_link_types})
                set(flag_var_name "CMAKE_${t}_LINKER_FLAGS_${config}")
                foreach(flag_value ${flag_values})
                    string(REPLACE "${flag_value}" "" "${flag_var_name}" "${${flag_var_name}}")
                    string(STRIP "${${flag_var_name}}" "${flag_var_name}")
                endforeach()
            endforeach()
        endforeach()

        foreach(config ${configs})
            foreach(t ${target_link_types})
                set(flag_var_name "CMAKE_${t}_LINKER_FLAGS_${config}")

                if(config STREQUAL "RELEASE" OR config STREQUAL "RELWITHDEBINFO"
                   OR config STREQUAL "MINSIZEREL")
                    string(APPEND "${flag_var_name}" " /INCREMENTAL:NO")
                elseif(config STREQUAL "DEBUG")
                    string(APPEND "${flag_var_name}" " /INCREMENTAL:YES")
                endif()

                # Assign value to the cache entry.
                get_property(help_text CACHE "${flag_var_name}" PROPERTY HELPSTRING)
                set("${flag_var_name}" "${${flag_var_name}}" CACHE STRING "${help_text}" FORCE)
            endforeach()
        endforeach()
    endif()

    if(QT_DEBUG_OPTIMIZATION_FLAGS)
        message(STATUS "")
        message(STATUS "DEBUG: Modified optimization flags to mirror qmake mkspecs.\n")
        qt_internal_print_optimization_flags_values("${enabled_languages}" "${configs}"
                                                    "${target_link_types}")
    endif()
endfunction()
