SOURCES = sse4_2.cpp
CONFIG -= x11 qt
mac:CONFIG -= app_bundle
isEmpty(QMAKE_CFLAGS_SSE4_2):error("This compiler does not support SSE4.2")
else:QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_SSE4_2
