option(host_build)
TEMPLATE = aux

# qmake documentation
QMAKE_DOCS = $$PWD/doc/qmake.qdocconf

# qmake binary
win32: EXTENSION = .exe

!build_pass {
    qmake_exe.target = $$OUT_PWD/qmake$$EXTENSION
    qmake_exe.commands = $(MAKE) binary
    qmake_exe.CONFIG = phony
    QMAKE_EXTRA_TARGETS += qmake_exe

    QMAKE_DISTCLEAN += qmake$$EXTENSION

    first.depends += qmake_exe
    QMAKE_EXTRA_TARGETS += first
}

qmake.path = $$[QT_HOST_BINS]
qmake.files = $$OUT_PWD/qmake$$EXTENSION
qmake.CONFIG = no_check_exist executable
INSTALLS += qmake
