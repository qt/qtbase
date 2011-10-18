HEADERS       = finddialog.h
SOURCES       = finddialog.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dialogs/extension
sources.files = $$SOURCES $$HEADERS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dialogs/extension
INSTALLS += target sources

QT += widgets
simulator: warning(This example might not fully work on Simulator platform)
