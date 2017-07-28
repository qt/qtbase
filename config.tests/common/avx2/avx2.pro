SOURCES = avx2.cpp
!defined(QMAKE_CFLAGS_AVX2, "var"):error("This compiler does not support AVX2")
else:QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_AVX2
