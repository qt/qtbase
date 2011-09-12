TEMPLATE		= app
CONFIG		+= qt warn_on
TARGET		= quotedfilenames
SOURCES		= main.cpp

RCCINPUT = "rc folder/test.qrc"
RCCOUTPUT = test.cpp

rcc_test.commands = rcc -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
rcc_test.output = $$RCCOUTPUT
rcc_test.input = RCCINPUT
rcc_test.variable_out = SOURCES
rcc_test.name = RCC_TEST
rcc_test.CONFIG += no_link
rcc_test.depends = $$QMAKE_RCC

QMAKE_EXTRA_COMPILERS += rcc_test

DESTDIR		= ./



