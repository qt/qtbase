QT *= gui-private

SOURCES += \
    $$PWD/qwinrtfontdatabase.cpp

HEADERS += \
    $$PWD/qwinrtfontdatabase_p.h

DEFINES += __WRL_NO_DEFAULT_LIB__

QMAKE_USE_PRIVATE += dwrite_1 ws2_32
