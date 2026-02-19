include(QtRunCMake)

set(cmake_opts "-DQt6_DIR=${Qt6_DIR}")

# Test that I18N_SOURCE_LANGUAGE changes CFBundleDevelopmentRegion.
set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/source-language-de-build")
run_cmake_with_options(source-language-de ${cmake_opts})

set(RunCMake_TEST_NO_CLEAN TRUE)
run_cmake_command(source-language-de-build ${CMAKE_COMMAND} --build .)

# Test that I18N_SOURCE_LANGUAGE without I18N_TRANSLATED_LANGUAGES still works.
set(RunCMake_TEST_NO_CLEAN FALSE)
set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/source-language-only-build")
run_cmake_with_options(source-language-only ${cmake_opts})

set(RunCMake_TEST_NO_CLEAN TRUE)
run_cmake_command(source-language-only-build ${CMAKE_COMMAND} --build .)

# Test that the default I18N_SOURCE_LANGUAGE ("en") keeps CFBundleDevelopmentRegion as "en".
set(RunCMake_TEST_NO_CLEAN FALSE)
set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/default-language-build")
run_cmake_with_options(default-language ${cmake_opts})

set(RunCMake_TEST_NO_CLEAN TRUE)
run_cmake_command(default-language-build ${CMAKE_COMMAND} --build .)
