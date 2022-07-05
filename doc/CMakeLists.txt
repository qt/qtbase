# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# special case skip regeneration
#
# Copy/Install doc configuration files to the build/install directory
#
qt_path_join(doc_install_dir ${QT_INSTALL_DIR} ${INSTALL_DOCDIR})
foreach(dir global config)
    qt_copy_or_install(DIRECTORY ${dir} DESTINATION ${doc_install_dir})
    if(QT_SUPERBUILD)
        qt_path_join(destination ${QtBase_BINARY_DIR} ${INSTALL_DOCDIR})
        file(COPY ${dir} DESTINATION ${destination})
    endif()
endforeach()
