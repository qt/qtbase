SOURCES = digiflip.cpp

symbian {
    TARGET.UID3 = 0xA000CF72
    CONFIG += qt_example
}

target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/embedded/digiflip
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/embedded/digiflip
INSTALLS += target sources
