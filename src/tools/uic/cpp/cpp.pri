INCLUDEPATH += $$PWD $$QT_BUILD_TREE/src/tools/uic

# Input
HEADERS += $$PWD/cppwritedeclaration.h \
           $$PWD/cppwriteincludes.h \
           $$PWD/cppwriteinitialization.h

SOURCES += $$PWD/cppwritedeclaration.cpp \
           $$PWD/cppwriteincludes.cpp \
           $$PWD/cppwriteinitialization.cpp
