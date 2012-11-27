HEADERS     = dialog.h
SOURCES     = dialog.cpp \
              main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/layouts/dynamiclayouts
INSTALLS += target

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
