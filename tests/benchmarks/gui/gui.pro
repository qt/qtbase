TEMPLATE = subdirs
SUBDIRS = \
        animation \
        image \
        kernel \
        math3d \
        painting \
        text

TRUSTED_BENCHMARKS += \
    painting/qtracebench

include(../trusted-benchmarks.pri)
