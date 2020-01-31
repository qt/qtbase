# Generated from qnetworkreply.pro.

#####################################################################
## tst_manual_qnetworkreply Binary:
#####################################################################

# special case begin
if (NOT QT_FEATURE_private_tests)
    return()
endif()
# special case end

add_qt_manual_test(tst_manual_qnetworkreply
    SOURCES
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Network
        Qt::Test
)

#### Keys ignored in scope 1:.:.:qnetworkreply.pro:<TRUE>:
# TEMPLATE = "app"
