include(QtFindWrapHelper NO_POLICY_SCOPE)

set(_qt_wrap_use_bundled FALSE)
if(QT_FEATURE_pcre2 AND NOT QT_FEATURE_system_pcre2)
    set(_qt_wrap_use_bundled TRUE)
endif()

qt_find_package_system_or_bundled(wrap_pcre2
    FRIENDLY_PACKAGE_NAME "PCRE2"
    WRAP_PACKAGE_TARGET "WrapPCRE2::WrapPCRE2"
    WRAP_PACKAGE_FOUND_VAR_NAME "WrapPCRE2_FOUND"
    BUNDLED_PACKAGE_NAME "Qt6BundledPcre2"
    BUNDLED_PACKAGE_TARGET "Qt6::BundledPcre2"
    SYSTEM_PACKAGE_NAME "WrapSystemPCRE2"
    SYSTEM_PACKAGE_TARGET "WrapSystemPCRE2::WrapSystemPCRE2"
    USE_BUNDLED_PACKAGE "${_qt_wrap_use_bundled}"
)
