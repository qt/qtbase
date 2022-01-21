cmake_minimum_required(VERSION 3.16)
project(sliders LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/widgets/sliders")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(sliders
    main.cpp
    slidersgroup.cpp slidersgroup.h
    window.cpp window.h
)

set_target_properties(sliders PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(sliders PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

install(TARGETS sliders
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
