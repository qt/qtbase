TEMPLATE = app
TARGET = qfileopeneventexternal
QT += core gui
SOURCES += qfileopeneventexternal.cpp
symbian: {
    RSS_RULES += "embeddability=KAppEmbeddable;"
    RSS_RULES.datatype_list += "priority = EDataTypePriorityHigh; type = \"application/x-tst_qfileopenevent\";"
    LIBS += -lapparc \
        -leikcore -lefsrv -lcone
}
