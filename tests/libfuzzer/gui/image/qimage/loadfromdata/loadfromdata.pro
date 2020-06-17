QT += gui
!isEmpty(QT.svg.name) {
    QT += svg
}
QTPLUGIN *= qminimal
SOURCES += main.cpp
FUZZ_ENGINE = $$(LIB_FUZZING_ENGINE)
isEmpty(FUZZ_ENGINE) {
    QMAKE_LFLAGS += -fsanitize=fuzzer
} else {
    LIBS += $$FUZZ_ENGINE
}
