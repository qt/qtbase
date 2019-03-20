# Generated from spinboxdelegate.pro.

#####################################################################
## spinboxdelegate Binary:
#####################################################################

add_qt_executable(spinboxdelegate
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/spinboxdelegate"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/spinboxdelegate"
    SOURCES
        delegate.cpp delegate.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)
