include(QtRunCMake)

function(run_cmake_and_build case)
    # Set common build directory for configure and build
    set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/${case}-build)
    run_cmake_with_options(${case} "-DQt6_DIR=${Qt6_DIR}")
    # Do not remove the current RunCMake_TEST_BINARY_DIR
    set(RunCMake_TEST_NO_CLEAN 1)
    run_cmake_command(${case}-build ${CMAKE_COMMAND} --build .)
endfunction()

run_cmake_and_build(find_package_basic)

# Qt found in a function is not visible to the deferred finalizer. Expect a clear error
# instead of a build failure that references "::moc".
run_cmake_with_options(find_package_in_function "-DQt6_DIR=${Qt6_DIR}")
