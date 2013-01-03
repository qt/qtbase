QT += widgets

HEADERS       = tabdialog.h
SOURCES       = main.cpp \
                tabdialog.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/dialogs/tabdialog
INSTALLS += target

wince50standard-x86-msvc2005: LIBS += libcmt.lib corelibc.lib ole32.lib oleaut32.lib uuid.lib commctrl.lib coredll.lib winsock.lib ws2.lib
