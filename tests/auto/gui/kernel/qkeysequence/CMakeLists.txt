# Generated from qkeysequence.pro.

#####################################################################
## tst_qkeysequence Test:
#####################################################################

add_qt_test(tst_qkeysequence
    SOURCES
        tst_qkeysequence.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
)

# Resources:
set(qkeysequence_resource_files
    "keys_de.qm"
    "qt_de.qm"
)

add_qt_resource(tst_qkeysequence "qkeysequence"
    PREFIX
        "/"
    FILES
        ${qkeysequence_resource_files}
)

