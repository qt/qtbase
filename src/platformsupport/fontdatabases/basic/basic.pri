HEADERS += \
        $$PWD/qbasicfontdatabase_p.h \
        $$PWD/qfontengine_ft_p.h

SOURCES += \
        $$PWD/qbasicfontdatabase.cpp \
        $$PWD/qfontengine_ft.cpp

QMAKE_USE_PRIVATE += freetype
