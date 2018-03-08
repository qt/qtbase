QT += widgets
requires(qtConfig(combobox))

HEADERS     = mainwindow.h
SOURCES     = main.cpp \
              mainwindow.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/richtext/calendar
INSTALLS += target
