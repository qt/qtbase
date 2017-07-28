SOURCES = ssse3.cpp
!defined(QMAKE_CFLAGS_SSSE3, "var"):error("This compiler does not support SSSE3")
else:QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_SSSE3
