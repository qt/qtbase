SOURCES = shani.cpp
!defined(QMAKE_CFLAGS_SHANI, "var"): error("This compiler does not support Secure Hash Algorithm extensions")
else: QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_SHANI
