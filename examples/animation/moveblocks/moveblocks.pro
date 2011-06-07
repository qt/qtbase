SOURCES = main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/animation/moveblocks
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS moveblocks.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/animation/moveblocks
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000E3F7
    CONFIG += qt_example
}
maemo5: CONFIG += qt_example
