qt_feature("qmake" PRIVATE
    SECTION "Core tools"
    LABEL "qmake tool"
    PURPOSE "The qmake tool helps simplify the build process for development projects across different platforms"
    CONDITION QT_FEATURE_settings AND QT_FEATURE_alloca AND
        (QT_FEATURE_alloca_malloc_h OR NOT WIN32) AND QT_FEATURE_cborstreamwriter AND
        QT_FEATURE_datestring AND QT_FEATURE_regularexpression AND QT_FEATURE_temporaryfile
)

qt_configure_add_summary_section(NAME "Core tools")
qt_configure_add_summary_entry(ARGS "qmake")
qt_configure_end_summary_section()
