TEMPLATE = app
HEADERS   = lightmaps.h \
            mapzoom.h \
            slippymap.h
SOURCES   = lightmaps.cpp \
            main.cpp \
            mapzoom.cpp \
            slippymap.cpp
QT += network widgets

target.path = $$[QT_INSTALL_EXAMPLES]/embedded/lightmaps
INSTALLS += target
