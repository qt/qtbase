TEMPLATE=subdirs
SUBDIRS=\
    qflags \
    q_func_info \
    qgetputenv \
    qglobal \
    qnumeric \
    qfloat16 \
    qrand \
    qrandomgenerator \
    qlogging \
    qtendian \
    qglobalstatic \
    qhooks

win32:!winrt: SUBDIRS += \
    qwinregistry
