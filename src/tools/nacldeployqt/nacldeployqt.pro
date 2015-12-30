option(host_build)

CONFIG += force_bootstrap
SOURCES += \
    template_app_manifest.cpp \
    template_background_js.cpp \
    template_debug.cpp \
    template_fullscreen.cpp \
    template_windowed.cpp \
    template_qtloader.cpp \
    main.cpp \

load(qt_tool)
CONFIG += c++11