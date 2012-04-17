win32|wince:HEADERS += arch/qatomic_msvc.h
vxworks:HEADERS += arch/qatomic_vxworks.h
integrity:HEADERS += arch/qatomic_integrity.h

HEADERS += \
    arch/qatomic_alpha.h \
    arch/qatomic_armv5.h \
    arch/qatomic_armv6.h \
    arch/qatomic_armv7.h \
    arch/qatomic_bfin.h \
    arch/qatomic_bootstrap.h \
    arch/qatomic_ia64.h \
    arch/qatomic_mips.h \
    arch/qatomic_power.h \
    arch/qatomic_s390.h \
    arch/qatomic_sh4a.h \
    arch/qatomic_sparc.h \
    arch/qatomic_x86.h \
    arch/qatomic_gcc.h \
    arch/qatomic_cxx11.h

unix {
    # fallback implementation when no other appropriate qatomic_*.h exists
    HEADERS += arch/qatomic_unix.h
    SOURCES += arch/qatomic_unix.cpp
}
