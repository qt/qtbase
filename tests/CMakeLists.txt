# Generated from tests.pro.

if(QT_BUILD_STANDALONE_TESTS)
    # Add qt_find_package calls for extra dependencies that need to be found when building
    # the standalone tests here.
    # special case begin
    qt_find_package(WrapDBus1 PROVIDED_TARGETS dbus-1)
    qt_find_package(ICU COMPONENTS i18n uc data PROVIDED_TARGETS ICU::i18n ICU::uc ICU::data)
    qt_find_package(WrapOpenSSL PROVIDED_TARGETS WrapOpenSSL::WrapOpenSSL)
    qt_find_package(WrapOpenSSLHeaders PROVIDED_TARGETS WrapOpenSSLHeaders::WrapOpenSSLHeaders)
    # special case end
endif()
qt_build_tests()
