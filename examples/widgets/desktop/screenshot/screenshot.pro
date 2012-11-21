HEADERS             = screenshot.h
SOURCES             = main.cpp \
                      screenshot.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/desktop/screenshot
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS screenshot.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/desktop/screenshot
INSTALLS += target sources

QT += widgets
simulator: warning(This example might not fully work on Simulator platform)
