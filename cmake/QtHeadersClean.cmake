# Add a custom ${module_include_name}_header_check target that builds each header in
# ${module_headers} with a custom set of defines. This makes sure our public headers
# are self-contained, and also compile with more strict compiler options.
function(qt_internal_add_headers_clean_target
        module_target
        module_include_name
        module_headers)
    # module_headers is a list of strings of the form
    #  <headerfile>[:feature]
    set(hclean_headers "")
    foreach(entry ${module_headers})
        string(REPLACE ":" ";" entry_list ${entry})
        list(LENGTH entry_list entry_list_length)
        list(GET entry_list 0 entry_path)

        if (${entry_list_length} EQUAL 2)
            list(GET entry_list 1 entry_feature)
            if (NOT QT_FEATURE_${entry_feature})
                message(STATUS "headersclean: Ignoring header ${entry_path} because of missing feature ${entry_feature}")
                continue()
            endif()
        endif()
        list(APPEND hclean_headers ${entry_path})
    endforeach()

    # Make sure that the header compiles with our strict options
    set(hcleanDEFS -DQT_NO_CAST_TO_ASCII
                 -DQT_NO_CAST_FROM_ASCII
                 -DQT_NO_URL_CAST_FROM_STRING
                 -DQT_NO_CAST_FROM_BYTEARRAY
                 -DQT_NO_KEYWORDS
                 -DQT_USE_QSTRINGBUILDER
                 -DQT_USE_FAST_OPERATOR_PLUS)

    set(compiler_to_run "${CMAKE_CXX_COMPILER}")
    if(CMAKE_CXX_COMPILER_LAUNCHER)
        list(PREPEND compiler_to_run "${CMAKE_CXX_COMPILER_LAUNCHER}")
    endif()

    set(prop_prefix "")
    get_target_property(target_type "${target}" TYPE)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        set(prop_prefix "INTERFACE_")
    endif()

    set(target_includes_genex
        "$<TARGET_PROPERTY:${module_target},${prop_prefix}INCLUDE_DIRECTORIES>")
    set(includes_exist_genex "$<BOOL:${target_includes_genex}>")
    set(target_includes_joined_genex
        "$<${includes_exist_genex}:-I$<JOIN:${target_includes_genex},;-I>>")

    # qmake doesn't seem to add the defines that are set by the header_only_module when checking the
    # the cleanliness of the module's header files.
    # This allows us to bypass an error with CMake 3.18 and lower when trying to evaluate
    # genex-enhanced compile definitions. An example of that is in
    # qttools/src/designer/src/uiplugin/CMakeLists.txt which ends up causing the following error
    # message:
    #  CMake Error at qtbase/cmake/QtModuleHelpers.cmake:35 (add_library):
    #    INTERFACE_LIBRARY targets may only have whitelisted properties.  The
    #    property "QT_PLUGIN_CLASS_NAME" is not allowed.
    #  Call Stack (most recent call first):
    #    src/designer/src/uiplugin/CMakeLists.txt:7 (qt_internal_add_module)
    if(NOT target_type STREQUAL "INTERFACE_LIBRARY")
        set(target_defines_genex
            "$<TARGET_PROPERTY:${module_target},${prop_prefix}COMPILE_DEFINITIONS>")
        set(defines_exist_genex "$<BOOL:${target_defines_genex}>")
        set(target_defines_joined_genex
            "$<${defines_exist_genex}:-D$<JOIN:${target_defines_genex},;-D>>")
    endif()

    # TODO: FIXME
    # Passing COMPILE_OPTIONS can break add_custom_command() if the values contain genexes
    # that add_custom_command does not support.
    #
    # Such a case happens on Linux where libraries linking against Threads::Threads bring in a
    # '<$<COMPILE_LANG_AND_ID:CUDA,NVIDIA>' genex.
    #
    # If this is really required for headersclean (and qmake's headerclean implementation does
    # actually pass all flags of the associated target), we'll have to replace the genex usage
    # with an implementation that recursively processes a target's dependencies property
    # to compile an expanded list of values for COMPILE_OPTIONS, and then filter out any genexes.
    #
    # This is similar to the proposed workaround at
    # https://gitlab.kitware.com/cmake/cmake/-/issues/21074#note_814979
    #
    # See also https://gitlab.kitware.com/cmake/cmake/-/issues/21336
    #
    #set(target_compile_options_genex "$<TARGET_PROPERTY:${module_target},COMPILE_OPTIONS>")
    #set(compile_options_exist_genex "$<BOOL:${target_compile_options_genex}>")
    #set(target_compile_options_joined_genex
    #    "$<${compile_options_exist_genex}:$<JOIN:${target_compile_options_genex},;>>")

    set(target_compile_flags_genex
        "$<TARGET_PROPERTY:${module_target},${prop_prefix}COMPILE_FLAGS>")
    set(compile_flags_exist_genex "$<BOOL:${target_compile_flags_genex}>")
    set(target_compile_flags_joined_genex
        "$<${compile_flags_exist_genex}:$<JOIN:${target_compile_flags_genex},;>>")

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU"
            OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang"
            OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
        # Turn on some extra warnings not found in -Wall -Wextra.

        set(hcleanFLAGS -Wall -Wextra -Werror -Woverloaded-virtual -Wshadow -Wundef -Wfloat-equal
            -Wnon-virtual-dtor -Wpointer-arith -Wformat-security -Wno-long-long -Wno-variadic-macros
            -pedantic-errors)

        if(QT_FEATURE_reduce_relocations AND UNIX)
            list(APPEND hcleanFLAGS -fPIC)
        endif()

        if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
            # these warnings are disabled because explicit constructors with zero or
            # multiple arguments are permitted in C++11:
            # 2304: non-explicit constructor with single argument may cause implicit type
            #       conversion
            # 2305: declaration of 'explicit' constructor without a single argument is
            #       redundant
            #
            # ICC 14+ has a bug with -Wshadow, emitting it for cases where there's no
            # shadowing (issue ID 0000698329, task DPD200245740)
            list(APPEND hcleanFLAGS -wd2304,2305 -Wshadow)
        else()
            # options accepted by GCC and Clang
            list(APPEND hcleanFLAGS -Wchar-subscripts -Wold-style-cast)

            if (NOT ((TEST_architecture_arch STREQUAL arm)
                    OR (TEST_architecture_arch STREQUAL mips)))
                list(APPEND hcleanFLAGS -Wcast-align)
            endif()

            if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
                list(APPEND hcleanFLAGS -Wzero-as-null-pointer-constant)
                if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 4.5)
                    list(APPEND hcleanFLAGS -Wdouble-promotion)
                endif()
                if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 4.9)
                    list(APPEND hcleanFLAGS -Wfloat-conversion)

                    # GCC 9 has a lot of false positives relating to these
                    list(APPEND hcleanFlags -Wno-deprecated-copy -Wno-redundant-move
                        -Wno-format-overflow -Wno-init-list-lifetime)
                endif()
            endif()
        endif()

        # Use strict mode C++17, with no GNU extensions (see -pedantic-errors above).
        list(APPEND hcleanFLAGS -std=c++17)

        set(cxx_flags ${CMAKE_CXX_FLAGS})

        if(APPLE AND CMAKE_OSX_SYSROOT)
            list(APPEND cxx_flags "${CMAKE_CXX_SYSROOT_FLAG}" "${CMAKE_OSX_SYSROOT}")
        endif()

        if(APPLE AND QT_FEATURE_framework)
            # For some reason CMake doesn't generate -iframework flags from the INCLUDE_DIRECTORIES
            # generator expression we provide, so pass it explicitly and hope for the best.
            list(APPEND framework_includes
                 "-iframework" "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_LIBDIR}")
        endif()

        foreach(header ${hclean_headers})
            get_filename_component(input_path "${header}" ABSOLUTE)
            set(artifact_path "header_check/${header}.o")
            get_filename_component(artifact_directory "${artifact_path}" DIRECTORY)
            set(comment_header_path "${CMAKE_CURRENT_SOURCE_DIR}/${header}")
            file(RELATIVE_PATH comment_header_path "${PROJECT_SOURCE_DIR}" "${comment_header_path}")

            add_custom_command(
                OUTPUT "${artifact_path}"
                COMMENT "headersclean: Checking header ${comment_header_path}"
                COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}"
                COMMAND
                ${compiler_to_run} -c ${cxx_flags}
                "${target_compile_flags_joined_genex}"
                "${target_defines_joined_genex}"
                ${hcleanFLAGS}
                "${target_includes_joined_genex}"
                ${framework_includes}
                ${hcleanDEFS}
                -xc++ "${input_path}"
                -o${artifact_path}
                IMPLICIT_DEPENDS CXX
                VERBATIM
                COMMAND_EXPAND_LISTS)
            list(APPEND hclean_artifacts "${artifact_path}")
        endforeach()

    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        # -Za would enable strict standards behavior, but we can't add it because
        # <windows.h> and <GL.h> violate the standards.
        set(hcleanFLAGS -std:c++latest -Zc:__cplusplus -WX -W3)

        # cl.exe needs a source path
        get_filename_component(source_path "${QT_MKSPECS_DIR}/features/data/dummy.cpp" REALPATH)

        foreach(header ${hclean_headers})
            # We need realpath here to make sure path starts with drive letter
            get_filename_component(input_path "${header}" REALPATH)
            set(artifact_path "header_${header}.o")
            set(comment_header_path "${CMAKE_CURRENT_SOURCE_DIR}/${header}")
            file(RELATIVE_PATH comment_header_path "${PROJECT_SOURCE_DIR}" "${comment_header_path}")

            add_custom_command(
                OUTPUT "${artifact_path}"
                COMMENT "headersclean: Checking header ${comment_header_path}"
                COMMAND
                ${compiler_to_run} -nologo -c ${CMAKE_CXX_FLAGS}
                "${target_compile_flags_joined_genex}"
                "${target_defines_joined_genex}"
                ${hcleanFLAGS}
                "${target_includes_joined_genex}"
                ${hcleanDEFS}
                -FI "${input_path}"
                -Fo${artifact_path} "${source_path}"
                IMPLICIT_DEPENDS CXX
                VERBATIM
                COMMAND_EXPAND_LISTS)
            list(APPEND hclean_artifacts "${artifact_path}")
        endforeach()
    else()
        message(ERROR "CMAKE_CXX_COMPILER_ID \"${CMAKE_CXX_COMPILER_ID}\" is not supported for the headersclean check.")
    endif()

    add_custom_target(${module_include_name}_header_check
        COMMENT "headersclean: Checking headers in ${module_include_name}"
        DEPENDS ${hclean_artifacts}
        VERBATIM)

    add_dependencies(${module_target} ${module_include_name}_header_check)
endfunction()
