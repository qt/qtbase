# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(validators LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/widgets/validators")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup()

qt_add_executable(validators
    ledwidget.cpp ledwidget.h
    localeselector.cpp localeselector.h
    main.cpp
    validators.ui
    validatorwidget.cpp validatorwidget.h
)

set_target_properties(validators PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(validators PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Resources:
set(validators_resource_files
    "ledoff.png"
    "ledon.png"
)

qt_add_resources(validators "validators"
    PREFIX
        "/"
    FILES
        ${validators_resource_files}
)

install(TARGETS validators
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
