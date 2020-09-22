# Generated from qkeysequence.pro.

#####################################################################
## tst_qkeysequence Test:
#####################################################################

qt_internal_add_test(tst_qkeysequence
    SOURCES
        tst_qkeysequence.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set(qkeysequence_resource_files
    "keys_de.qm"
    "qt_de.qm"
)

qt_internal_add_resource(tst_qkeysequence "qkeysequence"
    PREFIX
        "/"
    FILES
        ${qkeysequence_resource_files}
)

