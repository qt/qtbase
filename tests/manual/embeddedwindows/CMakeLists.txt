# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_manual_test(embeddedwindows
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
)

if(QT_FEATURE_xcb)
    target_link_libraries(embeddedwindows PRIVATE XCB::XCB)
endif()

if(APPLE)
    enable_language(OBJCXX)
    set_source_files_properties(main.cpp PROPERTIES LANGUAGE OBJCXX)
    set_property(TARGET embeddedwindows PROPERTY PROPERTY MACOSX_BUNDLE TRUE)
endif()
