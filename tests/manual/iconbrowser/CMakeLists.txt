# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

find_package(Qt6 REQUIRED COMPONENTS Gui Widgets)

qt_internal_add_manual_test(iconbrowser
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)

if (TARGET Qt::Quick)
find_package(Qt6 COMPONENTS QuickWidgets REQUIRED)

qt_add_qml_module(iconbrowser
    URI main
    VERSION 1.0
    QML_FILES "Main.qml"
    NO_RESOURCE_TARGET_PATH
)
target_link_libraries(iconbrowser PRIVATE
    Qt6::QuickWidgets
)
endif()

if (ANDROID)
    set(font_filename "MaterialIcons-Regular.ttf")
    if (QT_ALLOW_DOWNLOAD)
        include(FetchContent)

        FetchContent_Declare(
            MaterialIcons
            URL
                "https://github.com/google/material-design-icons/raw/master/font/${font_filename}"
            DOWNLOAD_DIR        ${CMAKE_CURRENT_BINARY_DIR}
            DOWNLOAD_NAME       "${font_filename}"
            DOWNLOAD_NO_EXTRACT TRUE
        )

        FetchContent_MakeAvailable(MaterialIcons)
    endif()

    if (EXISTS "${CMAKE_CURRENT_BINARY_DIR}/${font_filename}")
        set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/${font_filename}"
                                    PROPERTIES QT_RESOURCE_ALIAS ${font_filename})
        target_compile_definitions(iconbrowser PRIVATE "ICONBROWSER_RESOURCE")
        qt_add_resources(iconbrowser "icons"
            PREFIX
                "/qt-project.org/icons"
            FILES
                "${CMAKE_CURRENT_BINARY_DIR}/${font_filename}"
        )
    else()
        message(WARNING "Font file ${font_filename} not found and not downloaded!\n"
                        "Make sure the font file ${font_filename} is available in ${CMAKE_CURRENT_BINARY_DIR}.\n"
                        "Consider configuring with -DQT_ALLOW_DOWNLOAD=ON to download the font automatically.")
    endif()
endif()

