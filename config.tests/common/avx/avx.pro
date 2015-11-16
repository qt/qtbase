SOURCES = avx.cpp
CONFIG -= qt dylib release debug_and_release
CONFIG += debug console
!defined(QMAKE_CFLAGS_AVX, "var"):error("This compiler does not support AVX")
else:QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_AVX
