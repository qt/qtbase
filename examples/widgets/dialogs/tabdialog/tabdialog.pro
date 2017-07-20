QT += widgets

HEADERS       = tabdialog.h
SOURCES       = main.cpp \
                tabdialog.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/dialogs/tabdialog
INSTALLS += target
