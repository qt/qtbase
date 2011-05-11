
QT       += core gui

TARGET = applicationicon
TEMPLATE = app

SOURCES += main.cpp

OTHER_FILES += applicationicon.svg \
               applicationicon.png \
               applicationicon.desktop

symbian {
    include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
    # override icon
    ICON = applicationicon.svg
    TARGET.UID3 = 0xe9f919ee
    TARGET.EPOCSTACKSIZE = 0x14000
    TARGET.EPOCHEAPSIZE = 0x020000 0x800000
}

maemo5 {
    include($$QT_SOURCE_TREE/examples/maemo5pkgrules.pri)

    # override icon from maemo5pkgrules.pri
    icon.files = $${TARGET}.png
}
symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
simulator: warning(This example might not fully work on Simulator platform)
