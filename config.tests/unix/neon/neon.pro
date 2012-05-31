SOURCES = neon.cpp
CONFIG -= x11 qt
isEmpty(QMAKE_CFLAGS_NEON):error("This compiler does not support Neon")
else:QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_NEON
