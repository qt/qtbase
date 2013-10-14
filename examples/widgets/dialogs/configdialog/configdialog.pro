QT += widgets

HEADERS     = configdialog.h \
              pages.h
SOURCES     = configdialog.cpp \
              main.cpp \
              pages.cpp
RESOURCES   += configdialog.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/dialogs/configdialog
INSTALLS += target

wince50standard-x86-msvc2005: LIBS += libcmt.lib corelibc.lib ole32.lib oleaut32.lib uuid.lib commctrl.lib coredll.lib winsock.lib ws2.lib
