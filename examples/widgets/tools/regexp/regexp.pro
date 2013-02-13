QT += widgets

HEADERS       = regexpdialog.h
SOURCES       = regexpdialog.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tools/regexp
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)
