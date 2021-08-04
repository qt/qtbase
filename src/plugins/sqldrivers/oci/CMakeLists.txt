# Generated from oci.pro.

#####################################################################
## QOCIDriverPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QOCIDriverPlugin
    OUTPUT_NAME qsqloci
    PLUGIN_TYPE sqldrivers
    SOURCES
        main.cpp
        qsql_oci.cpp qsql_oci_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Oracle::OCI
        Qt::Core
        Qt::CorePrivate
        Qt::SqlPrivate
)

#### Keys ignored in scope 1:.:.:oci.pro:<TRUE>:
# OTHER_FILES = "oci.json"

## Scopes:
#####################################################################

qt_internal_extend_target(QOCIDriverPlugin CONDITION APPLE
    LINK_OPTIONS
        "-Wl,-flat_namespace,-U,_environ"
)

qt_internal_force_macos_intel_arch(QOCIDriverPlugin)
