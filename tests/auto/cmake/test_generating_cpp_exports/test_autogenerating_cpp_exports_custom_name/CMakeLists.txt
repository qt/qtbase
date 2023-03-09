# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_module(TestAutogeneratingCppExportsCustomName
    NO_UNITY_BUILD
    GENERATE_CPP_EXPORTS
    CPP_EXPORT_HEADER_BASE_NAME
        "customname_exports"
    SOURCES
        module_api.h
        module_api.cpp
    PUBLIC_LIBRARIES
        Qt::Core
)

qt_internal_extend_target(TestAutogeneratingCppExportsCustomName
    CONDITION GCC OR MINGW
    COMPILE_OPTIONS
        -fvisibility=hidden
)

add_executable(testapp2 use_api.cpp)
target_link_libraries(testapp2 PRIVATE TestAutogeneratingCppExportsCustomName)
set_target_properties(testapp2 PROPERTIES UNITY_BUILD OFF)
