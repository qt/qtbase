# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(make_includable input_file output_file)
    get_filename_component(infile "${CMAKE_CURRENT_SOURCE_DIR}/${input_file}" ABSOLUTE)
    set(outfile ${CMAKE_CURRENT_BINARY_DIR}/${output_file})
    file(READ ${infile} content)
    set(content "R\"(${content})\"")
    file(WRITE ${outfile} "${content}")
endfunction(make_includable)

make_includable(metadata_template.txt metadata_template.h)

qt_internal_add_plugin(QCtfTracePlugin
    SHARED
    CLASS_NAME QCtfTracePlugin
    PLUGIN_TYPE tracing
    SOURCES
        qctflib_p.h qctflib.cpp metadata_template.txt qctfplugin.cpp qctfplugin_p.h
    LIBRARIES
        Qt6::Core Qt6::CorePrivate
)

