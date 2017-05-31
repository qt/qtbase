INCLUDEPATH += $$PWD $$QT_BUILD_TREE/src/tools/uic

DEFINES += QT_UIC_CPP_GENERATOR

# Input
HEADERS += $$PWD/cppwritedeclaration.h \
           $$PWD/cppwriteincludes.h \
           $$PWD/cppwriteinitialization.h

SOURCES += $$PWD/cppwritedeclaration.cpp \
           $$PWD/cppwriteincludes.cpp \
           $$PWD/cppwriteinitialization.cpp
