include(QtRunCMake)

set(find_package_opts "-DQt6_DIR=${Qt6_DIR}")

run_cmake_with_options(qt_internal_add_compiler_dependent_flags_pass ${find_package_opts})
# Failing cases
set(RunCMake_TEST_EXPECT_RESULT 1)
# Missing OPTIONS keyword
foreach(case IN ITEMS
    missing_OPTIONS
    missing_COMPILERS
    empty_COMPILERS
    invalid_CONDITIONS
)
    set(RunCMake_TEST_EXPECT_stdout "case: ${case}")
    if(case STREQUAL "missing_OPTIONS")
        set(RunCMake_TEST_EXPECT_stderr "OPTIONS keyword was not passed")
    elseif(case STREQUAL "missing_COMPILERS")
        set(RunCMake_TEST_EXPECT_stderr "COMPILERS keyword was not passed")
    elseif(case STREQUAL "empty_COMPILERS")
        set(RunCMake_TEST_EXPECT_stderr "COMPILERS set cannot be empty")
    elseif(case STREQUAL "invalid_CONDITIONS")
        set(RunCMake_TEST_EXPECT_stderr "Unrecognized condition form: not_a_genex")
    endif()
    run_cmake_with_options(qt_internal_add_compiler_dependent_flags_fail
        -Dcase=${case} ${find_package_opts})
endforeach()
