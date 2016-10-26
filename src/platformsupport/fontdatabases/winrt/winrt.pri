QT *= gui-private

SOURCES += \
    $$PWD/qwinrtfontdatabase.cpp

HEADERS += \
    $$PWD/qwinrtfontdatabase_p.h

DEFINES += __WRL_NO_DEFAULT_LIB__

LIBS += $$QMAKE_LIBS_CORE -ldwrite
