# Generated from oci.pro.

#####################################################################
## qsqloci Plugin:
#####################################################################

qt_add_plugin(qsqloci
    TYPE sqldrivers
    CLASS_NAME QOCIDriverPlugin
    SOURCES
        main.cpp
        qsql_oci.cpp qsql_oci_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::SqlPrivate
    PUBLIC_LIBRARIES
        Oracle::OCI
        Qt::Core
        Qt::Sql
)

#### Keys ignored in scope 1:.:.:oci.pro:<TRUE>:
# OTHER_FILES = "oci.json"

## Scopes:
#####################################################################

qt_extend_target(qsqloci CONDITION APPLE
    LINK_OPTIONS
        "-Wl,-flat_namespace,-U,_environ"
)
