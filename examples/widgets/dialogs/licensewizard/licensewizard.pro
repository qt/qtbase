QT += widgets printsupport

HEADERS       = licensewizard.h
SOURCES       = licensewizard.cpp \
                main.cpp
RESOURCES     = licensewizard.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/dialogs/licensewizard
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)
