QT += widgets

HEADERS       = randomlistmodel.h
SOURCES       = randomlistmodel.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/tools/contiguouscache
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)
