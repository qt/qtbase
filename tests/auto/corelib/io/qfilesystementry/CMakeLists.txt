add_qt_test(tst_qfilesystementry SOURCES
        tst_qfilesystementry.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/io/qfilesystementry.cpp
        ${PROJECT_SOURCE_DIR}/src/corelib/io/qfilesystementry_p.h
    LIBRARIES
        Qt::CorePrivate
)
