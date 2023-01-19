# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#! [0]
qt_add_plugin(simplestyleplugin
    CLASS_NAME SimpleStylePlugin
    simplestyle.cpp simplestyle.h
    simplestyleplugin.cpp simplestyleplugin.h
)
#! [0]

if(QT_FEATURE_debug AND APPLE)
    set_property(TARGET simplestyleplugin
       APPEND_STRING PROPERTY OUTPUT_NAME "_debug")
endif()

get_target_property(is_bundle styleplugin MACOSX_BUNDLE)
if(APPLE AND is_bundle)
#! [1]
    set_target_properties(simplestyleplugin PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "$<TARGET_BUNDLE_CONTENT_DIR:styleplugin>/PlugIns/styles"
    )
#! [1]
else()
#! [2]
    set_target_properties(simplestyleplugin PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "$<TARGET_FILE_DIR:styleplugin>/styles"
    )
#! [2]
endif()

target_link_libraries(simplestyleplugin PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

install(TARGETS simplestyleplugin
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
