SOURCES += main.cpp composition.cpp arthurwidgets.cpp
HEADERS += composition.h arthurwidgets.h

RESOURCES += composition.qrc
QT += widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/painting/composition
INSTALLS += target
