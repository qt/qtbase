# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(controller_SRCS)
qt_add_dbus_interface(controller_SRCS
    ../common/car.xml
    car_interface
)

qt_add_executable(controller
    controller.cpp controller.h
    main.cpp
    ${controller_SRCS}
)

qt_add_resources(controller
    PREFIX
        "/"
    FILES
    down.svg
    left.svg
    right.svg
    up.svg
    connected.svg
    connecting.svg
)

set_target_properties(controller PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(controller PRIVATE
    Qt6::Core
    Qt6::DBus
    Qt6::Gui
    Qt6::Widgets
)

install(TARGETS controller
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET controller
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
