# Generated from qkeysequence.pro.

#####################################################################
## tst_qkeysequence Test:
#####################################################################

# Resources:
set(qkeysequence_resource_files
    "keys_de.qm"
    "qt_de.qm"
)

qt_internal_add_test(tst_qkeysequence
    SOURCES
        tst_qkeysequence.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA ${qkeysequence_resource_files}
    BUILTIN_TESTDATA
)
