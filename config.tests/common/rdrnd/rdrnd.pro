SOURCES += rdrnd.cpp
!defined(QMAKE_CFLAGS_RDRND, "var"): error("This compiler does not support the RDRAND instruction")
else: QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_RDRND
