HEADERS     = dialog.h
SOURCES     = dialog.cpp \
              main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/layouts/dynamiclayouts
sources.files = $$SOURCES $$HEADERS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/layouts/dynamiclayouts
INSTALLS += target sources

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
