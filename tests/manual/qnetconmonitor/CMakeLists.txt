# Generated from qnetconmonitor.pro.

#####################################################################
## qnetconmonitor Binary:
#####################################################################

# special case begin
if (NOT QT_FEATURE_private_tests)
    return()
endif()
# special case end

add_qt_manual_test(qnetconmonitor
    SOURCES
        tst_qnetconmonitor.cpp
    LIBRARIES
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Network
        Qt::Test
)

#### Keys ignored in scope 1:.:.:qnetconmonitor.pro:<TRUE>:
# TEMPLATE = "app"
