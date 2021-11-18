qt_feature("androiddeployqt" PRIVATE
    SECTION "Deployment"
    LABEL "Android deployment tool"
    PURPOSE "The Android deployment tool automates the process of creating Android packages."
    CONDITION NOT CMAKE_CROSSCOMPILING AND QT_FEATURE_regularexpression)

qt_configure_add_summary_section(NAME "Core tools")
qt_configure_add_summary_entry(ARGS "androiddeployqt")
qt_configure_end_summary_section()
