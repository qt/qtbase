# Generated from qmacnativewidget.pro.

#####################################################################
## qmacnativewidget Binary:
#####################################################################

add_qt_executable(qmacnativewidget
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/mac/qmacnativewidget"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/mac/qmacnativewidget"
    SOURCES
        main.mm
    LIBRARIES
        Qt::Widgets
        ${FWAppKit}
)

#### Keys ignored in scope 1:.:qmacnativewidget.pro:<NONE>:
# TEMPLATE = "app"
