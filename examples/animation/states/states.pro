SOURCES += main.cpp
RESOURCES += states.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/animation/states
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS states.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/animation/states
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000E3F8
    CONFIG += qt_example
}
QT += widgets
maemo5: CONFIG += qt_example
