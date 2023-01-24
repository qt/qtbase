# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_FEATURE_doubleconversion AND NOT QT_FEATURE_system_doubleconversion)
    list(APPEND tst_qstring_extra_defines QT_NO_DOUBLECONVERSION)
endif()

if(APPLE)
    list(APPEND tst_qstring_extra_libraries ${FWFoundation})
    list(APPEND tst_qstring_extra_sources tst_qstring_mac.mm)
endif()
if(WASM)
    list(APPEND tst_qstring_extra_sources tst_qstring_wasm.cpp)
endif()

foreach(test tst_qstring tst_qstring_restricted_ascii tst_qstring_no_cast_from_ascii)
    qt_internal_add_test(${test}
        SOURCES
            tst_qstring.cpp
            ${tst_qstring_extra_sources}
        LIBRARIES
            Qt::CorePrivate
            ${tst_qstring_extra_libraries}
        DEFINES
            ${tst_qstring_extra_defines}
    )
endforeach()

qt_internal_extend_target(tst_qstring_restricted_ascii
    DEFINES
        QT_RESTRICTED_CAST_FROM_ASCII
        tst_QString=tst_QString_restricted_ascii
)

qt_internal_extend_target(tst_qstring_no_cast_from_ascii
    DEFINES
        QT_NO_CAST_FROM_ASCII
        tst_QString=tst_QString_no_cast_from_ascii
)
