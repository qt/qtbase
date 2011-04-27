HEADERS   = treemodelcompleter.h \
            mainwindow.h
SOURCES   = treemodelcompleter.cpp \
            main.cpp \
            mainwindow.cpp
RESOURCES = treemodelcompleter.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/treemodelcompleter
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS treemodelcompleter.pro resources
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/treemodelcompleter
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
