if(NOT SBOM_PROJECT_NAME)
    set(SBOM_PROJECT_NAME "${PROJECT_NAME}")
endif()
# Convert to lower case, otherwise on case-sensitive filesystems the generated
# filenames may not match the expected ones.
string(TOLOWER "${SBOM_PROJECT_NAME}" SBOM_PROJECT_NAME)

if(NOT SBOM_VERSION)
    set(SBOM_VERSION "1.0.0")
endif()

if(NOT SBOM_INSTALL_DIR)
    set(SBOM_INSTALL_DIR "sbom")
endif()

set(sbom_document_base_name "${SBOM_PROJECT_NAME}-${SBOM_VERSION}")
set(sbom_install_dir "${CMAKE_BINARY_DIR}/installed/${SBOM_INSTALL_DIR}")

set(spdx_file "${sbom_install_dir}/${sbom_document_base_name}.spdx")
set(spdx_json_file "${sbom_install_dir}/${sbom_document_base_name}.spdx.json")
set(cydx_file "${sbom_install_dir}/${sbom_document_base_name}.cdx.json")

set(sbom_documents "")
set(no_sbom_documents "")

if(FORMAT_CASE STREQUAL "spdx23" OR FORMAT_CASE STREQUAL "all")
    if(QT_SBOM_GENERATE_SPDX_V2)
        list(APPEND sbom_documents "${spdx_file}")
    else()
        list(APPEND no_sbom_documents "${spdx_file}")
    endif()

    if(QT_SBOM_GENERATE_SPDX_V2_JSON)
        list(APPEND sbom_documents "${spdx_json_file}")
    else()
        list(APPEND no_sbom_documents "${spdx_json_file}")
    endif()
endif()

if(FORMAT_CASE STREQUAL "cydx16" OR FORMAT_CASE STREQUAL "all")
    if(QT_SBOM_GENERATE_CYDX_V1_6)
        list(APPEND sbom_documents "${cydx_file}")
    else()
        list(APPEND no_sbom_documents "${cydx_file}")
    endif()
endif()

if(FORMAT_CASE STREQUAL "none")
    set(no_sbom_documents ${spdx_file} ${spdx_json_file} ${cydx_file})
    set(sbom_documents "")
endif()

# These values will be used by the check.cmake script after installation.
file(GENERATE
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/result.cmake"
    CONTENT
        "
set(SBOM_DOCUMENTS \"${sbom_documents}\")
set(NO_SBOM_DOCUMENTS \"${no_sbom_documents}\")
set(ORIGINAL_QT_GENERATE_SBOM \"${original_QT_GENERATE_SBOM}\")
set(ORIGINAL_QT_SBOM_GENERATE_SPDX_V2 \"${original_QT_SBOM_GENERATE_SPDX_V2}\")
set(ORIGINAL_QT_SBOM_GENERATE_CYDX_V1_6 \"${original_QT_SBOM_GENERATE_CYDX_V1_6}\")
set(RESULT_QT_GENERATE_SBOM \"${QT_GENERATE_SBOM}\")
set(RESULT_QT_SBOM_GENERATE_SPDX_V2 \"${QT_SBOM_GENERATE_SPDX_V2}\")
set(RESULT_QT_SBOM_GENERATE_CYDX_V1_6 \"${QT_SBOM_GENERATE_CYDX_V1_6}\")
"
)
