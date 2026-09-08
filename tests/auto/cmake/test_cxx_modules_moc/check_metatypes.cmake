# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Verifies that the meta types collected for test_cxx_modules_moc cover the Q_OBJECT classes of
# all module unit kinds, not just the ones from ordinary sources and headers.

if(NOT EXISTS "${metatypes_file}")
    message(FATAL_ERROR "Meta types file ${metatypes_file} does not exist.")
endif()

file(READ "${metatypes_file}" metatypes_content)

# Receiver comes from an ordinary source, the rest from the primary module interface unit, an
# interface partition and an internal partition, in that order.
foreach(class_name Receiver PrimaryObject PartObject InternalObject)
    if(NOT metatypes_content MATCHES "\"className\": \"${class_name}\"")
        message(FATAL_ERROR
            "Meta types file ${metatypes_file} does not contain class ${class_name}.")
    endif()
endforeach()
