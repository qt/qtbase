# Generated from qsettings.pro.

#####################################################################
## tst_qsettings Test:
#####################################################################

qt_internal_add_test(tst_qsettings
    SOURCES
        tst_qsettings.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    INCLUDE_DIRECTORIES
        ../../kernel/qmetatype
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
)

# Resources:
set(qsettings_resource_files
    "bom.ini"
    "resourcefile.ini"
    "resourcefile2.ini"
    "resourcefile3.ini"
    "resourcefile4.ini"
    "resourcefile5.ini"
    "resourcefile6.plist"
    "withcomments.ini"
    "float.ini"
)

qt_internal_add_resource(tst_qsettings "qsettings"
    PREFIX
        "/"
    FILES
        ${qsettings_resource_files}
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
