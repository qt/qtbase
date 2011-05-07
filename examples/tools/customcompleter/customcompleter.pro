HEADERS   = mainwindow.h \
            textedit.h
SOURCES   = main.cpp \
            mainwindow.cpp \
            textedit.cpp
RESOURCES = customcompleter.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/customcompleter
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS customcompleter.pro resources
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/customcompleter
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets
