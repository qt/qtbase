
QT       += core gui widgets

TARGET = applicationicon
TEMPLATE = app

SOURCES += main.cpp

OTHER_FILES += applicationicon.svg \
               applicationicon.png \
               applicationicon.desktop

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/applicationicon
INSTALLS += target
