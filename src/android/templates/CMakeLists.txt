# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Android template files

# special case skip regeneration
# special case begin
set(template_files
    AndroidManifest.xml
    build.gradle)

set(template_directories
    res)

qt_path_join(destination ${QT_INSTALL_DIR} "src/android/templates")
qt_copy_or_install(FILES ${template_files}
                   DESTINATION "${destination}")

qt_copy_or_install(DIRECTORY ${template_directories}
                   DESTINATION "${destination}")
# special case end
