# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Android Java Resource files

# special case skip regeneration
# special case begin
set(resource_directories
    res
    src)

qt_path_join(destination ${QT_INSTALL_DIR} "src/android/java")
qt_copy_or_install(DIRECTORY ${resource_directories}
                   DESTINATION "${destination}"
)
# special case end
