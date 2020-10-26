# Generated from app.pro.

cmake_minimum_required(VERSION 3.14)
project(plugandpaint LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/tools/plugandpaint")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(plugandpaint
    interfaces.h
    main.cpp
    mainwindow.cpp mainwindow.h
    paintarea.cpp paintarea.h
    plugindialog.cpp plugindialog.h
)
set_target_properties(plugandpaint PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(plugandpaint PUBLIC
    # Remove: L../plugins
    Qt::Core
    Qt::Gui
    Qt::Widgets
    pnp_basictools # special case
)
target_link_libraries(plugandpaint PUBLIC pnp_basictools) # special case

if(macx-xcode)
    target_link_libraries(plugandpaint PUBLIC
        (
        )
        pnp_basictools$
    )
endif()

if(NOT macx-xcode)
    target_link_libraries(plugandpaint PUBLIC
        pnp_basictools
    )
endif()

if(((NOT (macx-xcode)) AND (( NOT debug_and_release OR build_pass ) AND CONFIG(debug,debug OR release))) AND (APPLE))
    target_link_libraries(plugandpaint PUBLIC
        (LIBS, 0)
        (LIBS, 1)
        _debug
    )
endif()

if(((NOT (macx-xcode)) AND (( NOT debug_and_release OR build_pass ) AND CONFIG(debug,debug OR release))) AND (WIN32))
    target_link_libraries(plugandpaint PUBLIC
        (LIBS, 0)
        (LIBS, 1)
        d
    )
endif()

install(TARGETS plugandpaint
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
