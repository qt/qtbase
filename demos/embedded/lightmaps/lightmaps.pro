TEMPLATE = app
HEADERS   = lightmaps.h \
            mapzoom.h \
            slippymap.h
SOURCES   = lightmaps.cpp \
            main.cpp \
            mapzoom.cpp \
            slippymap.cpp
QT += network

symbian {
    TARGET.UID3 = 0xA000CF75
    include($$QT_SOURCE_TREE/demos/symbianpkgrules.pri)
    TARGET.CAPABILITY = NetworkServices
    TARGET.EPOCHEAPSIZE = 0x20000 0x2000000
}

target.path = $$[QT_INSTALL_DEMOS]/qtbase/embedded/lightmaps
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_DEMOS]/qtbase/embedded/lightmaps
INSTALLS += target sources
