HEADERS     = codeeditor.h
SOURCES     = main.cpp \
              codeeditor.cpp
# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/codeeditor
sources.files = $$SOURCES $$HEADERS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/codeeditor
INSTALLS += target sources

QT += widgets

