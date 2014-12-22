SOURCES = ipc.cpp
CONFIG -= qt dylib
mac:CONFIG -= app_bundle
linux:LIBS += -lpthread -lrt
