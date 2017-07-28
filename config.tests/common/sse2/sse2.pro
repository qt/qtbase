SOURCES = sse2.cpp
!defined(QMAKE_CFLAGS_SSE2, var): error("This compiler does not support SSE2")
else:QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_SSE2
