TEMPLATE = lib
CONFIG += static
TARGET = $$qtLibraryTarget(preprocessor)

include(../../config.pri)

INCLUDEPATH = $$ANGLE_DIR/src/compiler/preprocessor/new

DEFINES += _SECURE_SCL=0


FLEX_SOURCES =  \
    $$ANGLE_DIR/src/compiler/preprocessor/new/Tokenizer.l

BISON_SOURCES = \
    $$ANGLE_DIR/src/compiler/preprocessor/new/ExpressionParser.y

HEADERS += \
    $$ANGLE_DIR/src/compiler/preprocessor/new/Diagnostics.h \
    $$ANGLE_DIR/src/compiler/preprocessor/new/DirectiveHandler.h \
    $$ANGLE_DIR/src/compiler/preprocessor/new/DirectiveParser.h \
    $$ANGLE_DIR/src/compiler/preprocessor/new/ExpressionParser.h \
    $$ANGLE_DIR/src/compiler/preprocessor/new/Input.h \
    $$ANGLE_DIR/src/compiler/preprocessor/new/Lexer.h \
    $$ANGLE_DIR/src/compiler/preprocessor/new/Macro.h \
    $$ANGLE_DIR/src/compiler/preprocessor/new/MacroExpander.h \
    $$ANGLE_DIR/src/compiler/preprocessor/new/numeric_lex.h \
    $$ANGLE_DIR/src/compiler/preprocessor/new/pp_utils.h \
    $$ANGLE_DIR/src/compiler/preprocessor/new/Preprocessor.h \
    $$ANGLE_DIR/src/compiler/preprocessor/new/SourceLocation.h \
    $$ANGLE_DIR/src/compiler/preprocessor/new/Token.h \
    $$ANGLE_DIR/src/compiler/preprocessor/new/Tokenizer.h

SOURCES += \
    $$ANGLE_DIR/src/compiler/preprocessor/new/Diagnostics.cpp \
    $$ANGLE_DIR/src/compiler/preprocessor/new/DirectiveHandler.cpp \
    $$ANGLE_DIR/src/compiler/preprocessor/new/DirectiveParser.cpp \
    $$ANGLE_DIR/src/compiler/preprocessor/new/Input.cpp \
    $$ANGLE_DIR/src/compiler/preprocessor/new/Lexer.cpp \
    $$ANGLE_DIR/src/compiler/preprocessor/new/Macro.cpp \
    $$ANGLE_DIR/src/compiler/preprocessor/new/MacroExpander.cpp \
    $$ANGLE_DIR/src/compiler/preprocessor/new/Preprocessor.cpp \
    $$ANGLE_DIR/src/compiler/preprocessor/new/Token.cpp

# NOTE: 'win_flex' and 'bison' can be found in qt5/gnuwin32/bin
flex.commands = $$addGnuPath(win_flex) --noline --nounistd --outfile=${QMAKE_FILE_BASE}.cpp ${QMAKE_FILE_NAME}
flex.output = ${QMAKE_FILE_BASE}.cpp
flex.input = FLEX_SOURCES
flex.dependency_type = TYPE_C
flex.variable_out = GENERATED_SOURCES
QMAKE_EXTRA_COMPILERS += flex

bison.commands = $$addGnuPath(bison) --no-lines --skeleton=yacc.c  --output=${QMAKE_FILE_BASE}.cpp ${QMAKE_FILE_NAME}
bison.output = ${QMAKE_FILE_BASE}.cpp
bison.input = BISON_SOURCES
bison.dependency_type = TYPE_C
bison.variable_out = GENERATED_SOURCES
QMAKE_EXTRA_COMPILERS += bison
