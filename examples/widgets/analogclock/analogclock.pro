HEADERS       = analogclock.h
SOURCES       = analogclock.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/analogclock
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS analogclock.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/analogclock
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000A64F
    CONFIG += qt_example
}
QT += widgets
