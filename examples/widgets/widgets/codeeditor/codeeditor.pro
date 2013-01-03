QT += widgets

HEADERS     = codeeditor.h
SOURCES     = main.cpp \
              codeeditor.cpp
# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/codeeditor
INSTALLS += target
