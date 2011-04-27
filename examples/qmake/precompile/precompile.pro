#############################################
#
# Example for using Precompiled Headers
#
#############################################
#! [0]
TEMPLATE  = app
LANGUAGE  = C++
CONFIG	 += console precompile_header

# Use Precompiled headers (PCH)
PRECOMPILED_HEADER  = stable.h

HEADERS	  = stable.h \
            mydialog.h \
            myobject.h
SOURCES	  = main.cpp \
            mydialog.cpp \
            myobject.cpp \
            util.cpp
FORMS     = mydialog.ui
#! [0]
