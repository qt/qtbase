QT += widgets

HEADERS     = dialog.h
SOURCES     = dialog.cpp \
              main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/layouts/dynamiclayouts
INSTALLS += target
