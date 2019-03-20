# Generated from dragdroprobot.pro.

#####################################################################
## dragdroprobot Binary:
#####################################################################

add_qt_executable(dragdroprobot
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/graphicsview/dragdroprobot"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/graphicsview/dragdroprobot"
    SOURCES
        coloritem.cpp coloritem.h
        main.cpp
        robot.cpp robot.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(dragdroprobot "robot" PREFIX "/" FILES
    images/head.png)

