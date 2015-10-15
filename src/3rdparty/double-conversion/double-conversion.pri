INCLUDEPATH += $$PWD/include $$PWD/include/double-conversion
SOURCES += \
    $$PWD/bignum.cc \
    $$PWD/bignum-dtoa.cc \
    $$PWD/cached-powers.cc \
    $$PWD/diy-fp.cc \
    $$PWD/double-conversion.cc \
    $$PWD/fast-dtoa.cc \
    $$PWD/fixed-dtoa.cc \
    $$PWD/strtod.cc

HEADERS += \
    $$PWD/bignum-dtoa.h \
    $$PWD/bignum.h \
    $$PWD/cached-powers.h \
    $$PWD/diy-fp.h \
    $$PWD/include/double-conversion/double-conversion.h \
    $$PWD/fast-dtoa.h \
    $$PWD/fixed-dtoa.h \
    $$PWD/ieee.h \
    $$PWD/strtod.h \
    $$PWD/include/double-conversion/utils.h

OTHER_FILES += README
