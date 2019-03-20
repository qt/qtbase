# Generated from regularexpression.pro.

#####################################################################
## regularexpression Binary:
#####################################################################

add_qt_executable(regularexpression
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/regularexpression"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/regularexpression"
    SOURCES
        main.cpp
        regularexpressiondialog.cpp regularexpressiondialog.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(regularexpression "regularexpression" FILES
    images/copy.png)

