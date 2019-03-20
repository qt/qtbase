# Generated from blurpicker.pro.

#####################################################################
## blurpicker Binary:
#####################################################################

add_qt_executable(blurpicker
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/effects/blurpicker"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/effects/blurpicker"
    SOURCES
        blureffect.cpp blureffect.h
        blurpicker.cpp blurpicker.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(blurpicker "blurpicker" PREFIX "/" FILES
    images/accessories-calculator.png
    images/accessories-text-editor.png
    images/background.jpg
    images/help-browser.png
    images/internet-group-chat.png
    images/internet-mail.png
    images/internet-web-browser.png
    images/office-calendar.png
    images/system-users.png)

