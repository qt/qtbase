qt_pull_features_into_current_scope(PRIVATE_FEATURES Qt::Gui)

if(QT_FEATURE_xcb)
    add_subdirectory(xcb)
endif()

if(APPLE)
    add_subdirectory(cocoa)
endif(APPLE)

