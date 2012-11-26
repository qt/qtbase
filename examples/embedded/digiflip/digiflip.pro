QT += widgets

SOURCES = digiflip.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/embedded/digiflip
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/embedded/digiflip
INSTALLS += target sources
