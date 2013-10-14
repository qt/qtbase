QT += widgets

HEADERS       = finddialog.h
SOURCES       = finddialog.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/dialogs/extension
INSTALLS += target
