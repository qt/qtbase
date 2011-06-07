HEADERS     = draglabel.h \
              dragwidget.h
RESOURCES   = draggabletext.qrc
SOURCES     = draglabel.cpp \
              dragwidget.cpp \
              main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/draganddrop/draggabletext
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.txt *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/draganddrop/draggabletext
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000CF64
    CONFIG += qt_example
}
maemo5: CONFIG += qt_example
simulator: warning(This example might not fully work on Simulator platform)
