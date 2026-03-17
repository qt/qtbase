QT += widgets
requires(qtConfig(listview))

SOURCES   = adddialog.cpp \
            addresswidget.cpp \
            main.cpp \
            mainwindow.cpp \
            newaddresstab.cpp

HEADERS   = adddialog.h \
            addresswidget.h \
            contact.h \
            mainwindow.h \
            newaddresstab.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/itemviews/addressbook
INSTALLS += target
