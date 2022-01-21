cmake_minimum_required(VERSION 3.16)
project(androidnotifier LANGUAGES CXX)

if(NOT ANDROID)
    message(FATAL_ERROR "Example only works on Android")
endif()

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/corelib/platform/androidnotifier")

qt_add_executable(androidnotifier
    MANUAL_FINALIZATION
    main.cpp
    notificationclient.cpp
    notificationclient.h
)

target_link_libraries(androidnotifier PRIVATE
    Qt6::Widgets
)

set_property(TARGET androidnotifier APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
             ${CMAKE_CURRENT_SOURCE_DIR}/android)

qt_finalize_executable(androidnotifier)

set(qml_resource_files
    "images/happy.png"
    "images/sad.png"
)

qt_add_resources(androidnotifier "main"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)

install(TARGETS androidnotifier
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
