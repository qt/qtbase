QT += widgets

SOURCES += main.cpp lighting.cpp
HEADERS += lighting.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/effects/lighting
INSTALLS += target
