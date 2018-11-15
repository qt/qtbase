QT -= gui
CONFIG -= app_bundle
CONFIG += console
SOURCES += main.cpp
FUZZ_ENGINE = $$(LIB_FUZZING_ENGINE)
isEmpty(FUZZ_ENGINE) {
    QMAKE_LFLAGS += -fsanitize=fuzzer
} else {
    LIBS += $$FUZZ_ENGINE
}
