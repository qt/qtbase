cmake_minimum_required(VERSION 3.16)
project(bindablesubscription LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/corelib/bindableproperties/bindablesubscription")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(bindablesubscription
    ../shared/subscriptionwindow.cpp ../shared/subscriptionwindow.h ../shared/subscriptionwindow.ui
    main.cpp
    bindablesubscription.cpp bindablesubscription.h
    bindableuser.cpp bindableuser.h
)

target_link_libraries(bindablesubscription PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

# Resources:
set(countries_resource_files
    "../shared/finland.png"
    "../shared/germany.png"
    "../shared/norway.png"
)

qt_add_resources(bindablesubscription "countries"
    PREFIX
        "/"
    BASE
        "../shared"
    FILES
        ${countries_resource_files}
)

install(TARGETS bindablesubscription
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
