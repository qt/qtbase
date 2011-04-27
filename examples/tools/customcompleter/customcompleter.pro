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
maemo5: CONFIG += qt_example

symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
simulator: warning(This example might not fully work on Simulator platform)
