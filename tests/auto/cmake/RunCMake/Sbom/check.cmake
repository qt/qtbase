function(check_exists file)
    if(NOT EXISTS "${file}")
        get_filename_component(file_dir "${file}" DIRECTORY)
        file(GLOB dir_contents "${file_dir}/*")
        string(APPEND RunCMake_TEST_FAILED "${file} does not exist\n. "
            "Contents of directory ${file_dir}:\n ${dir_contents}\n")
    endif()
    set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
endfunction()

function(check_not_exists file)
    if(EXISTS "${file}")
        get_filename_component(file_dir "${file}" DIRECTORY)
        file(GLOB dir_contents "${file_dir}/*")
        string(APPEND RunCMake_TEST_FAILED "${file} exists\n. "
            "Contents of directory ${file_dir}:\n ${dir_contents}\n")
    endif()
    set(RunCMake_TEST_FAILED "${RunCMake_TEST_FAILED}" PARENT_SCOPE)
endfunction()

# Check that the correct option values are used for the project root sbom.
set(root_result_file "${RunCMake_TEST_BINARY_DIR}/result.cmake")
if(EXISTS "${root_result_file}")
    include("${root_result_file}")

    if(NOT "${ORIGINAL_QT_GENERATE_SBOM}" STREQUAL "${RESULT_QT_GENERATE_SBOM}")
        string(APPEND RunCMake_TEST_FAILED
            "QT_GENERATE_SBOM is ${RESULT_QT_GENERATE_SBOM}, expected ${ORIGINAL_QT_GENERATE_SBOM} \n")
    endif()

    if(NOT "${ORIGINAL_QT_SBOM_GENERATE_SPDX_V2}" STREQUAL "${RESULT_QT_SBOM_GENERATE_SPDX_V2}")
        string(APPEND RunCMake_TEST_FAILED
            "QT_SBOM_GENERATE_SPDX_V2 is ${RESULT_QT_SBOM_GENERATE_SPDX_V2}, "
            "expected ${ORIGINAL_QT_SBOM_GENERATE_SPDX_V2} \n")
    endif()

    if(NOT "${ORIGINAL_QT_SBOM_GENERATE_CYDX_V1_6}" STREQUAL "${RESULT_QT_SBOM_GENERATE_CYDX_V1_6}")
        string(APPEND RunCMake_TEST_FAILED
            "QT_SBOM_GENERATE_CYDX_V1_6 is ${RESULT_QT_SBOM_GENERATE_CYDX_V1_6}, "
            "expected ${ORIGINAL_QT_SBOM_GENERATE_CYDX_V1_6} \n")
    endif()
endif()


# Glob for all result.cmake files recursively in the root of the test binary dir, and run checks
# for each of them.
file(GLOB_RECURSE result_files
    "${RunCMake_TEST_BINARY_DIR}/**/result.cmake"
)

# Confirm that the all subproject sbom files are installed, including the root one.
foreach(result_file IN LISTS result_files)
    include("${result_file}")

    foreach(sbom_doc IN LISTS SBOM_DOCUMENTS)
        check_exists("${sbom_doc}")
    endforeach()

    foreach(sbom_doc IN LISTS NO_SBOM_DOCUMENTS)
        check_not_exists("${sbom_doc}")
    endforeach()
endforeach()

