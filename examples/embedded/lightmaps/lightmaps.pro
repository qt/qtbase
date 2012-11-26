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
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/embedded/lightmaps
INSTALLS += target sources
