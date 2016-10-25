# Qt kernel library base module

HEADERS +=  \
	global/qglobal.h \
        global/qsystemdetection.h \
        global/qcompilerdetection.h \
        global/qprocessordetection.h \
	global/qnamespace.h \
        global/qendian.h \
        global/qnumeric_p.h \
        global/qnumeric.h \
        global/qglobalstatic.h \
        global/qlibraryinfo.h \
        global/qlogging.h \
        global/qtypeinfo.h \
        global/qsysinfo.h \
        global/qisenum.h \
        global/qtypetraits.h \
        global/qflags.h \
        global/qhooks_p.h \
        global/qversiontagging.h

SOURCES += \
        global/archdetect.cpp \
	global/qglobal.cpp \
        global/qglobalstatic.cpp \
        global/qlibraryinfo.cpp \
	global/qmalloc.cpp \
        global/qnumeric.cpp \
        global/qlogging.cpp \
        global/qhooks.cpp

VERSIONTAGGING_SOURCES = global/qversiontagging.cpp

# qlibraryinfo.cpp includes qconfig.cpp
INCLUDEPATH += $$QT_BUILD_TREE/src/corelib/global

# Only used on platforms with CONFIG += precompile_header
PRECOMPILED_HEADER = global/qt_pch.h

# qlogging.cpp uses backtrace(3), which is in a separate library on the BSDs.
LIBS_PRIVATE += $$QMAKE_LIBS_EXECINFO

if(linux*|hurd*):!cross_compile:!static:!*-armcc* {
   QMAKE_LFLAGS += -Wl,-e,qt_core_boilerplate
   prog=$$quote(if (/program interpreter: (.*)]/) { print $1; })
   DEFINES += ELF_INTERPRETER=\\\"$$system(LC_ALL=C readelf -l /bin/ls | perl -n -e \'$$prog\')\\\"
}

slog2 {
    LIBS_PRIVATE += -lslog2
    DEFINES += QT_USE_SLOG2
}

journald {
    QMAKE_USE_PRIVATE += journald
    DEFINES += QT_USE_JOURNALD
}

syslog {
    DEFINES += QT_USE_SYSLOG
}

gcc:ltcg {
    versiontagging_compiler.commands = $$QMAKE_CXX -c $(CXXFLAGS) $(INCPATH)

    # Disable LTO, as the symbols disappear somehow under GCC
    versiontagging_compiler.commands += -fno-lto

    versiontagging_compiler.commands += -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
    versiontagging_compiler.dependency_type = TYPE_C
    versiontagging_compiler.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${first(QMAKE_EXT_OBJ)}
    versiontagging_compiler.input = VERSIONTAGGING_SOURCES
    versiontagging_compiler.variable_out = OBJECTS
    versiontagging_compiler.name = compiling[versiontagging] ${QMAKE_FILE_IN}
    silent: versiontagging_compiler.commands = @echo compiling[versiontagging] ${QMAKE_FILE_IN} && $$versiontagging_compiler.commands
    QMAKE_EXTRA_COMPILERS += versiontagging_compiler
} else {
    SOURCES += $$VERSIONTAGGING_SOURCES
}
