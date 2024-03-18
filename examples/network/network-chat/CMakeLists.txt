# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(network-chat LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Widgets)

qt_standard_project_setup()

qt_add_executable(network-chat
    chatdialog.cpp chatdialog.h chatdialog.ui
    client.cpp client.h
    connection.cpp connection.h
    main.cpp
    peermanager.cpp peermanager.h
    server.cpp server.h
)

set_target_properties(network-chat PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_compile_definitions(network-chat PRIVATE
    QT_USE_QSTRINGBUILDER
)

target_link_libraries(network-chat PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Widgets
)

install(TARGETS network-chat
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET network-chat
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
