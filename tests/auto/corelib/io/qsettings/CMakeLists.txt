# Generated from qsettings.pro.

#####################################################################
## tst_qsettings Test:
#####################################################################

add_qt_test(tst_qsettings
    GUI
    SOURCES
        tst_qsettings.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    INCLUDE_DIRECTORIES
        ../../kernel/qmetatype
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Gui
)

# Resources:
add_qt_resource(tst_qsettings "qsettings"
    PREFIX
        "/"
    FILES
        "bom.ini"
        "resourcefile.ini"
        "resourcefile2.ini"
        "resourcefile3.ini"
        "resourcefile4.ini"
        "resourcefile5.ini"
        "resourcefile6.plist"
        "withcomments.ini"
)


## Scopes:
#####################################################################

extend_target(tst_qsettings CONDITION MSVC
    PUBLIC_LIBRARIES
        advapi32
)

extend_target(tst_qsettings CONDITION APPLE
    PUBLIC_LIBRARIES
        ${FWCoreFoundation}
)
