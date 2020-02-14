include(QtFindWrapHelper NO_POLICY_SCOPE)

set(_qt_wrap_use_bundled FALSE)
if(QT_FEATURE_harfbuzz AND NOT QT_FEATURE_system_harfbuzz)
    set(_qt_wrap_use_bundled TRUE)
endif()

qt_find_package_system_or_bundled(wrap_harfbuzz
    FRIENDLY_PACKAGE_NAME "Harfbuzz"
    WRAP_PACKAGE_TARGET "WrapHarfbuzz::WrapHarfbuzz"
    WRAP_PACKAGE_FOUND_VAR_NAME "WrapHarfbuzz_FOUND"
    BUNDLED_PACKAGE_NAME "Qt6BundledHarfbuzz"
    BUNDLED_PACKAGE_TARGET "Qt6::BundledHarfbuzz"
    SYSTEM_PACKAGE_NAME "WrapSystemHarfbuzz"
    SYSTEM_PACKAGE_TARGET "WrapSystemHarfbuzz::WrapSystemHarfbuzz"
    USE_BUNDLED_PACKAGE "${_qt_wrap_use_bundled}"
)
