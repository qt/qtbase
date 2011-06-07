HEADERS   = layoutitem.h \
            window.h
SOURCES   = layoutitem.cpp \
            main.cpp \
            window.cpp
RESOURCES = basicgraphicslayouts.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/graphicsview/basicgraphicslayouts
sources.files = $$SOURCES $$HEADERS $$RESOURCES basicgraphicslayouts.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/graphicsview/basicgraphicslayouts
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000A645
    CONFIG += qt_example
}
maemo5: CONFIG += qt_example
simulator: warning(This example might not fully work on Simulator platform)
