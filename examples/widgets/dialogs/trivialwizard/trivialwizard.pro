QT += widgets

SOURCES       = trivialwizard.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/dialogs/trivialwizard
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)
