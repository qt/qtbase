TARGET = arch
SOURCES = arch.cpp
CONFIG -= qt dylib release debug_and_release
CONFIG += debug console
mac:CONFIG -= app_bundle
