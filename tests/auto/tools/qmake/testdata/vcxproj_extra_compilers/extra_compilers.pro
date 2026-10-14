TEMPLATE = app
QT = core
SOURCES = main.cpp

# Two extra compilers that work on the same file, like the ones for DBUS_ADAPTORS.
GEN_INPUT = gen.in

gen_header.input = GEN_INPUT
gen_header.output = ${QMAKE_FILE_BASE}_gen.h
gen_header.commands = copy ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
gen_header.name = Generate header
gen_header.variable_out = GEN_HEADERS

gen_source.input = GEN_INPUT
gen_source.output = ${QMAKE_FILE_BASE}_gen.cpp
gen_source.commands = copy ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
gen_source.name = Generate source
gen_source.variable_out = GENERATED_SOURCES

QMAKE_EXTRA_COMPILERS += gen_header gen_source
