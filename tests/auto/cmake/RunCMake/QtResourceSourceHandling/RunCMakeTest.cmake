include(QtRunCMake)

set(cmake_opts "-DQt6_DIR=${Qt6_DIR}")

# A .qrc added as a target source without rcc running on it warns.
set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/warn-build")
run_cmake_with_options(warn ${cmake_opts})

# The warning is silenced by QT_NO_UNHANDLED_QRC_SOURCE_WARNING.
set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/silenced-variable-build")
run_cmake_with_options(silenced-variable ${cmake_opts})

# A .qrc marked HEADER_FILE_ONLY is for IDE visibility only and does not warn.
set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/header-file-only-build")
run_cmake_with_options(header-file-only ${cmake_opts})

# With AUTORCC enabled, rcc runs on the .qrc, so there is no warning.
set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/autorcc-build")
run_cmake_with_options(autorcc ${cmake_opts})

# A .qrc consumed by qt_add_resources() is handled and does not warn.
set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/handled-build")
run_cmake_with_options(handled ${cmake_opts})
