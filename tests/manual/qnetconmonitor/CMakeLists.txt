# Generated from qnetconmonitor.pro.

#####################################################################
## qnetconmonitor Binary:
#####################################################################

# special case begin
if (NOT QT_FEATURE_private_tests)
    return()
endif()
# special case end

qt_internal_add_manual_test(qnetconmonitor
    SOURCES
        tst_qnetconmonitor.cpp
    PUBLIC_LIBRARIES
        Qt::NetworkPrivate
        Qt::Test
)

#### Keys ignored in scope 1:.:.:qnetconmonitor.pro:<TRUE>:
# TEMPLATE = "app"
