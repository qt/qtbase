include(QtFindWrapHelper NO_POLICY_SCOPE)

set(_qt_wrap_use_bundled FALSE)
if(QT_FEATURE_png AND NOT QT_FEATURE_system_png)
    set(_qt_wrap_use_bundled TRUE)
endif()

qt_find_package_system_or_bundled(wrap_png
    FRIENDLY_PACKAGE_NAME "PNG"
    WRAP_PACKAGE_TARGET "WrapPNG::WrapPNG"
    WRAP_PACKAGE_FOUND_VAR_NAME "WrapPNG_FOUND"
    BUNDLED_PACKAGE_NAME "Qt6BundledLibpng"
    BUNDLED_PACKAGE_TARGET "Qt6::BundledLibpng"
    SYSTEM_PACKAGE_NAME "WrapSystemPNG"
    SYSTEM_PACKAGE_TARGET "WrapSystemPNG::WrapSystemPNG"
    USE_BUNDLED_PACKAGE "${_qt_wrap_use_bundled}"
)
