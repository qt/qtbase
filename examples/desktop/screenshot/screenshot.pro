HEADERS             = screenshot.h
SOURCES             = main.cpp \
                      screenshot.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/desktop/screenshot
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS screenshot.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/desktop/screenshot
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
