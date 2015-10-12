option(host_build)
TEMPLATE = aux

# qmake documentation
QMAKE_DOCS = $$PWD/doc/qmake.qdocconf

# qmake binary
win32: EXTENSION = .exe
qmake.path = $$[QT_HOST_BINS]
qmake.files = $$OUT_PWD/../bin/qmake$$EXTENSION
INSTALLS += qmake
