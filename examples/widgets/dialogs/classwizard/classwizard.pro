QT += widgets

HEADERS       = classwizard.h
SOURCES       = classwizard.cpp \
                main.cpp
RESOURCES     = classwizard.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/dialogs/classwizard
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)
