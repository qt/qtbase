HEADERS += \
        $$PWD/qbasicfontdatabase_p.h \
        $$QT_SOURCE_TREE/src/gui/text/qfontengine_ft_p.h

SOURCES += \
        $$PWD/qbasicfontdatabase.cpp \
        $$QT_SOURCE_TREE/src/gui/text/qfontengine_ft.cpp

QMAKE_USE += freetype
