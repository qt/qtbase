QT += concurrent widgets

SOURCES += main.cpp primecounter.cpp
HEADERS += primecounter.h

target.path = $$[QT_INSTALL_EXAMPLES]/qtconcurrent/primecounter
INSTALLS += target

FORMS += primecounter.ui
