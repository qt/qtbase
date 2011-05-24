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

symbian: CONFIG += qt_example
QT += widgets
maemo5: CONFIG += qt_example

symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
simulator: warning(This example might not fully work on Simulator platform)
