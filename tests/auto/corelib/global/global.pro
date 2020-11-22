TEMPLATE=subdirs
SUBDIRS=\
    qcompare \
    qflags \
    q_func_info \
    qgetputenv \
    qglobal \
    qnumeric \
    qfloat16 \
    qkeycombination \
    qrandomgenerator \
    qlogging \
    qtendian \
    qglobalstatic \
    qhooks \
    qoperatingsystemversion

win32: SUBDIRS += \
    qwinregistry
