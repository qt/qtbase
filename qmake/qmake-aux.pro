option(host_build)
TEMPLATE = aux

# qmake documentation
QMAKE_DOCS = $$PWD/doc/qmake.qdocconf

# qmake binary
win32: EXTENSION = .exe

!build_pass {
    qmake_exe.target = $$OUT_PWD/qmake$$EXTENSION
    qmake_exe.depends = ../bin/qmake$$EXTENSION builtin-qt.conf
    equals(QMAKE_DIR_SEP, /): \
        qmake_exe.commands = cat ../bin/qmake$$EXTENSION builtin-qt.conf > qmake$$EXTENSION && chmod +x qmake$$EXTENSION
    else: \
        qmake_exe.commands = copy /B ..\bin\qmake$$EXTENSION + builtin-qt.conf qmake$$EXTENSION
    QMAKE_EXTRA_TARGETS += qmake_exe

    QMAKE_CLEAN += builtin-qt.conf
    QMAKE_DISTCLEAN += qmake$$EXTENSION

    first.depends += qmake_exe
    QMAKE_EXTRA_TARGETS += first
}

qmake.path = $$[QT_HOST_BINS]
qmake.files = $$OUT_PWD/qmake$$EXTENSION
qmake.CONFIG = no_check_exist executable
INSTALLS += qmake
