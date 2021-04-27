# Generated from app.pro.
# special case skip regeneration

qt_add_executable(plugandpaint
    interfaces.h
    main.cpp
    mainwindow.cpp mainwindow.h
    paintarea.cpp paintarea.h
    plugindialog.cpp plugindialog.h
)
set_target_properties(plugandpaint PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(plugandpaint PUBLIC
    Qt::Widgets
    pnp_basictools
)

install(TARGETS plugandpaint
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
