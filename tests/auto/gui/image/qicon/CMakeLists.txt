# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qicon Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    icons/*)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    second_icons/*)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    fallback_icons/*)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    *.png)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    *.svg)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    *.svgz)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qicon
    SOURCES
        tst_qicon.cpp
    LIBRARIES
        Qt::Gui
    TESTDATA ${test_data}
)

# Resources:
set(tst_qicon_resource_files
    "./fallback_icons/red.png"
    "./icons/testtheme/16x16/actions/appointment-new.png"
    "./icons/testtheme/22x22/actions/appointment-new.png"
    "./icons/testtheme/index.theme"
    "./icons/testtheme/scalable/actions/svg-only.svg"
    "./icons/themeparent/16x16/actions/address-book-new.png"
    "./icons/themeparent/16x16/actions/appointment-new.png"
    "./icons/themeparent/22x22/actions/address-book-new.png"
    "./icons/themeparent/22x22/actions/appointment-new.png"
    "./icons/themeparent/32x32/actions/address-book-new.png"
    "./icons/themeparent/32x32/actions/appointment-new.png"
    "./icons/themeparent/icon-theme.cache"
    "./icons/themeparent/index.theme"
    "./icons/themeparent/scalable/actions/address-book-new.svg"
    "./icons/themeparent/scalable/actions/appointment-new.svg"
    "./icons/fallbacktheme/index.theme"
    "./icons/fallbacktheme/16x16/edit-cut.png"
    "./icons/hicolor/index.theme"
    "./icons/hicolor/16x16/hicolor-icon.png"
    "./second_icons/testtheme/32x32/actions/appointment-new.png"
    "./styles/commonstyle/images/standardbutton-open-128.png"
    "./styles/commonstyle/images/standardbutton-open-16.png"
    "./styles/commonstyle/images/standardbutton-open-32.png"
    "./styles/commonstyle/images/standardbutton-open-64.png"
    "./styles/commonstyle/images/standardbutton-save-128.png"
    "./styles/commonstyle/images/standardbutton-save-16.png"
    "./styles/commonstyle/images/standardbutton-save-32.png"
    "./styles/commonstyle/images/standardbutton-save-64.png"
    "image.png"
    "rect.png"
    "tst_qicon.cpp"
)

qt_internal_add_resource(tst_qicon "tst_qicon"
    PREFIX
        "/"
    FILES
        ${tst_qicon_resource_files}
)
set(qmake_immediate_resource_files
    "tst_qicon.cpp"
)

qt_internal_add_resource(tst_qicon "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)


## Scopes:
#####################################################################

qt_internal_extend_target(tst_qicon CONDITION TARGET Qt::Widgets
    LIBRARIES
        Qt::Widgets
)
