# Generated from compiler.pro.

#####################################################################
## tst_compiler Test:
#####################################################################

qt_add_test(tst_compiler
    EXCEPTIONS # special case
    SOURCES
        baseclass.cpp baseclass.h
        derivedclass.cpp derivedclass.h
        othersource.cpp
        tst_compiler.cpp
)

## Scopes:
#####################################################################
