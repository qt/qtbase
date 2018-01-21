QT += widgets
requires(qtConfig(tablewidget))
qtHaveModule(printsupport): QT += printsupport

HEADERS     = detailsdialog.h \
              mainwindow.h
SOURCES     = detailsdialog.cpp \
              main.cpp \
              mainwindow.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/richtext/orderform
INSTALLS += target
