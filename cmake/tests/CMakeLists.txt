# These macros are taken from the ECM:

# a macro for tests that have a simple format where the name matches the
# directory and project
macro(add_test_variant NAME BASE COMMAND)
    string(REPLACE "." "/" src_dir "${BASE}")
    string(REPLACE "." "/" build_dir "${NAME}")
    string(REGEX REPLACE "[^.]*\\." "" proj "${NAME}")
    add_test("cmake_${NAME}" ${CMAKE_CTEST_COMMAND}
        --build-and-test
            "${CMAKE_CURRENT_SOURCE_DIR}/${src_dir}"
            "${CMAKE_CURRENT_BINARY_DIR}/${build_dir}"
        --build-two-config
        --build-generator ${CMAKE_GENERATOR}
        --build-makeprogram ${CMAKE_MAKE_PROGRAM}
        --build-project ${proj}
        ${${NAME}_EXTRA_OPTIONS}
        --test-command ${COMMAND} ${ARGN})
endmacro()

macro(add_test_macro NAME)
    add_test_variant("${NAME}" "${NAME}" ${ARGN})
endmacro()

add_test_macro(features features)
