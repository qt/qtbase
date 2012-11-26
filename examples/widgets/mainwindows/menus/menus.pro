HEADERS       = mainwindow.h
SOURCES       = mainwindow.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/mainwindows/menus
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS menus.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/mainwindows/menus
INSTALLS += target sources

QT += widgets
