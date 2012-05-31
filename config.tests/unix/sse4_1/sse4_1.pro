SOURCES = sse4_1.cpp
CONFIG -= x11 qt
mac:CONFIG -= app_bundle
isEmpty(QMAKE_CFLAGS_SSE4_1):error("This compiler does not support SSE4.1")
else:QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_SSE4_1
