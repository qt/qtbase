# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(convert LANGUAGES CXX)

if (ANDROID)
    message(FATAL_ERROR "This project cannot be built on Android.")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core)

qt_standard_project_setup()

qt_add_executable(convert
    cborconverter.cpp cborconverter.h
    converter.cpp converter.h
    datastreamconverter.cpp datastreamconverter.h
    debugtextdumper.cpp debugtextdumper.h
    jsonconverter.cpp jsonconverter.h
    main.cpp
    nullconverter.cpp nullconverter.h
    textconverter.cpp textconverter.h
    variantorderedmap.h
    xmlconverter.cpp xmlconverter.h
)

target_link_libraries(convert PRIVATE
    Qt6::Core
)

install(TARGETS convert
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET convert
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
