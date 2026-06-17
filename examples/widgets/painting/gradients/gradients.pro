SOURCES += main.cpp gradients.cpp arthurwidgets.cpp hoverpoints.cpp
HEADERS += gradients.h arthurwidgets.h hoverpoints.h

QT += widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/painting/gradients
INSTALLS += target

