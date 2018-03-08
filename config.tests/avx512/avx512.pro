SOURCES = avx512.cpp

!defined(AVX512, "var"): error("You must set the AVX512 variable!")

varname = QMAKE_CFLAGS_AVX512$$AVX512
value = $$eval($$varname)
!defined($$varname, "var"): error("This compiler does not support AVX512")

QMAKE_CXXFLAGS += $$value
DEFINES += WANT_AVX512=$$AVX512 WANT_AVX512$$AVX512
