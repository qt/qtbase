CONFIG -= qt debug_and_release
# Detect target by preprocessing a file that uses Q_PROCESSOR_* macros from qprocessordetection.h
COMMAND = $$QMAKE_CXX $$QMAKE_CXXFLAGS -E $$PWD/arch.cpp
# system function converts newline in output into spaces
COMPILER_ARCH = $$system($$COMMAND)
# Message back to configure so that it can set QT_ARCH and QT_HOST_ARCH
message($$COMPILER_ARCH)
