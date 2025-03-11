# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#
# Copy/Install doc configuration files to the build/install directory
#
qt_path_join(doc_install_dir ${QT_INSTALL_DIR} ${INSTALL_DOCDIR})
foreach(dir global config)
    qt_copy_or_install(DIRECTORY ${dir} DESTINATION ${doc_install_dir})
    if(QT_SUPERBUILD OR "${PROJECT_NAME}" STREQUAL "QtBase")
        qt_path_join(destination ${QtBase_BINARY_DIR} ${INSTALL_DOCDIR})
        file(COPY ${dir} DESTINATION ${destination})

        if(NOT QT_NO_SET_QTBASE_DOC_FILE_DEPS_COPYING)
            # Make sure touched doc files cause a reconfigure, so they get re-copied.
            # TODO: Consider making this a build time file copy dependency.
            # It is more complicated, because all documentation generation custom targets would have
            # to depend on the file outputs of add_custom_command(copy), and it's not clear how the
            # dependencies should be set up in that case (coarsness, use install vs build dirs,
            # etc).
            file(GLOB_RECURSE files LIST_DIRECTORIES FALSE "${dir}/*")
            _qt_internal_append_cmake_configure_depends(${files})
        endif()
    endif()
endforeach()
