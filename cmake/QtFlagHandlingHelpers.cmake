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
        if(CLANG AND QT_FEATURE_sanitizer)
            return()
        endif()
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

    if ((GCC OR CLANG) AND NOT EMSCRIPTEN AND NOT UIKIT AND NOT MSVC)
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

    if((GCC OR CLANG OR ICC) AND NOT EMSCRIPTEN AND NOT UIKIT AND NOT MSVC)
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
    set("${result}" "${deprecations}" PARENT_SCOPE)
endfunction()

# Sets the exceptions flags for the given target
function(qt_internal_set_no_exceptions_flags target)
    target_compile_definitions("${target}" PRIVATE "QT_NO_EXCEPTIONS")
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        set(_flag "/wd4530" "/wd4577")
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        set(_flag "-fno-exceptions")
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
        set(_flag "-fno-exceptions")
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        if (MSVC)
            set(_flag "/wd4530" "/wd4577")
        else()
            set(_flag "-fno-exceptions")
        endif()
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
        set(_flag "-fno-exceptions")
    endif()
    if (_flag)
        target_compile_options("${target}" PRIVATE ${_flag})
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

function(qt_set_msvc_cplusplus_options target visibility)
    # For MSVC we need to explicitly pass -Zc:__cplusplus to get correct __cplusplus.
    # Check qt_config_compile_test for more info.
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND MSVC_VERSION GREATER_EQUAL 1913)
        target_compile_options("${target}" ${visibility} "-Zc:__cplusplus" "-permissive-")
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

# Saves the list of known optimization flags for the current compiler in out_var.
#
# Mostly used for removing them before adding new ones.
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

# Return's the current compiler's optimize_full flags if available.
# Otherwise returns the regular optimization level flag.
function(qt_internal_get_optimize_full_flags out_var)
    set(optimize_full_flags "${QT_CFLAGS_OPTIMIZE_FULL}")
    if(NOT optimize_full_flags)
        set(optimize_full_flags "${QT_CFLAGS_OPTIMIZE}")
    endif()
    set(${out_var} "${optimize_full_flags}" PARENT_SCOPE)
endfunction()

# Prints the compiler and linker flags for each configuration, language and target type.
#
# Usually it would print the cache variables, but one may also override the variables
# in a specific directory scope, so this is useful for debugging.
#
# Basically dumps either scoped or cached
# CMAKE_<LANG>_FLAGS_CONFIG> and CMAKE_<TYPE>_LINKER_FLAGS_<CONFIG> variables.

function(qt_internal_print_optimization_flags_values)
    qt_internal_get_enabled_languages_for_flag_manipulation(languages)
    qt_internal_get_configs_for_flag_manipulation(configs)
    qt_internal_get_target_link_types_for_flag_manipulation(target_link_types)

    qt_internal_print_optimization_flags_values_helper(
        "${languages}" "${configs}" "${target_link_types}")
endfunction()

# Helper function for printing the optimization flags.
function(qt_internal_print_optimization_flags_values_helper languages configs target_link_types)
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

# Saves the list of configs for which flag manipulation will occur.
function(qt_internal_get_configs_for_flag_manipulation out_var)
    set(configs RELEASE RELWITHDEBINFO MINSIZEREL DEBUG)

    # Opt into additional non-standard configs for flag removal only.
    if(QT_ADDITIONAL_OPTIMIZATION_FLAG_CONFIGS)
        list(APPEND configs ${QT_ADDITIONAL_OPTIMIZATION_FLAG_CONFIGS})
    endif()

    set(${out_var} "${configs}" PARENT_SCOPE)
endfunction()

# Saves the list of target link types for which flag manipulation will occur.
function(qt_internal_get_target_link_types_for_flag_manipulation out_var)
    set(target_link_types EXE SHARED MODULE STATIC)
    set(${out_var} "${target_link_types}" PARENT_SCOPE)
endfunction()

# Saves list of enabled languages for which it is safe to manipulate compilation flags.
function(qt_internal_get_enabled_languages_for_flag_manipulation out_var)
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
    set(${out_var} "${enabled_languages}" PARENT_SCOPE)
endfunction()

# Removes all known compiler optimization flags for the given CONFIGS, for all enabled 'safe'
# languages.
#
# IN_CACHE         - remove them globally (aka in the corresponding cache entries)
# IN_CURRENT_SCOPE - remove them only in the current directory scope (effectively setting them,
#                    if they did not exist beforehand)
# CONFIGS          - should be a list of upper case configs like DEBUG, RELEASE, RELWITHDEBINFO.
# LANGUAGES        - optional list of languages like 'C', 'CXX', for which to remove the flags
#                    if not provided, defaults to the list of enabled C-like languages
function(qt_internal_remove_known_optimization_flags)
    qt_parse_all_arguments(
        arg
        "qt_internal_remove_known_optimization_flags"
        "IN_CACHE;IN_CURRENT_SCOPE"
        ""
        "CONFIGS;LANGUAGES"
        ${ARGN})

    if(NOT arg_CONFIGS)
        message(FATAL_ERROR
            "You must specify at least one configuration for which to add the flags.")
    endif()

    if(arg_LANGUAGES)
        set(enabled_languages "${arg_LANGUAGES}")
    else()
        qt_internal_get_enabled_languages_for_flag_manipulation(enabled_languages)
    endif()

    qt_internal_get_all_possible_optimization_flag_values(flag_values)
    set(configs ${arg_CONFIGS})

    foreach(lang ${enabled_languages})
        foreach(config ${configs})
            set(flag_var_name "CMAKE_${lang}_FLAGS_${config}")
            foreach(flag_value ${flag_values})
                string(REPLACE "${flag_value}" "" "${flag_var_name}" "${${flag_var_name}}")
                string(STRIP "${${flag_var_name}}" "${flag_var_name}")
            endforeach()

            if(arg_IN_CACHE)
                get_property(help_text CACHE "${flag_var_name}" PROPERTY HELPSTRING)
                set("${flag_var_name}" "${${flag_var_name}}" CACHE STRING "${help_text}" FORCE)
            elseif(arg_IN_CURRENT_SCOPE)
                set("${flag_var_name}" "${${flag_var_name}}" PARENT_SCOPE)
            else()
                message(
                    FATAL_ERROR
                    "qt_internal_remove_known_optimization_flags expects a scope argument.")
            endif()
        endforeach()
    endforeach()
endfunction()

# Adds compiler flags for the given CONFIGS either in the current scope or globally in the
# cache.
#
# FLAGS            - should be a single string of flags separated by spaces.
# IN_CACHE         - add them globally (aka in the corresponding cache entries)
# IN_CURRENT_SCOPE - add them only in the current directory scope (effectively setting them,
#                    if they did not exist beforehand)
# CONFIGS          - should be a list of upper case configs like DEBUG, RELEASE, RELWITHDEBINFO.
# LANGUAGES        - optional list of languages like 'C', 'CXX', for which to add the flags
#                    if not provided, defaults to the list of enabled C-like languages
function(qt_internal_add_compiler_flags)
    qt_parse_all_arguments(
        arg
        "qt_internal_add_compiler_flags"
        "IN_CACHE;IN_CURRENT_SCOPE"
        "FLAGS"
        "CONFIGS;LANGUAGES"
        ${ARGN})

    if(NOT arg_CONFIGS)
        message(FATAL_ERROR
            "You must specify at least one configuration for which to add the flags.")
    endif()
    if(NOT arg_FLAGS)
        message(FATAL_ERROR "You must specify at least one flag to add.")
    endif()

    if(arg_LANGUAGES)
        set(enabled_languages "${arg_LANGUAGES}")
    else()
        qt_internal_get_enabled_languages_for_flag_manipulation(enabled_languages)
    endif()

    set(configs ${arg_CONFIGS})

    foreach(lang ${enabled_languages})
        foreach(config ${configs})
            set(flag_var_name "CMAKE_${lang}_FLAGS_${config}")
            string(APPEND "${flag_var_name}" " ${arg_FLAGS}")

            if(arg_IN_CACHE)
                get_property(help_text CACHE "${flag_var_name}" PROPERTY HELPSTRING)
                set("${flag_var_name}" "${${flag_var_name}}" CACHE STRING "${help_text}" FORCE)
            elseif(arg_IN_CURRENT_SCOPE)
                set("${flag_var_name}" "${${flag_var_name}}" PARENT_SCOPE)
            else()
                message(
                    FATAL_ERROR
                    "qt_internal_add_compiler_flags expects a scope argument.")
            endif()
        endforeach()
    endforeach()
endfunction()

# Convenience function that adds compiler flags for all release configurations.
#
# FLAGS            - should be a single string of flags separated by spaces.
# IN_CACHE         - add them globally (aka in the corresponding cache entries)
# IN_CURRENT_SCOPE - add them only in the current directory scope (effectively setting them,
#                    if they did not exist beforehand)
# LANGUAGES        - optional list of languages like 'C', 'CXX', for which to add the flags
#                    if not provided, defaults to the list of enabled C-like languages
function(qt_internal_add_compiler_flags_for_release_configs)
    qt_parse_all_arguments(
        arg
        "qt_internal_add_compiler_flags_for_release_configs"
        "IN_CACHE;IN_CURRENT_SCOPE"
        "FLAGS"
        "LANGUAGES"
        ${ARGN})

    set(args "")

    if(arg_LANGUAGES)
        set(enabled_languages "${arg_LANGUAGES}")
    else()
        qt_internal_get_enabled_languages_for_flag_manipulation(enabled_languages)
    endif()

    set(configs RELEASE RELWITHDEBINFO MINSIZEREL)

    list(APPEND args CONFIGS ${configs})

    if(arg_FLAGS)
        list(APPEND args FLAGS "${arg_FLAGS}")
    endif()

    if(arg_IN_CACHE)
        list(APPEND args IN_CACHE)
    endif()
    if(arg_IN_CURRENT_SCOPE)
        list(APPEND args IN_CURRENT_SCOPE)
    endif()
    list(APPEND args LANGUAGES ${enabled_languages})

    qt_internal_add_compiler_flags(${args})

    if(arg_IN_CURRENT_SCOPE)
        foreach(lang ${enabled_languages})
            foreach(config ${configs})
                set(flag_var_name "CMAKE_${lang}_FLAGS_${config}")
                set("${flag_var_name}" "${${flag_var_name}}" PARENT_SCOPE)
            endforeach()
        endforeach()
    endif()
endfunction()

# Convenience function that replaces all optimization flags with the equivalent of '-O3'
# (optimize_full) flag for all release configs.
#
# This is the equivalent of qmake's CONFIG += optimize_full.
# It is meant to be called in a subdirectory scope to enable full optimizations for a particular
# Qt module, like Core or Gui.
function(qt_internal_add_optimize_full_flags)
    qt_parse_all_arguments(
        arg
        "qt_internal_add_optimize_full_flags"
        "IN_CACHE;IN_CURRENT_SCOPE"
        ""
        ""
        ${ARGN})

    set(args "")
    if(arg_IN_CACHE)
        list(APPEND args IN_CACHE)
    endif()
    if(arg_IN_CURRENT_SCOPE)
        list(APPEND args IN_CURRENT_SCOPE)
    endif()

    qt_internal_get_enabled_languages_for_flag_manipulation(enabled_languages)
    set(configs RELEASE RELWITHDEBINFO MINSIZEREL)

    qt_internal_remove_known_optimization_flags(${args} CONFIGS ${configs})

    # If the respective compiler doesn't have optimize_full flags, use regular optimization flags.
    # Mainly MSVC.
    qt_internal_get_optimize_full_flags(optimize_full_flags)
    list(APPEND args FLAGS "${optimize_full_flags}")

    qt_internal_add_compiler_flags_for_release_configs(${args})

    if(arg_IN_CURRENT_SCOPE)
        foreach(lang ${enabled_languages})
            foreach(config ${configs})
                set(flag_var_name "CMAKE_${lang}_FLAGS_${config}")
                set("${flag_var_name}" "${${flag_var_name}}" PARENT_SCOPE)
            endforeach()
        endforeach()
    endif()
endfunction()

# Convenience function to replace a compiler flag with another one, for the given configurations
# for all enabled 'safe' languages.
# Essentially a glorified string(REPLACE).
# Can be used to remove compiler flags.
#
# match_string     - string to match
# replace_string   - replacement string
# IN_CACHE         - replace them globally (aka in the corresponding cache entries)
# IN_CURRENT_SCOPE - replace them only in the current directory scope (effectively setting them,
#                    if they did not exist beforehand)
# CONFIGS          - should be a list of upper case configs like DEBUG, RELEASE, RELWITHDEBINFO.
# LANGUAGES        - optional list of languages like 'C', 'CXX', for which to replace the flags
#                    if not provided, defaults to the list of enabled C-like languages
function(qt_internal_replace_compiler_flags match_string replace_string)
    qt_parse_all_arguments(
        arg
        "qt_internal_replace_compiler_flags"
        "IN_CACHE;IN_CURRENT_SCOPE"
        ""
        "CONFIGS;LANGUAGES"
        ${ARGN})

    if(NOT arg_CONFIGS)
        message(FATAL_ERROR
            "You must specify at least one configuration for which to replace the flags.")
    endif()

    if(arg_LANGUAGES)
        set(enabled_languages "${arg_LANGUAGES}")
    else()
        qt_internal_get_enabled_languages_for_flag_manipulation(enabled_languages)
    endif()
    set(configs ${arg_CONFIGS})

    foreach(lang ${enabled_languages})
        foreach(config ${configs})
            set(flag_var_name "CMAKE_${lang}_FLAGS_${config}")

            # Handle an empty input string and an empty match string as a set().
            if(match_string STREQUAL "" AND "${${flag_var_name}}" STREQUAL "")
                set(${flag_var_name} "${replace_string}")
            else()
                string(REPLACE
                       "${match_string}" "${replace_string}"
                       "${flag_var_name}" "${${flag_var_name}}")
            endif()

            if(arg_IN_CACHE)
                get_property(help_text CACHE "${flag_var_name}" PROPERTY HELPSTRING)
                set("${flag_var_name}" "${${flag_var_name}}" CACHE STRING "${help_text}" FORCE)
            elseif(arg_IN_CURRENT_SCOPE)
                set("${flag_var_name}" "${${flag_var_name}}" PARENT_SCOPE)
            else()
                message(
                    FATAL_ERROR
                    "qt_internal_replace_compiler_flags expects a scope argument.")
            endif()
        endforeach()
    endforeach()
endfunction()

# Convenience function to add linker flags, for the given configurations and target link types.
#
# FLAGS            - should be a single string of flags separated by spaces.
# IN_CACHE         - add them globally (aka in the corresponding cache entries)
# IN_CURRENT_SCOPE - add them only in the current directory scope (effectively setting them,
#                    if they did not exist beforehand)
# CONFIGS          - should be a list of upper case configs like DEBUG, RELEASE, RELWITHDEBINFO.
# TYPES            - should be a list of target link types as expected by CMake's
#                    CMAKE_<LINKER_TYPE>_LINKER_FLAGS_<CONFIG> cache variable.
#                    e.g EXE, MODULE, SHARED, STATIC.
function(qt_internal_add_linker_flags)
    qt_parse_all_arguments(
        arg
        "qt_internal_add_linker_flags"
        "IN_CACHE;IN_CURRENT_SCOPE"
        "FLAGS"
        "CONFIGS;TYPES"
        ${ARGN})

    if(NOT arg_TYPES)
        message(FATAL_ERROR
            "You must specify at least one linker target type for which to add the flags.")
    endif()
    if(NOT arg_CONFIGS)
        message(FATAL_ERROR
            "You must specify at least one configuration for which to add the flags.")
    endif()
    if(NOT arg_FLAGS)
        message(FATAL_ERROR "You must specify at least one flag to add.")
    endif()

    set(configs ${arg_CONFIGS})
    set(target_link_types ${arg_TYPES})

    foreach(config ${configs})
        foreach(t ${target_link_types})
            set(flag_var_name "CMAKE_${t}_LINKER_FLAGS_${config}")
            string(APPEND "${flag_var_name}" " ${arg_FLAGS}")

            if(arg_IN_CACHE)
                get_property(help_text CACHE "${flag_var_name}" PROPERTY HELPSTRING)
                set("${flag_var_name}" "${${flag_var_name}}" CACHE STRING "${help_text}" FORCE)
            elseif(arg_IN_CURRENT_SCOPE)
                set("${flag_var_name}" "${${flag_var_name}}" PARENT_SCOPE)
            else()
                message(
                    FATAL_ERROR
                    "qt_internal_add_linker_flags expects a scope argument.")
            endif()
        endforeach()
    endforeach()
endfunction()

# Convenience function to replace a linker flag with another one, for the given configurations
# and target link types.
# Essentially a glorified string(REPLACE).
# Can be used to remove linker flags.
#
# match_string     - string to match
# replace_string   - replacement string
# IN_CACHE         - replace them globally (aka in the corresponding cache entries)
# IN_CURRENT_SCOPE - replace them only in the current directory scope (effectively setting them,
#                    if they did not exist beforehand)
# CONFIGS          - should be a list of upper case configs like DEBUG, RELEASE, RELWITHDEBINFO.
# TYPES            - should be a list of target link types as expected by CMake's
#                    CMAKE_<LINKER_TYPE>_LINKER_FLAGS_<CONFIG> cache variable.
#                    e.g EXE, MODULE, SHARED, STATIC.
function(qt_internal_replace_linker_flags match_string replace_string)
    qt_parse_all_arguments(
        arg
        "qt_internal_replace_compiler_flags"
        "IN_CACHE;IN_CURRENT_SCOPE"
        ""
        "CONFIGS;TYPES"
        ${ARGN})

    if(NOT arg_TYPES)
        message(FATAL_ERROR
            "You must specify at least one linker target type for which to replace the flags.")
    endif()
    if(NOT arg_CONFIGS)
        message(FATAL_ERROR
            "You must specify at least one configuration for which to replace the flags.")
    endif()

    set(configs ${arg_CONFIGS})
    set(target_link_types ${arg_TYPES})

    foreach(config ${configs})
        foreach(t ${target_link_types})
            set(flag_var_name "CMAKE_${t}_LINKER_FLAGS_${config}")

            # Handle an empty input string and an empty match string as a set().
            if(match_string STREQUAL "" AND "${${flag_var_name}}" STREQUAL "")
                set(${flag_var_name} "${replace_string}")
            else()
                string(REPLACE
                       "${match_string}" "${replace_string}"
                       "${flag_var_name}" "${${flag_var_name}}")
            endif()

            if(arg_IN_CACHE)
                get_property(help_text CACHE "${flag_var_name}" PROPERTY HELPSTRING)
                set("${flag_var_name}" "${${flag_var_name}}" CACHE STRING "${help_text}" FORCE)
            elseif(arg_IN_CURRENT_SCOPE)
                set("${flag_var_name}" "${${flag_var_name}}" PARENT_SCOPE)
            else()
                message(
                    FATAL_ERROR
                    "qt_internal_replace_compiler_flags expects a scope argument.")
            endif()
        endforeach()
    endforeach()
endfunction()

# This function finds the optimization flags set by the default CMake platform modules or toolchain
# files and replaces them with flags that Qt qmake builds expect, for all the usual
# CMAKE_BUILD_TYPE configurations.
# This normalizes things like using -O2 for both Release and RelWithDebInfo, among other compilation
# flags. Also some linker flags specific to MSVC.
# See QTBUG-85992 for details.
function(qt_internal_set_up_config_optimizations_like_in_qmake)
    # Allow opt out.
    if(QT_USE_DEFAULT_CMAKE_OPTIMIZATION_FLAGS)
        return()
    endif()

    qt_internal_get_enabled_languages_for_flag_manipulation(enabled_languages)
    qt_internal_get_configs_for_flag_manipulation(configs)
    qt_internal_get_target_link_types_for_flag_manipulation(target_link_types)

    # You can set QT_DEBUG_OPTIMIZATION_FLAGS to see the before and after results.
    if(QT_DEBUG_OPTIMIZATION_FLAGS)
        message(STATUS "")
        message(STATUS "DEBUG: Original CMake optimization flags.\n")
        qt_internal_print_optimization_flags_values_helper("${enabled_languages}" "${configs}"
                                                           "${target_link_types}")
    endif()

    # Remove known optimization flags.
    qt_internal_remove_known_optimization_flags(IN_CACHE CONFIGS ${configs})

    # Re-add optimization flags as per qmake mkspecs.
    foreach(lang ${enabled_languages})
        foreach(config ${configs})
            set(flag_var_name "CMAKE_${lang}_FLAGS_${config}")
            set(value_to_append "")

            # Release and RelWithDebInfo should get the same base optimization flags.
            if(config STREQUAL "RELEASE" AND QT_CFLAGS_OPTIMIZE)
                set(value_to_append "${QT_CFLAGS_OPTIMIZE}")
            elseif(config STREQUAL "RELWITHDEBINFO" AND QT_CFLAGS_OPTIMIZE)
                set(value_to_append "${QT_CFLAGS_OPTIMIZE}")

            # MinSizeRel should get the optimize size flag if available, otherwise the regular
            # release flag.
            elseif(config STREQUAL "MINSIZEREL")
                if(QT_CFLAGS_OPTIMIZE_SIZE)
                    set(value_to_append "${QT_CFLAGS_OPTIMIZE_SIZE}")
                else()
                    set(value_to_append "${QT_CFLAGS_OPTIMIZE}")
                endif()
            endif()

            # Debug should get the OPTIMIZE_DEBUG flag if the respective feature is ON.
            if(config STREQUAL "DEBUG" AND QT_FEATURE_optimize_debug)
                set(value_to_append "${QT_CFLAGS_OPTIMIZE_DEBUG}")
            endif()

            set(configs_for_optimize_size RELEASE RELWITHDEBINFO)
            if(QT_FEATURE_optimize_size AND config IN_LIST configs_for_optimize_size)
                set(value_to_append "${QT_CFLAGS_OPTIMIZE_SIZE}")
            endif()

            # Check if the fake 'optimize_full' feature is enabled.
            # Use the max optimization level flag for all release configs, effectively
            # overriding any previous setting.
            set(configs_for_optimize RELEASE RELWITHDEBINFO MINSIZEREL)
            if(QT_FEATURE_optimize_full AND config IN_LIST configs_for_optimize)
                qt_internal_get_optimize_full_flags(optimize_full_flags)
                set(value_to_append "${optimize_full_flags}")
            endif()

            # Assign value to the cache entry.
            if(value_to_append)
                string(APPEND "${flag_var_name}" " ${value_to_append}")
                get_property(help_text CACHE "${flag_var_name}" PROPERTY HELPSTRING)
                set("${flag_var_name}" "${${flag_var_name}}" CACHE STRING "${help_text}" FORCE)
            endif()

        endforeach()
    endforeach()

    if(MSVC)
        # Handle MSVC /INCREMENTAL flag which should not be enabled for Release configurations.
        # First remove them from all configs, and re-add INCREMENTAL for Debug only.
        set(flag_values "/INCREMENTAL:YES" "/INCREMENTAL:NO" "/INCREMENTAL")
        foreach(flag_value ${flag_values})
            qt_internal_replace_linker_flags(
                    "${flag_value}" ""
                    CONFIGS ${configs}
                    TYPES ${target_link_types}
                    IN_CACHE)
        endforeach()

        set(flag_value "/INCREMENTAL:NO")
        qt_internal_add_linker_flags(
                FLAGS "${flag_value}"
                CONFIGS RELEASE RELWITHDEBINFO MINSIZEREL
                TYPES EXE SHARED # when linking static libraries, link.exe can't recognize this parameter, clang-cl will error out.
                IN_CACHE)
    endif()

    # Allow opting into generating debug info in object files with a fake feature.
    # This would allow us to enable caching with sccache.
    # See QTQAINFRA-3934 for details.
    if(MSVC AND QT_FEATURE_msvc_obj_debug_info)
        qt_internal_replace_compiler_flags(
                "/Zi" "/Z7"
                CONFIGS RELWITHDEBINFO DEBUG
                TYPES ${target_link_types}
                IN_CACHE)
    endif()

    if(QT_DEBUG_OPTIMIZATION_FLAGS)
        message(STATUS "")
        message(STATUS "DEBUG: Modified optimization flags to mirror qmake mkspecs.\n")
        qt_internal_print_optimization_flags_values_helper("${enabled_languages}" "${configs}"
                                                           "${target_link_types}")
    endif()
endfunction()
