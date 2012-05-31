SOURCES = sse2.cpp
CONFIG -= x11 qt
mac:CONFIG -= app_bundle
isEmpty(QMAKE_CFLAGS_SSE2):error("This compiler does not support SSE2")
else:QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_SSE2
