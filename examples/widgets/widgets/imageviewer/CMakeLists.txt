cmake_minimum_required(VERSION 3.16)
project(imageviewer LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/widgets/imageviewer")

find_package(Qt6
    REQUIRED COMPONENTS Core Gui Widgets
    OPTIONAL_COMPONENTS PrintSupport
)

qt_add_executable(imageviewer
    imageviewer.cpp imageviewer.h
    main.cpp
)

set_target_properties(imageviewer PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(imageviewer PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

if (TARGET Qt::PrintSupport)
    target_link_libraries(imageviewer PUBLIC Qt::PrintSupport)
endif()

if(TARGET Qt::PrintSupport)
    target_link_libraries(imageviewer PUBLIC
        Qt::PrintSupport
    )
endif()

install(TARGETS imageviewer
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
