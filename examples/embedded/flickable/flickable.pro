SOURCES = flickable.cpp main.cpp
HEADERS = flickable.h

symbian {
    TARGET.UID3 = 0xA000CF73
    CONFIG += qt_demo
}

target.path = $$[QT_INSTALL_DEMOS]/qtbase/embedded/flickable
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro
sources.path = $$[QT_INSTALL_DEMOS]/qtbase/embedded/flickable
INSTALLS += target sources
