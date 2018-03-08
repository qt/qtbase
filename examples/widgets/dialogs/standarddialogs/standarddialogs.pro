QT += widgets
requires(qtConfig(filedialog))

HEADERS       = dialog.h
SOURCES       = dialog.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/dialogs/standarddialogs
INSTALLS += target
