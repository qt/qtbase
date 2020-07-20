TEMPLATE=subdirs
SUBDIRS=\
    qflags \
    q_func_info \
    qgetputenv \
    qglobal \
    qnumeric \
    qfloat16 \
    qrandomgenerator \
    qlogging \
    qtendian \
    qglobalstatic \
    qhooks

win32: SUBDIRS += \
    qwinregistry
