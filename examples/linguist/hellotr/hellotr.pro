#! [0]
SOURCES      = main.cpp
#! [0] #! [1]
TRANSLATIONS = hellotr_la.ts
#! [1]

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/linguist/hellotr
sources.files = $$SOURCES *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/linguist/hellotr
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets
