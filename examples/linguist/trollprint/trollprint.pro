HEADERS       = mainwindow.h \
                printpanel.h
SOURCES       = main.cpp \
                mainwindow.cpp \
                printpanel.cpp
TRANSLATIONS  = trollprint_pt.ts

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/linguist/trollprint
sources.files = $$SOURCES $$HEADERS $$TRANSLATIONS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/linguist/trollprint
INSTALLS += target sources

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example

symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
simulator: warning(This example might not fully work on Simulator platform)
