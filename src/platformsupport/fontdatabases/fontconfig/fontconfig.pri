HEADERS += $$PWD/qfontconfigdatabase_p.h \
    fontdatabases/fontconfig/qfontenginemultifontconfig_p.h
SOURCES += $$PWD/qfontconfigdatabase.cpp \
    fontdatabases/fontconfig/qfontenginemultifontconfig.cpp
DEFINES -= QT_NO_FONTCONFIG
