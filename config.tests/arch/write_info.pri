targetinfofile = $$basename(_PRO_FILE_)
targetinfofile ~= s/pro$/target.txt/

win32 {
    ext = .exe
} else:android {
    file_prefix = lib
    ext = .so
} else:wasm {
    equals(WASM_OBJECT_FILES, 1): \
        ext = .o
    else: \
        ext = .wasm
}

content = $${file_prefix}$${TARGET}$${ext}
write_file($$OUT_PWD/$$targetinfofile, content)
