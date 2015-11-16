SOURCES = ssse3.cpp
CONFIG -= qt dylib release debug_and_release
CONFIG += debug console
!defined(QMAKE_CFLAGS_SSSE3, "var"):error("This compiler does not support SSSE3")
else:QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_SSSE3
