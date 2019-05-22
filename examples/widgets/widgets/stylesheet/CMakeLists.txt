# Generated from stylesheet.pro.

#####################################################################
## stylesheet Binary:
#####################################################################

add_qt_executable(stylesheet
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/stylesheet"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/stylesheet"
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h mainwindow.ui
        stylesheeteditor.cpp stylesheeteditor.h stylesheeteditor.ui
    PUBLIC_LIBRARIES
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)

# Resources:
add_qt_resource(stylesheet "stylesheet" PREFIX "/" FILES
    images/checkbox_checked.png
    images/checkbox_checked_hover.png
    images/checkbox_checked_pressed.png
    images/checkbox_unchecked.png
    images/checkbox_unchecked_hover.png
    images/checkbox_unchecked_pressed.png
    images/down_arrow.png
    images/down_arrow_disabled.png
    images/frame.png
    images/pagefold.png
    images/pushbutton.png
    images/pushbutton_hover.png
    images/pushbutton_pressed.png
    images/radiobutton_checked.png
    images/radiobutton_checked_hover.png
    images/radiobutton_checked_pressed.png
    images/radiobutton_unchecked.png
    images/radiobutton_unchecked_hover.png
    images/radiobutton_unchecked_pressed.png
    images/sizegrip.png
    images/spindown.png
    images/spindown_hover.png
    images/spindown_off.png
    images/spindown_pressed.png
    images/spinup.png
    images/spinup_hover.png
    images/spinup_off.png
    images/spinup_pressed.png
    images/up_arrow.png
    images/up_arrow_disabled.png
    layouts/default.ui
    layouts/pagefold.ui
    qss/coffee.qss
    qss/default.qss
    qss/pagefold.qss)

