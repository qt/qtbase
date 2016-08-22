TEMPLATE = lib
CONFIG += dll bsymbolic_functions
SOURCES = bsymbolic_functions.c

isEmpty(QMAKE_LFLAGS_BSYMBOLIC_FUNC): error("Nope")
