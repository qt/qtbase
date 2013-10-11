QT += widgets printsupport

HEADERS       = licensewizard.h
SOURCES       = licensewizard.cpp \
                main.cpp
RESOURCES     = licensewizard.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/dialogs/licensewizard
INSTALLS += target
