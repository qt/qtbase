HEADERS       = regexpdialog.h
SOURCES       = regexpdialog.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/regexp
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS regexp.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/regexp
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets
