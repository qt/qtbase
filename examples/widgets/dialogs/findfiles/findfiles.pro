QT += widgets

HEADERS       = window.h
SOURCES       = main.cpp \
                window.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/dialogs/findfiles
INSTALLS += target
