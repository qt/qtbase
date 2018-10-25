# Requires GNU C extensions
CONFIG -= strict_c

INCLUDEPATH += $$PWD/xkbcommon \
               $$PWD/xkbcommon/xkbcommon \
               $$PWD/xkbcommon/src \
               $$PWD/xkbcommon/src/xkbcomp

include($$shadowed($$PWD/../gui/qtgui-config.pri))

# Unused (but needs to be set to something) - we don't use APIs that read xkb
# config files from file system. We use APIs that fetch the necessary keymap
# details directly from X server.
DEFINES += DFLT_XKB_CONFIG_ROOT='\\"/usr/share/X11/xkb\\"'
# Unused (but needs to be set to something) - After QTBUG-42181, this needs to
# be become a configure switch.
DEFINES += XLOCALEDIR='\\"/usr/share/X11/locale/\\"'

### RMLVO names can be overwritten with environmental variables (see libxkbcommon documentation)
DEFINES += DEFAULT_XKB_RULES='\\"evdev\\"'
DEFINES += DEFAULT_XKB_MODEL='\\"pc105\\"'
DEFINES += DEFAULT_XKB_LAYOUT='\\"us\\"'

# Need to rename several files, qmake has problems processing a project when
# sub-directories contain files with an equal names.

SOURCES += \
    $$PWD/xkbcommon/src/keysym-utf.c \
    $$PWD/xkbcommon/src/keymap.c \
    $$PWD/xkbcommon/src/keymap-priv.c \
    $$PWD/xkbcommon/src/utils.c \
    $$PWD/xkbcommon/src/atom.c \
    $$PWD/xkbcommon/src/compose/paths.c \
    $$PWD/xkbcommon/src/compose/parser.c \
    $$PWD/xkbcommon/src/compose/compose-state.c \ # renamed: keymap.c -> compose-state.c
    $$PWD/xkbcommon/src/compose/table.c \
    $$PWD/xkbcommon/src/xkbcomp/xkbcomp-keymap.c \ # renamed: keymap.c -> xkbcomp-keymap.c
    $$PWD/xkbcommon/src/xkbcomp/xkbcomp.c \
    $$PWD/xkbcommon/src/xkbcomp/keymap-dump.c \
    $$PWD/xkbcommon/src/xkbcomp/rules.c \
    $$PWD/xkbcommon/src/xkbcomp/expr.c \
    $$PWD/xkbcommon/src/xkbcomp/action.c \
    $$PWD/xkbcommon/src/xkbcomp/compat.c \
    $$PWD/xkbcommon/src/xkbcomp/types.c \
    $$PWD/xkbcommon/src/xkbcomp/scanner.c \
    $$PWD/xkbcommon/src/xkbcomp/xkbcomp-parser.c \ # renamed: parser.c -> xkbcomp-parser.c
    $$PWD/xkbcommon/src/xkbcomp/ast-build.c \
    $$PWD/xkbcommon/src/xkbcomp/keywords.c \
    $$PWD/xkbcommon/src/xkbcomp/keycodes.c \
    $$PWD/xkbcommon/src/xkbcomp/vmod.c \
    $$PWD/xkbcommon/src/xkbcomp/include.c \
    $$PWD/xkbcommon/src/xkbcomp/symbols.c \
    $$PWD/xkbcommon/src/context-priv.c \
    $$PWD/xkbcommon/src/text.c \
    $$PWD/xkbcommon/src/context.c \
    $$PWD/xkbcommon/src/keysym.c \
    $$PWD/xkbcommon/src/utf8.c \
    $$PWD/xkbcommon/src/state.c

TR_EXCLUDE += $$PWD/*
