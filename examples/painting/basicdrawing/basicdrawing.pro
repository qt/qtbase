HEADERS       = renderarea.h \
                window.h
SOURCES       = main.cpp \
                renderarea.cpp \
                window.cpp
RESOURCES     = basicdrawing.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/painting/basicdrawing
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS basicdrawing.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/painting/basicdrawing
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000A649
    CONFIG += qt_example
}
QT += widgets
