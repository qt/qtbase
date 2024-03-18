# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(car_SRCS)
qt_add_dbus_adaptor(car_SRCS
    ../common/car.xml
    qobject.h
    "" # empty parent_class value on purpose to not pass -l flag
    car_adaptor
)

qt_add_executable(car
    car.cpp car.h
    main.cpp
    ${car_SRCS}
)

set_target_properties(car PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(car PRIVATE
    Qt6::Core
    Qt6::DBus
    Qt6::Gui
    Qt6::Widgets
)

install(TARGETS car
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET car
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
