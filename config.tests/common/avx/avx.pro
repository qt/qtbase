SOURCES = avx.cpp
CONFIG -= x11 qt
mac:CONFIG -= app_bundle
isEmpty(QMAKE_CFLAGS_AVX):error("This compiler does not support AVX")
else:QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_AVX
