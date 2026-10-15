# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(harmonynativewindowid LANGUAGES CXX)

if(NOT OHOS)
    message(FATAL_ERROR "Example only works on HarmonyOS")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(NodeAddonApi REQUIRED)

qt_standard_project_setup()

qt_add_executable(harmonynativewindowid
    main.cpp
)

target_compile_definitions(harmonynativewindowid PRIVATE
    NAPI_CPP_EXCEPTIONS
    NODE_ADDON_API_ENABLE_TYPE_CHECK_ON_AS
)

target_link_libraries(harmonynativewindowid PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    NodeAddonApi::NodeAddonApi
    ace_napi.z
)

install(TARGETS harmonynativewindowid
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET harmonynativewindowid
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
