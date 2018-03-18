HEADERS += \
    arch/qatomic_bootstrap.h \
    arch/qatomic_cxx11.h

qtConfig(std-atomic64): QMAKE_USE += libatomic
