HEADERS             = screenshot.h
SOURCES             = main.cpp \
                      screenshot.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/desktop/screenshot
INSTALLS += target

QT += widgets
simulator: warning(This example might not fully work on Simulator platform)
