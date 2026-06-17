SOURCES += main.cpp pathdeform.cpp arthurwidgets.cpp
HEADERS += pathdeform.h arthurwidgets.h

QT += widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/painting/deform
INSTALLS += target
