
ARCH_SUBDIR=x86
contains(QMAKE_TARGET.arch, x86_64): {
    ARCH_SUBDIR=amd64
} else {
    !contains(QMAKE_TARGET.arch, x86): message("ERROR: Could not detect architecture from QMAKE_TARGET.arch")
}

MIDL_GENERATED = $$PWD/generated/$${ARCH_SUBDIR}

INCLUDEPATH += $$MIDL_GENERATED

SOURCES +=  $${MIDL_GENERATED}/ia2_api_all_i.c

HEADERS +=  $${MIDL_GENERATED}/ia2_api_all.h

OTHER_FILES = \
    $$PWD/idl/ia2_api_all.idl

LIBS += -lrpcrt4

TR_EXCLUDE += $$PWD/*
