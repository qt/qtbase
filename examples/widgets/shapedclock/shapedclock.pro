HEADERS       = shapedclock.h
SOURCES       = shapedclock.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/shapedclock
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS shapedclock.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/shapedclock
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000C605
    include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
}
