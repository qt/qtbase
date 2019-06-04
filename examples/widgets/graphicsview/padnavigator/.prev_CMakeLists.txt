# Generated from padnavigator.pro.

#####################################################################
## padnavigator Binary:
#####################################################################

add_qt_executable(padnavigator
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/graphicsview/padnavigator"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/graphicsview/padnavigator"
    SOURCES
        flippablepad.cpp flippablepad.h
        form.ui
        main.cpp
        padnavigator.cpp padnavigator.h
        roundrectitem.cpp roundrectitem.h
        splashitem.cpp splashitem.h
    PUBLIC_LIBRARIES
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)

# Resources:
add_qt_resource(padnavigator "padnavigator" FILES
    images/artsfftscope.png
    images/blue_angle_swirl.jpg
    images/kontact_contacts.png
    images/kontact_journal.png
    images/kontact_mail.png
    images/kontact_notes.png
    images/kopeteavailable.png
    images/metacontact_online.png
    images/minitools.png)


#### Keys ignored in scope 1:.:.:padnavigator.pro:<TRUE>:
# CONFIG = "console"

## Scopes:
#####################################################################

extend_target(padnavigator CONDITION TARGET Qt::OpenGL
    PUBLIC_LIBRARIES
        Qt::OpenGL
)
