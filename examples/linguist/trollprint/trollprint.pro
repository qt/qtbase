HEADERS       = mainwindow.h \
                printpanel.h
SOURCES       = main.cpp \
                mainwindow.cpp \
                printpanel.cpp
TRANSLATIONS  = trollprint_pt.ts

# install
target.path = $$[QT_INSTALL_EXAMPLES]/linguist/trollprint
INSTALLS += target

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
