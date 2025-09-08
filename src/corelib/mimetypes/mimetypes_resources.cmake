# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# List of files that need to be packaged as resources.
# This file exists solely because of unit tests that need access to this
# information as well. This was previously handled by referencing a qrc
# file with the same information

set(corelib_mimetypes_resource_file
    "${CMAKE_CURRENT_LIST_DIR}/3rdparty/tika-mimetypes.xml"
)

function(corelib_add_mimetypes_resources target)
    set(source_file "${corelib_mimetypes_resource_file}")
    set_source_files_properties("${source_file}"
        PROPERTIES QT_RESOURCE_ALIAS "tika-mimetypes.xml"
    )
    qt_internal_add_resource(${target} "mimetypes"
        PREFIX
            "/qt-project.org/qmime/tika/packages"
        FILES
            "${source_file}"
    )
endfunction()
