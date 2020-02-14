include(QtFindWrapHelper NO_POLICY_SCOPE)

set(_qt_wrap_use_bundled FALSE)
if(QT_FEATURE_freetype AND NOT QT_FEATURE_system_freetype)
    set(_qt_wrap_use_bundled TRUE)
endif()

qt_find_package_system_or_bundled(wrap_freetype
    FRIENDLY_PACKAGE_NAME "Freetype"
    WRAP_PACKAGE_TARGET "WrapFreetype::WrapFreetype"
    WRAP_PACKAGE_FOUND_VAR_NAME "WrapFreetype_FOUND"
    BUNDLED_PACKAGE_NAME "Qt6BundledFreetype"
    BUNDLED_PACKAGE_TARGET "Qt6::BundledFreetype"
    SYSTEM_PACKAGE_NAME "WrapSystemFreetype"
    SYSTEM_PACKAGE_TARGET "WrapSystemFreetype::WrapSystemFreetype"
    USE_BUNDLED_PACKAGE "${_qt_wrap_use_bundled}"
)
