SOURCES = sse4_2.cpp
!defined(QMAKE_CFLAGS_SSE4_2, "var"):error("This compiler does not support SSE4.2")
else:QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_SSE4_2
