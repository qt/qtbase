HEADERS   = window.h
SOURCES   = main.cpp \
            window.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/itemviews/simplewidgetmapper
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/itemviews/simplewidgetmapper
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
