HEADERS =   softkeys.h
SOURCES += \
            main.cpp \
            softkeys.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/softkeys
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS softkeys.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/softkeys
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000CF6B
    include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
}
