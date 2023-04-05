# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_add_executable(echopluginwindow
    echointerface.h
    echowindow.cpp echowindow.h
    main.cpp
)

set_target_properties(echopluginwindow PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(echopluginwindow PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

if(QT6_IS_SHARED_LIBS_BUILD)
    # Build the shared plugin too when building this example target.
    add_dependencies(echopluginwindow echoplugin)
else()
    # Link the echoplugin if Qt is built statically.
    target_link_libraries(echopluginwindow PRIVATE
        echoplugin
    )
endif()

install(TARGETS echopluginwindow
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
