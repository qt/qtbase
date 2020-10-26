# Generated from textedit.pro.

cmake_minimum_required(VERSION 3.14)
project(textedit LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/richtext/textedit")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS PrintSupport) # special case

qt_add_executable(textedit
    main.cpp
    textedit.cpp textedit.h
)
set_target_properties(textedit PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(textedit PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)
# special case begin
if (TARGET Qt::PrintSupport)
    target_link_libraries(textedit PUBLIC Qt::PrintSupport)
endif()
# special case end


# Resources:
set(textedit_resource_files
    "example.html"
    "images/logo32.png"
    "images/mac/checkbox-checked.png"
    "images/mac/checkbox.png"
    "images/mac/editcopy.png"
    "images/mac/editcut.png"
    "images/mac/editpaste.png"
    "images/mac/editredo.png"
    "images/mac/editundo.png"
    "images/mac/exportpdf.png"
    "images/mac/filenew.png"
    "images/mac/fileopen.png"
    "images/mac/fileprint.png"
    "images/mac/filesave.png"
    "images/mac/format-indent-less.png"
    "images/mac/format-indent-more.png"
    "images/mac/textbold.png"
    "images/mac/textcenter.png"
    "images/mac/textitalic.png"
    "images/mac/textjustify.png"
    "images/mac/textleft.png"
    "images/mac/textright.png"
    "images/mac/textunder.png"
    "images/mac/zoomin.png"
    "images/mac/zoomout.png"
    "images/win/checkbox-checked.png"
    "images/win/checkbox.png"
    "images/win/editcopy.png"
    "images/win/editcut.png"
    "images/win/editpaste.png"
    "images/win/editredo.png"
    "images/win/editundo.png"
    "images/win/exportpdf.png"
    "images/win/filenew.png"
    "images/win/fileopen.png"
    "images/win/fileprint.png"
    "images/win/filesave.png"
    "images/win/format-indent-less.png"
    "images/win/format-indent-more.png"
    "images/win/textbold.png"
    "images/win/textcenter.png"
    "images/win/textitalic.png"
    "images/win/textjustify.png"
    "images/win/textleft.png"
    "images/win/textright.png"
    "images/win/textunder.png"
    "images/win/zoomin.png"
    "images/win/zoomout.png"
)

qt6_add_resources(textedit "textedit"
    PREFIX
        "/"
    FILES
        ${textedit_resource_files}
)

if(TARGET Qt::PrintSupport)
    target_link_libraries(textedit PUBLIC
        Qt::PrintSupport
    )
endif()

install(TARGETS textedit
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
