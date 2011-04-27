# Note: These version based 'plugins' are not an actual Qt plugins,
# they are just regular runtime loaded libraries
include(../qpluginbase.pri)

CONFIG  -= plugin

QTDIR_build:DESTDIR	 = $$QT_BUILD_TREE/plugins/s60

MMP_RULES += NOEXPORTLIBRARY

symbian-abld|symbian-sbsv2 {
    defBlock = \
        "$${LITERAL_HASH}ifdef WINSCW" \
        "DEFFILE ../bwins/qts60plugin.def" \
        "$${LITERAL_HASH}else" \
        "DEFFILE ../eabi/qts60plugin.def" \
        "$${LITERAL_HASH}endif"
} else {
    CONFIG *= def_files
    DEF_FILE = ../eabi/qts60pluginu.def
}

MMP_RULES += defBlock