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
target_include_directories(QtHarfBuzz PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)

qt_install_3rdparty_library(QtHarfBuzz)
