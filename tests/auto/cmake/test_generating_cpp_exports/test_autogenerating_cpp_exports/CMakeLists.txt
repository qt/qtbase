# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_module(TestAutogeneratingCppExports
    NO_UNITY_BUILD
    GENERATE_CPP_EXPORTS
    SOURCES
        module_api.h
        module_api.cpp
    PUBLIC_LIBRARIES
        Qt::Core
)

qt_internal_extend_target(TestAutogeneratingCppExports
    CONDITION GCC OR MINGW
    COMPILE_OPTIONS
        -fvisibility=hidden
)

add_executable(TestAutogeneratingCppExportsApp use_api.cpp)
target_link_libraries(TestAutogeneratingCppExportsApp PRIVATE TestAutogeneratingCppExports)
set_target_properties(TestAutogeneratingCppExportsApp PROPERTIES UNITY_BUILD OFF)
