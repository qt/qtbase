# Generated from qnetworkreply.pro.

#####################################################################
## tst_manual_qnetworkreply Binary:
#####################################################################

# special case begin
if (NOT QT_FEATURE_private_tests)
    return()
endif()
# special case end

qt_internal_add_manual_test(tst_manual_qnetworkreply
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Network
        Qt::NetworkPrivate
        Qt::Test
)

#### Keys ignored in scope 1:.:.:qnetworkreply.pro:<TRUE>:
# TEMPLATE = "app"
