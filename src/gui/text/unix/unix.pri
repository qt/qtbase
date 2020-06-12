HEADERS += text/unix/qgenericunixfontdatabase_p.h

qtConfig(fontconfig) {
    HEADERS += \
        text/unix/qfontconfigdatabase_p.h \
        text/unix/qfontenginemultifontconfig_p.h

    SOURCES += \
        text/unix/qfontconfigdatabase.cpp \
        text/unix/qfontenginemultifontconfig.cpp

    QMAKE_USE_PRIVATE += fontconfig
}
