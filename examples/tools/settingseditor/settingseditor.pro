HEADERS       = locationdialog.h \
                mainwindow.h \
                settingstree.h \
                variantdelegate.h
SOURCES       = locationdialog.cpp \
                main.cpp \
                mainwindow.cpp \
                settingstree.cpp \
                variantdelegate.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/settingseditor
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS settingseditor.pro inifiles
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/settingseditor
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
