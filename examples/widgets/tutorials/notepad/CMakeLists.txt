# Generated from notepad.pro.

cmake_minimum_required(VERSION 3.14)
project(notepad LANGUAGES CXX)

find_package(Qt5 COMPONENTS Widgets) # special case: add
find_package(Qt5 COMPONENTS PrintSupport) # special case: add

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

add_executable(notepad WIN32 MACOSX_BUNDLE
    main.cpp
    notepad.cpp notepad.h notepad.qrc notepad.ui
)
# special case begin
target_link_libraries(notepad PUBLIC
    Qt::Widgets
)
if (TARGET Qt::PrintSupport)
    target_link_libraries(notepad PUBLIC
        Qt::PrintSupport
    )
endif()
# special case end

install(TARGETS notepad
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
