add_library(QtHarfBuzz STATIC
    src/harfbuzz-shaper-all.cpp
    src/harfbuzz-buffer.c
    src/harfbuzz-gdef.c
    src/harfbuzz-gsub.c
    src/harfbuzz-gpos.c
    src/harfbuzz-impl.c
    src/harfbuzz-open.c
    src/harfbuzz-stream.c
    )
target_include_directories(QtHarfBuzz PUBLIC src)
