SOURCES = main.cpp
for (config, SIMD) {
    uc = $$upper($$config)
    DEFINES += QT_COMPILER_SUPPORTS_$${uc}

    add_cflags {
        cflags = QMAKE_CFLAGS_$${uc}
        !defined($$cflags, var): error("This compiler does not support $${uc}")
        QMAKE_CXXFLAGS += $$eval($$cflags)
    }
}
