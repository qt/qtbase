HEADERS       = regexpdialog.h
SOURCES       = regexpdialog.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/regexp
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS regexp.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/regexp
INSTALLS += target sources

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
