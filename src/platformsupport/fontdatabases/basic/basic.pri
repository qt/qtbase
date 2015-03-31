DEFINES += QT_NO_FONTCONFIG

HEADERS += \
        $$PWD/qbasicfontdatabase_p.h \
        $$QT_SOURCE_TREE/src/gui/text/qfontengine_ft_p.h

SOURCES += \
        $$PWD/qbasicfontdatabase.cpp \
        $$QT_SOURCE_TREE/src/gui/text/qfontengine_ft.cpp

CONFIG += opentype

include($$QT_SOURCE_TREE/src/3rdparty/freetype_dependency.pri)
