win32|wince:HEADERS += arch/qatomic_msvc.h

HEADERS += \
    arch/qatomic_bootstrap.h \
    arch/qatomic_cxx11.h

atomic64-libatomic: LIBS += -latomic
