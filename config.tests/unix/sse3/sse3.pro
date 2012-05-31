SOURCES = sse3.cpp
CONFIG -= x11 qt
mac:CONFIG -= app_bundle
isEmpty(QMAKE_CFLAGS_SSE3):error("This compiler does not support SSE3")
else:QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_SSE3
