# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include(QtRunCMake)

set(find_package_opts "-DQt6_DIR=${Qt6_DIR}")

run_cmake_with_options(glx_missing_egl_requested ${find_package_opts})

# FindOpenGL has GLES2 and GLES3 components since CMake 3.27.
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.27")
    run_cmake_with_options(gles3_missing_gles3_requested ${find_package_opts})
    run_cmake_with_options(glx_missing_gles3_requested ${find_package_opts})

    set(RunCMake_TEST_EXPECT_RESULT 1)
    set(RunCMake_TEST_EXPECT_stderr "dependency WrapOpenGL could not be found")
    run_cmake_with_options(gles3_missing_cache_var_used
        ${find_package_opts} -DOPENGL_USE_GLES3=ON)
    run_cmake_with_options(gles3_missing_opt_out
        ${find_package_opts} -DQT_NO_OVERRIDE_OPENGL_USE_VARIABLES=ON)
endif()
