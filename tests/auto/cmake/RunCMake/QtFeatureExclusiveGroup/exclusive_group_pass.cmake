add_library(FeatureTest INTERFACE)
qt_feature_module_begin(LIBRARY FeatureTest NO_HEADERS)

qt_feature("group_a"
    LABEL "group_a"
    AUTODETECT OFF
    ENABLE ON
    EXCLUSIVE_GROUP group
)
qt_feature("group_b"
    LABEL "group_b"
    AUTODETECT OFF
    EXCLUSIVE_GROUP group
)
qt_feature("other_group_a"
    LABEL "other_group_a"
    AUTODETECT OFF
    ENABLE ON
    EXCLUSIVE_GROUP other_group
)
qt_feature("other_group_b"
    LABEL "other_group_b"
    AUTODETECT OFF
    EXCLUSIVE_GROUP other_group
)
qt_feature("no_group_a"
    LABEL "no_group_a"
    AUTODETECT OFF
    ENABLE ON
)
qt_feature("no_group_b"
    LABEL "no_group_b"
    AUTODETECT OFF
    ENABLE ON
)

qt_feature_module_end(FeatureTest)

foreach(feature IN ITEMS group_a other_group_a no_group_a no_group_b)
    if(NOT QT_FEATURE_${feature})
        message(FATAL_ERROR "QT_FEATURE_${feature} should be ON")
    endif()
endforeach()
foreach(feature IN ITEMS group_b other_group_b)
    if(QT_FEATURE_${feature})
        message(FATAL_ERROR "QT_FEATURE_${feature} should be OFF")
    endif()
endforeach()
