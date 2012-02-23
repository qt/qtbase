TARGET = qsqloci

SOURCES = main.cpp
OTHER_FILES += oci.json
include(../../../sql/drivers/oci/qsql_oci.pri)

include(../qsqldriverbase.pri)
