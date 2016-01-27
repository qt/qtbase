TEMPLATE = aux
CONFIG -= qt c++11
PREPROCESSOR_SOURCES += c++default.cpp

preprocessor.commands = $(CXX) $(CXXFLAGS) $(INCPATH) -o $@ -E $<
msvc:preprocessor.commands = $(CXX) $(CXXFLAGS) $(INCPATH) -E ${QMAKE_FILE_IN} > ${QMAKE_FILE_OUT}
preprocessor.output = ${QMAKE_FILE_BASE}.ii
preprocessor.input = PREPROCESSOR_SOURCES
preprocessor.variable_out = GENERATED_FILES
QMAKE_EXTRA_COMPILERS += preprocessor

all.target = all
all.depends += c++default.ii
QMAKE_EXTRA_TARGETS += all
