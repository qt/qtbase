DEFINES += QT_NO_FONTCONFIG

HEADERS += \
        $$PWD/qbasicfontdatabase_p.h \
        $$QT_SOURCE_TREE/src/gui/text/qfontengine_ft_p.h

SOURCES += \
        $$PWD/qbasicfontdatabase.cpp \
        $$QT_SOURCE_TREE/src/gui/text/qfontengine_ft.cpp

CONFIG += opentype

contains(QT_CONFIG, freetype) {
    include($$QT_SOURCE_TREE/src/3rdparty/freetype.pri)
} else:contains(QT_CONFIG, system-freetype) {
    # pull in the proper freetype2 include directory
    include($$QT_SOURCE_TREE/config.tests/unix/freetype/freetype.pri)
}
