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

qt_feature_module_end(FeatureTest)
