# Generated from savegame.pro.

#####################################################################
## savegame Binary:
#####################################################################

add_qt_executable(savegame
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/corelib/serialization/savegame"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/corelib/serialization/savegame"
    SOURCES
        character.cpp character.h
        game.cpp game.h
        level.cpp level.h
        main.cpp
    LIBRARIES
        # Remove: gui
)

#### Keys ignored in scope 1:.:savegame.pro:<NONE>:
# CONFIG = "-app_bundle"
# TEMPLATE = "app"
