SOURCES = aesni.cpp
!defined(QMAKE_CFLAGS_AESNI, "var"): error("This compiler does not support AES New Instructions")
else: QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_AESNI
