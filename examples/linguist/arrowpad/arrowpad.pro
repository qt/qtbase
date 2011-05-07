#! [0]
HEADERS      = arrowpad.h \
               mainwindow.h
SOURCES      = arrowpad.cpp \
               main.cpp \
               mainwindow.cpp
#! [0] #! [1]
TRANSLATIONS = arrowpad_fr.ts \
               arrowpad_nl.ts
#! [1]

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/linguist/arrowpad
sources.files = $$SOURCES $$HEADERS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/linguist/arrowpad
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets
