# Needed to make the sbom functions available.
find_package(Qt6 REQUIRED Core)

sbom_test_begin()

_qt_internal_setup_sbom(
    GENERATE_SBOM_DEFAULT "TRUE"
)

include(CMakePackageConfigHelpers)
function(create_sbom_lib_target target)
    add_library(${target} STATIC)
    target_sources(${target} PRIVATE sources/utils_helper.cpp)
    install(TARGETS ${target}
        EXPORT MainTargets
        ARCHIVE DESTINATION lib
    )
    _qt_internal_add_sbom(${target}
        TYPE "LIBRARY"
        RUNTIME_PATH bin
        ARCHIVE_PATH lib
        LIBRARY_PATH lib
    )

    # The installation bits are needed for the follow-up test which checks 'DocumentRef-' values.
    if(INSTALL_EXPORT_TARGETS)
        install(EXPORT MainTargets
            NAMESPACE Main::
            DESTINATION lib/cmake/Main
        )
        configure_package_config_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/MainConfig.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/MainConfig.cmake"
            INSTALL_DESTINATION lib/cmake/Main
        )

        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/MainConfig.cmake
                DESTINATION lib/cmake/Main )
    endif()

    # This is used the by spdx and cydx deps checking code.
    _qt_internal_sbom_get_spdx_id_for_target(${target} ${target}_spdx_id)
    set(${target}_spdx_id "${${target}_spdx_id}" PARENT_SCOPE)
endfunction()

macro(set_common_sbom_begin_args out_var)
    set(${out_var}
        SBOM_PROJECT_NAME "${SBOM_PROJECT_NAME}"
        SUPPLIER "${SBOM_SUPPLIER}"
        SUPPLIER_URL "${SBOM_SUPPLIER_URL}"
        VERSION "${SBOM_VERSION}"
    )
endmacro()

macro(check_unique_suffix_in_spdx_id target unique_suffix)
    if(QT_GENERATE_SBOM)
        if(NOT "${${target}_spdx_id}" MATCHES "${unique_suffix}")
            message(FATAL_ERROR
                "Expected unique suffix '${unique_suffix}' to be part of the spdx id for target "
                "'${target}', but got '${${target}_spdx_id}'")
        endif()
    endif()
endmacro()

# Macro to check spdx id of target exactly ends with the given string
macro(check_spdx_id_for_target_ends_with target expected_ending)
    if(QT_GENERATE_SBOM)
        if(NOT "${${target}_spdx_id}" MATCHES "${expected_ending}$")
            message(FATAL_ERROR
                "Expected spdx id '${${target}_spdx_id}' for target '${target}' to end with "
                "'${expected_ending}', but it did not.")
        endif()
    endif()
endmacro()

set(SBOM_PROJECT_NAME "001-auto-suffix")
set(SBOM_SUPPLIER "QtProjectTest")
set(SBOM_SUPPLIER_URL "https://qt-project.org/SbomTest")

# Case 1, default auto added hash suffix
set(SBOM_VERSION "1.0.0")
set_common_sbom_begin_args(sbom_begin_args)
_qt_internal_sbom_begin_project(
    ${sbom_begin_args}
)
sbom_test_record_project()
_qt_internal_sbom_get_spdx_id_unique_suffix(unique_suffix)
create_sbom_lib_target(lib_001)
check_unique_suffix_in_spdx_id(lib_001 "${unique_suffix}")
_qt_internal_sbom_end_project()

# Case 2, no auto spdx suffix
set(SBOM_PROJECT_NAME "002-no-suffix")
set_common_sbom_begin_args(sbom_begin_args)
_qt_internal_sbom_begin_project(
    ${sbom_begin_args}
    NO_AUTO_SPDX_ID_SUFFIX
)
sbom_test_record_project()
_qt_internal_sbom_get_spdx_id_unique_suffix(unique_suffix)
if(QT_GENERATE_SBOM AND NOT "${unique_suffix}" STREQUAL "")
    message(FATAL_ERROR
        "Expected unique suffix to be empty when NO_AUTO_SPDX_ID_SUFFIX is used, but got "
        "'${unique_suffix}'")
endif()
create_sbom_lib_target(lib_002)
check_spdx_id_for_target_ends_with(lib_002 "lib-002")
_qt_internal_sbom_end_project()

# Case 3, custom spdx suffix
set(SBOM_PROJECT_NAME "003-custom-suffix")
set_common_sbom_begin_args(sbom_begin_args)
_qt_internal_sbom_begin_project(
    ${sbom_begin_args}
    SPDX_ID_SUFFIX "my-custom-suffix"
)
sbom_test_record_project()
_qt_internal_sbom_get_spdx_id_unique_suffix(unique_suffix)
if(QT_GENERATE_SBOM AND NOT "${unique_suffix}" STREQUAL "-my-custom-suffix")
    message(FATAL_ERROR
        "Expected unique suffix to be '-my-custom-suffix', but got '${unique_suffix}'")
endif()
# Also install the target, to query its DocumentRef- id in a follow-up test.
set(INSTALL_EXPORT_TARGETS ON)
create_sbom_lib_target(lib_003)
check_unique_suffix_in_spdx_id(lib_003 "${unique_suffix}")
_qt_internal_sbom_end_project()
unset(INSTALL_EXPORT_TARGETS)

# Case 4, global no auto add suffix
set(SBOM_PROJECT_NAME "004-global-no-auto-suffix")
set_common_sbom_begin_args(sbom_begin_args)
set(QT_SBOM_NO_AUTO_SPDX_SUFFIX ON)
_qt_internal_sbom_begin_project(
    ${sbom_begin_args}
)
sbom_test_record_project()
_qt_internal_sbom_get_spdx_id_unique_suffix(unique_suffix)
if(QT_GENERATE_SBOM AND NOT "${unique_suffix}" STREQUAL "")
    message(FATAL_ERROR
        "Expected unique suffix to be empty when QT_SBOM_NO_AUTO_SPDX_SUFFIX is set, but got "
        "'${unique_suffix}'")
endif()
create_sbom_lib_target(lib_004)
check_spdx_id_for_target_ends_with(lib_004 "lib-004")
_qt_internal_sbom_end_project()
unset(QT_SBOM_NO_AUTO_SPDX_SUFFIX)

# Case 5, fake deterministic id
set(SBOM_PROJECT_NAME "005-deterministic")
set_common_sbom_begin_args(sbom_begin_args)
set(QT_SBOM_FAKE_DETERMINISTIC_BUILD ON)
_qt_internal_sbom_begin_project(
    ${sbom_begin_args}
)
sbom_test_record_project()
_qt_internal_sbom_get_spdx_id_unique_suffix(unique_suffix)
if(QT_GENERATE_SBOM AND NOT "${unique_suffix}" STREQUAL "-fake-id-suffix")
    message(FATAL_ERROR
        "Expected unique suffix to be '-fake-id-suffix', but got '${unique_suffix}'")
endif()
create_sbom_lib_target(lib_005)
check_unique_suffix_in_spdx_id(lib_005 "${unique_suffix}")
_qt_internal_sbom_end_project()
unset(QT_SBOM_FAKE_DETERMINISTIC_BUILD)

# Case 6, shorter hash length
set(SBOM_PROJECT_NAME "006-short-hash")
set_common_sbom_begin_args(sbom_begin_args)
_qt_internal_sbom_begin_project(
    ${sbom_begin_args}
    SPDX_ID_SUFFIX_HASH_LENGTH 4
)
sbom_test_record_project()
_qt_internal_sbom_get_spdx_id_unique_suffix(unique_suffix)
string(LENGTH "${unique_suffix}" unique_suffix_length)
if(QT_GENERATE_SBOM AND NOT unique_suffix_length EQUAL 5)
    message(FATAL_ERROR
        "Expected unique suffix length to be 5 (4 chars + the preceding dash) when "
        "SPDX_ID_HASH_LENGTH is set to 4, but got "
        "'${unique_suffix}' with length ${unique_suffix_length}")
endif()
_qt_internal_sbom_end_project()

sbom_test_end()
