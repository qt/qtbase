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

        foreach(header ${hclean_headers})
            get_filename_component(input_path "${header}" ABSOLUTE)
            set(artifact_path "header_${header}.o")

            add_custom_command(
                OUTPUT "${artifact_path}"
                COMMENT "headersclean: Checking header ${header}"
                COMMAND "${CMAKE_CXX_COMPILER}" -c ${cxx_flags} ${hcleanFLAGS}
                -I "${QT_BUILD_DIR}/include" -I "${CMAKE_INSTALL_PREFIX}/include"
                ${hcleanDEFS} -xc++ "${input_path}"
                -o${artifact_path}
                IMPLICIT_DEPENDS CXX
                VERBATIM)
            list(APPEND hclean_artifacts "${artifact_path}")
        endforeach()

    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        # -Za would enable strict standards behavior, but we can't add it because
        # <windows.h> and <GL.h> violate the standards.
        set(hcleanFLAGS -std:c++17 -WX -W3)

        # cl.exe needs a source path
        get_filename_component(source_path "${QT_MKSPECS_DIR}/features/data/dummy.cpp" REALPATH)

        foreach(header ${hclean_headers})
            # We need realpath here to make sure path starts with drive letter
            get_filename_component(input_path "${header}" REALPATH)
            set(artifact_path "header_${header}.o")

            add_custom_command(
                OUTPUT "${artifact_path}"
                COMMENT "headersclean: Checking header ${header}"
                COMMAND "${CMAKE_CXX_COMPILER}" -nologo -c ${CMAKE_CXX_FLAGS} ${hcleanFLAGS}
                        -I "${QT_BUILD_DIR}/include" -I "${CMAKE_INSTALL_PREFIX}/include"
                        ${hcleanDEFS} -FI "${input_path}"
                        -Fo${artifact_path} "${source_path}"
                IMPLICIT_DEPENDS CXX
                VERBATIM)
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
