SOURCES = avx512.cpp
CONFIG -= qt dylib release debug_and_release
CONFIG += debug console

isEmpty(AVX512): error("You must set the AVX512 variable!")

varname = QMAKE_CFLAGS_AVX512$$AVX512
value = $$eval($$varname)
isEmpty($$varname): error("This compiler does not support AVX512")

QMAKE_CXXFLAGS += $$value
DEFINES += AVX512WANT=$$AVX512
