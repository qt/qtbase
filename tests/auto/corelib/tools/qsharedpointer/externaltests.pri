SOURCES += $$PWD/externaltests.cpp
HEADERS += $$PWD/externaltests.h
cleanedQMAKESPEC = $$replace(QMAKESPEC, \\\\, /)
DEFINES += DEFAULT_MAKESPEC=\\\"$$cleanedQMAKESPEC\\\"

cross_compile:DEFINES += QTEST_NO_RTTI QTEST_CROSS_COMPILED
wince*:DEFINES += QTEST_CROSS_COMPILED QTEST_NO_RTTI
