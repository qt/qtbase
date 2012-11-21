HEADERS       = window.h
SOURCES       = main.cpp \
                window.cpp
RESOURCES     = systray.qrc

QT += widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/desktop/systray
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS systray.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/desktop/systray
INSTALLS += target sources

simulator: warning(This example might not fully work on Simulator platform)
