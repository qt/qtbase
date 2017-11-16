SOURCES = main.cpp
for (config, SIMD) {
    uc = $$upper($$config)
    DEFINES += QT_COMPILER_SUPPORTS_$${uc}
}
