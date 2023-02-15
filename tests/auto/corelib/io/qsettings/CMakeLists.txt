# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsettings Test:
#####################################################################

# Test:
set(qsettings_resource_files
    "bom.ini"
    "resourcefile.ini"
    "resourcefile2.ini"
    "resourcefile3.ini"
    "resourcefile6.plist"
    "withcomments.ini"
    "float.ini"
    "qt5settings.ini"
    "utf8settings.ini"
)

qt_internal_add_test(tst_qsettings
    SOURCES
        tst_qsettings.cpp
    INCLUDE_DIRECTORIES
        ../../kernel/qmetatype
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
    TESTDATA ${qsettings_resource_files}
    BUILTIN_TESTDATA
)


## Scopes:
#####################################################################

qt_internal_extend_target(tst_qsettings CONDITION MSVC
    LIBRARIES
        advapi32
)

qt_internal_extend_target(tst_qsettings CONDITION APPLE
    LIBRARIES
        ${FWCoreFoundation}
)
