# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#! [qt_wrap_ui]
set(SOURCES mainwindow.cpp main.cpp)
qt_wrap_ui(SOURCES mainwindow.ui)
qt_add_executable(myapp ${SOURCES})
#! [qt_wrap_ui]

#! [qt_add_ui_1]
qt_add_executable(myapp mainwindow.cpp main.cpp)
qt_add_ui(myapp SOURCES mainwindow.ui)
#! [qt_add_ui_1]

#! [qt_add_ui_2]
qt_add_executable(myapp mainwindow.cpp main.cpp)
qt_add_ui(myapp INCLUDE_PREFIX "src/files" SOURCES mainwindow.ui)
#! [qt_add_ui_2]

#! [qt_add_ui_3]
qt_add_executable(myapp widget1.cpp widget2.cpp main.cpp)
qt_add_ui(myapp INCLUDE_PREFIX "src/files" SOURCES widget1.ui widget2.ui)
#! [qt_add_ui_3]

#! [qt_add_ui_4]
qt_add_executable(myapp widget1.cpp widget2.cpp main.cpp)
qt_add_ui(myapp INCLUDE_PREFIX "src/files"
                 SOURCES "my_ui_files_1/widget1.ui" "my_ui_files_2/widget2.ui")
#! [qt_add_ui_4]

#! [qt_add_ui_5]
qt_add_executable(myapp widget1.cpp widget2.cpp main.cpp)
qt_add_ui(myapp INCLUDE_PREFIX "src/files_1" SOURCES "my_ui_files/widget1.ui")
qt_add_ui(myapp INCLUDE_PREFIX "src/files_2" SOURCES "my_ui_files/widget2.ui")
#! [qt_add_ui_5]
