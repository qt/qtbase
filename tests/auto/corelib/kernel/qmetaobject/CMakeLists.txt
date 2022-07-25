set(tst_qmetaobject_SOURCES
    tst_qmetaobject.cpp
    forwarddeclared.h
    forwarddeclared.cpp
)

qt_internal_add_test(tst_qmetaobject
    SOURCES
        ${tst_qmetaobject_SOURCES}
    LIBRARIES
        Qt::CorePrivate
)

