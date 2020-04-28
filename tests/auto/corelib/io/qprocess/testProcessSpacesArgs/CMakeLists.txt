# Generated from nospace.pro.

#####################################################################
## nospace Binary:
#####################################################################

qt_add_executable(nospace
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    PUBLIC_LIBRARIES
        Qt::Gui
)

# special case begin
qt_add_executable(onespace
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    PUBLIC_LIBRARIES
        Qt::Gui
)
set_target_properties(onespace PROPERTIES OUTPUT_NAME "one space")

qt_add_executable(twospaces
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    PUBLIC_LIBRARIES
        Qt::Gui
)
set_target_properties(twospaces PROPERTIES OUTPUT_NAME "two space s")
# special case end

#### Keys ignored in scope 1:.:.:nospace.pro:<TRUE>:
# OBJECTS_DIR = "$${OBJECTS_DIR}-nospace"
