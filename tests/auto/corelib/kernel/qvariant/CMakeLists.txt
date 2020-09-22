# Generated from qvariant.pro.

#####################################################################
## tst_qvariant Test:
#####################################################################

qt_internal_add_test(tst_qvariant
    SOURCES
        tst_qvariant.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    INCLUDE_DIRECTORIES
        ../../../other/qvariant_common
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
)

# Resources:
set(qvariant_resource_files
    "stream/qt4.9/"
    "stream/qt5.0/"
)

qt_internal_add_resource(tst_qvariant "qvariant"
    PREFIX
        "/"
    FILES
        ${qvariant_resource_files}
)


## Scopes:
#####################################################################

qt_internal_extend_target(tst_qvariant CONDITION MSVC
    COMPILE_OPTIONS
        /bigobj
)

qt_internal_extend_target(tst_qvariant CONDITION NOT QT_FEATURE_doubleconversion AND NOT QT_FEATURE_system_doubleconversion
    DEFINES
        QT_NO_DOUBLECONVERSION
)
