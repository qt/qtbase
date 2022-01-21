cmake_minimum_required(VERSION 3.16)
project(styles LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/widgets/styles")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(styles
    main.cpp
    norwegianwoodstyle.cpp norwegianwoodstyle.h
    widgetgallery.cpp widgetgallery.h
)

set_target_properties(styles PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(styles PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

# Resources:
set(styles_resource_files
    "images/woodbackground.png"
    "images/woodbutton.png"
)

qt_add_resources(styles "styles"
    PREFIX
        "/"
    FILES
        ${styles_resource_files}
)

install(TARGETS styles
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
