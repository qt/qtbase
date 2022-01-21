cmake_minimum_required(VERSION 3.16)
project(subscription LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/corelib/bindableproperties/subscription")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(subscription
    ../shared/subscriptionwindow.cpp ../shared/subscriptionwindow.h ../shared/subscriptionwindow.ui
    main.cpp
    subscription.cpp subscription.h
    user.cpp user.h
)

target_link_libraries(subscription PUBLIC
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

qt_add_resources(subscription "countries"
    PREFIX
        "/"
    BASE
        "../shared"
    FILES
        ${countries_resource_files}
)

install(TARGETS subscription
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
