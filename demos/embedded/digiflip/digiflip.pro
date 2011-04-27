SOURCES = digiflip.cpp

symbian {
    TARGET.UID3 = 0xA000CF72
    include($$QT_SOURCE_TREE/demos/symbianpkgrules.pri)
}

target.path = $$[QT_INSTALL_DEMOS]/qtbase/embedded/digiflip
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro
sources.path = $$[QT_INSTALL_DEMOS]/qtbase/embedded/digiflip
INSTALLS += target sources
