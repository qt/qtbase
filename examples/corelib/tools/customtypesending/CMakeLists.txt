cmake_minimum_required(VERSION 3.16)
project(customtypesending LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/corelib/tools/customtypesending")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(customtypesending
    main.cpp
    message.cpp message.h
    window.cpp window.h
)

set_target_properties(customtypesending PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(customtypesending PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

install(TARGETS customtypesending
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
