option(host_build)

CONFIG += force_bootstrap
SOURCES += \
    template_app_manifest.cpp \
    template_background_js.cpp \
    template_debug.cpp \
    template_fullscreen.cpp \
    template_windowed.cpp \
    script_loader.cpp \
    main.cpp \

load(qt_tool)
CONFIG += c++11