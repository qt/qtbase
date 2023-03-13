# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/dbus/remotecontrolledcar/controller")

set(controller_SRCS)
qt_add_dbus_interface(controller_SRCS
    ../common/car.xml
    car_interface
)

qt_add_executable(controller
    controller.cpp controller.h controller.ui
    main.cpp
    ${controller_SRCS}
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
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
