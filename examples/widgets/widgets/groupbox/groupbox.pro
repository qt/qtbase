QT += widgets

HEADERS       = window.h
SOURCES       = window.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/groupbox
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)
