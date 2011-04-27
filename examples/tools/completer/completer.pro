HEADERS   = fsmodel.h \
            mainwindow.h
SOURCES   = fsmodel.cpp \
            main.cpp \
            mainwindow.cpp
RESOURCES = completer.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/completer
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS completer.pro resources
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/completer
INSTALLS += target sources

symbian: CONFIG += qt_example
