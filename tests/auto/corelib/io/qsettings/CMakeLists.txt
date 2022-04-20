# Generated from qsettings.pro.

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
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
    TESTDATA ${qsettings_resource_files}
    BUILTIN_TESTDATA
)


## Scopes:
#####################################################################

qt_internal_extend_target(tst_qsettings CONDITION MSVC
    PUBLIC_LIBRARIES
        advapi32
)

qt_internal_extend_target(tst_qsettings CONDITION APPLE
    PUBLIC_LIBRARIES
        ${FWCoreFoundation}
)
