# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(securesocketclient LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/network/securesocketclient")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Widgets)

qt_standard_project_setup()

qt_add_executable(securesocketclient
    certificateinfo.cpp certificateinfo.h certificateinfo.ui
    main.cpp
    sslclient.cpp sslclient.h sslclient.ui
    sslerrors.ui
)

set_target_properties(securesocketclient PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(securesocketclient PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Widgets
)

# Resources:
set(securesocketclient_resource_files
    "encrypted.png"
)

qt_add_resources(securesocketclient "securesocketclient"
    PREFIX
        "/"
    FILES
        ${securesocketclient_resource_files}
)

install(TARGETS securesocketclient
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
