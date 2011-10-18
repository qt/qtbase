HEADERS       = window.h
SOURCES       = window.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/groupbox
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS groupbox.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/groupbox
INSTALLS += target sources

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
