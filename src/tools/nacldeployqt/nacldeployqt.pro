option(host_build)

CONFIG += force_bootstrap
SOURCES += \
    template_debug.cpp \
    template_fullscreen.cpp \
    main.cpp \

load(qt_tool)
CONFIG += c++11