targetinfofile = $$basename(_PRO_FILE_)
targetinfofile ~= s/pro$/target.txt/

win32 {
    ext = .exe
} else:wasm {
    ext = .wasm
}

content = $${file_prefix}$${TARGET}$${ext}
write_file($$OUT_PWD/$$targetinfofile, content)
