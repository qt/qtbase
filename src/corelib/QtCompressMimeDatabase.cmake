# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
cmake_minimum_required(VERSION 3.16)

get_filename_component(output_directory "${OUTPUT_FILE}" DIRECTORY)
file(MAKE_DIRECTORY "${output_directory}")

find_program(xmlstarlet NAMES xmlstarlet xml)
if(xmlstarlet)
    set(XML_STARLET_COMMAND COMMAND "${xmlstarlet}" sel -D -B -t -c / "${INPUT_FILE}")
else()
    get_filename_component(mime_types_resource_filename "${INPUT_FILE}" NAME)
    message(STATUS "xmlstarlet command was not found. ${mime_types_resource_filename}"
        " will not be minified.")
endif()

# The function tries to minify 'input_file' using the globally defined 'XML_STARLET_COMMAND'.
# Sets the full path of the output file to the 'out_var' argument. If 'XML_STARLET_COMMAND' fails
# to minify the 'input_file', 'out_var' is set to 'input_file'.
function(try_minify_xml out_var input_file output_directory)
    get_filename_component(output_filename "${input_file}" NAME)
    set(minified_output "${output_directory}/${output_filename}")

    # Assume XML_STARLET_COMMAND is globally defined.
    if(XML_STARLET_COMMAND)
        execute_process(${XML_STARLET_COMMAND}
            OUTPUT_FILE "${minified_output}"
            RESULT_VARIABLE failed_to_minify
        )
        if(NOT failed_to_minify)
            set(${out_var} "${minified_output}" PARENT_SCOPE)
            return()
        endif()
    endif()
    set(${out_var} "${input_file}" PARENT_SCOPE)
endfunction()

unset(archive_command)

if(COMPRESSION_TYPE STREQUAL "none")
    try_minify_xml(qmimeprovider_db_data_file "${INPUT_FILE}" "${output_directory}")
else()
    # CMake versions less than 3.26 don't support the value of the zstd compression level higher
    # than 9. We want zstd to compress with level 19 if it's possible, so if zstd executable is
    # found on a file system we prefer to use the 'External' archiving API instead of the CMake API.
    # See for details: https://gitlab.kitware.com/cmake/cmake/-/issues/24160
    if(COMPRESSION_TYPE STREQUAL "zstd" AND CMAKE_VERSION VERSION_LESS 3.26)
        find_program(archive_command NAMES zstd zstd.exe)
        if(archive_command)
            set(ARCHIVING_API "External")
        endif()
    endif()

    set(qmimeprovider_db_data_file "${OUTPUT_FILE}.archive")
    if(ARCHIVING_API STREQUAL "External")
        message(STATUS "Using external archive command to compress the mime type database")
        if(COMPRESSION_TYPE STREQUAL "zstd")
            find_program(archive_command NAMES zstd zstd.exe)
            if(archive_command)
                set(archive_command_args -cq19 -T1)
            else()
                message(WARNING
                    "Unable to use the preffered ${COMPRESSION_TYPE} compression for the mime type database."
                    " ${COMPRESSION_TYPE} binary is not found. Trying gzip.")
            endif()
        endif()
        if(NOT archive_command)
            # Trying to use gzip if zstd is binary is not found.
            set(COMPRESSION_TYPE "gzip")
            find_program(archive_command NAMES gzip gzip.exe)
            set(archive_command_args -nc9)
        endif()
        if(archive_command)
            if(NOT XML_STARLET_COMMAND)
                set(intput_file_arg INPUT_FILE "${INPUT_FILE}")
            endif()
            execute_process(${XML_STARLET_COMMAND}
                COMMAND ${archive_command}
                ${archive_command_args}
                ${intput_file_arg}
                OUTPUT_FILE "${qmimeprovider_db_data_file}"
                RESULTS_VARIABLE results
                ERROR_VARIABLE error_string
            )

            foreach(result IN LISTS results)
                if(NOT result EQUAL 0)
                    message(WARNING "Unable to compress mime type database: ${error_string}")
                endif()
            endforeach()
        else()
            message(WARNING "Unable to find ${COMPRESSION_TYPE} binary."
                " Please make sure that the ${COMPRESSION_TYPE} binary is in PATH."
                " Adding the uncompressed mime type database.")
            set(COMPRESSION_TYPE "none")
            set(qmimeprovider_db_data_file "${INPUT_FILE}")
        endif()
    else()
        message(STATUS "Using CMake archive command to compress the mime type database")

        try_minify_xml(mimetypes_resource_file_minified "${INPUT_FILE}" "${output_directory}")

        if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.26 AND COMPRESSION_TYPE STREQUAL "zstd")
            set(additional_file_archive_create_parameters COMPRESSION_LEVEL 19)
        elseif(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
            set(additional_file_archive_create_parameters COMPRESSION_LEVEL 9)
        endif()

        if(COMPRESSION_TYPE STREQUAL "gzip")
            set(cmake_compression_type "GZip")
        elseif(COMPRESSION_TYPE STREQUAL "zstd")
            set(cmake_compression_type "Zstd")
        endif()

        file(ARCHIVE_CREATE OUTPUT "${qmimeprovider_db_data_file}"
            PATHS "${mimetypes_resource_file_minified}"
            FORMAT raw
            COMPRESSION ${cmake_compression_type}
            ${additional_file_archive_create_parameters}
        )
        if(NOT mimetypes_resource_file_minified STREQUAL INPUT_FILE)
            file(REMOVE "${mimetypes_resource_file_minified}")
        endif()
    endif()
endif()
file(READ "${qmimeprovider_db_data_file}" qmime_db_data HEX)
file(SIZE "${qmimeprovider_db_data_file}" qmime_db_data_size)
if(NOT qmimeprovider_db_data_file STREQUAL INPUT_FILE)
    file(SIZE "${INPUT_FILE}" qmime_db_resource_size)
    file(REMOVE "${qmimeprovider_db_data_file}")
else()
    set(qmime_db_resource_size ${qmime_db_data_size})
endif()

string(REGEX MATCHALL "([a-f0-9][a-f0-9])" qmime_db_hex "${qmime_db_data}")

list(TRANSFORM qmime_db_hex PREPEND "0x")
math(EXPR qmime_db_data_size "${qmime_db_data_size} - 1")
foreach(index RANGE 0 ${qmime_db_data_size} 12)
    list(APPEND index_list ${index})
endforeach()
list(TRANSFORM qmime_db_hex PREPEND "\n " AT ${index_list})
list(JOIN qmime_db_hex ", " qmime_db_hex_joined)

if(NOT COMPRESSION_TYPE STREQUAL "none")
    string(TOUPPER "${COMPRESSION_TYPE}" compression_type_upper)
    set(qmime_db_content "#define MIME_DATABASE_IS_${compression_type_upper}\n")
endif()
string(APPEND qmime_db_content
    "static const unsigned char mimetype_database[] = {"
    "${qmime_db_hex_joined}"
    "\n};\n"
    "static constexpr size_t MimeTypeDatabaseOriginalSize = ${qmime_db_resource_size};\n"
)

file(WRITE "${OUTPUT_FILE}" "${qmime_db_content}")
