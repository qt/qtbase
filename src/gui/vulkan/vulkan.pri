qtConfig(vulkan) {
    CONFIG += generated_privates

    HEADERS += \
        vulkan/qvulkaninstance.h \
        vulkan/qplatformvulkaninstance.h \
        vulkan/qvulkanwindow.h \
        vulkan/qvulkanwindow_p.h

    SOURCES += \
        vulkan/qvulkaninstance.cpp \
        vulkan/qplatformvulkaninstance.cpp \
        vulkan/qvulkanfunctions.cpp \
        vulkan/qvulkanwindow.cpp

    # Applications must inherit the Vulkan header include path.
    QMAKE_USE += vulkan/nolink
}

qtConfig(vkgen) {
    # Generate qvulkanfunctions.h, qvulkanfunctions_p.h, qvulkanfunctions_p.cpp
    QMAKE_QVKGEN_INPUT = vulkan/vk.xml
    QMAKE_QVKGEN_LICENSE_HEADER = $$QT_SOURCE_TREE/header.LGPL
    qtPrepareTool(QMAKE_QVKGEN, qvkgen)

    qvkgen_h.commands = $$QMAKE_QVKGEN ${QMAKE_FILE_IN} $$shell_quote($$QMAKE_QVKGEN_LICENSE_HEADER) ${QMAKE_FILE_OUT_PATH}/${QMAKE_FILE_OUT_BASE}
    qvkgen_h.output = $$OUT_PWD/vulkan/qvulkanfunctions.h
    qvkgen_h.input = QMAKE_QVKGEN_INPUT
    qtConfig(vulkan): \
        qvkgen_h.variable_out = HEADERS
    else: \
        qvkgen_h.CONFIG += target_predeps no_link
    QMAKE_EXTRA_COMPILERS += qvkgen_h

    qvkgen_ph.commands = $$escape_expand(\\n)
    qvkgen_ph.output = $$OUT_PWD/vulkan/qvulkanfunctions_p.h
    qvkgen_ph.input = QMAKE_QVKGEN_INPUT
    qvkgen_ph.depends += $$OUT_PWD/vulkan/qvulkanfunctions.h
    qtConfig(vulkan): \
        qvkgen_ph.variable_out = HEADERS
    else: \
        qvkgen_ph.CONFIG += target_predeps no_link
    QMAKE_EXTRA_COMPILERS += qvkgen_ph

    qvkgen_pimpl.commands = $$escape_expand(\\n)
    qvkgen_pimpl.output = $$OUT_PWD/vulkan/qvulkanfunctions_p.cpp
    qvkgen_pimpl.input = QMAKE_QVKGEN_INPUT
    qvkgen_pimpl.depends += $$OUT_PWD/vulkan/qvulkanfunctions_p.h
    qtConfig(vulkan): \
        qvkgen_pimpl.variable_out = SOURCES
    else: \
        qvkgen_pimpl.CONFIG += target_predeps no_link
    QMAKE_EXTRA_COMPILERS += qvkgen_pimpl
} else {
    # generate dummy files to make qmake happy
    write_file($$OUT_PWD/vulkan/qvulkanfunctions.h)
    write_file($$OUT_PWD/vulkan/qvulkanfunctions_p.h)
}

# Ensure qvulkanfunctions.h gets installed correctly
targ_headers.CONFIG += no_check_exist
