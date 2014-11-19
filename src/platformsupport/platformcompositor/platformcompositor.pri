contains(QT_CONFIG, opengl) {
    SOURCES += $$PWD/qopenglcompositor.cpp \
               $$PWD/qopenglcompositorbackingstore.cpp

    HEADERS += $$PWD/qopenglcompositor_p.h \
               $$PWD/qopenglcompositorbackingstore_p.h
}
