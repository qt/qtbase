HEADERS       = licensewizard.h
SOURCES       = licensewizard.cpp \
                main.cpp
RESOURCES     = licensewizard.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/dialogs/licensewizard
sources.files = $$SOURCES $$HEADERS $$FORMS $$RESOURCES *.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/dialogs/licensewizard
INSTALLS += target sources

QT += widgets printsupport
simulator: warning(This example might not fully work on Simulator platform)
