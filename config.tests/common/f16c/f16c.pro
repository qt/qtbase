SOURCES = f16c.cpp
CONFIG -= qt dylib release debug_and_release
CONFIG += debug console
!defined(QMAKE_CFLAGS_F16C, "var"):error("This compiler does not support F16C")
else:QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_F16C
