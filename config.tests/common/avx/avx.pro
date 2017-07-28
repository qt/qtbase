SOURCES = avx.cpp
!defined(QMAKE_CFLAGS_AVX, "var"):error("This compiler does not support AVX")
else:QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_AVX
