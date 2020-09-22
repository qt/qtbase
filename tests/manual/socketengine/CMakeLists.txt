# Generated from socketengine.pro.

#####################################################################
## tst_socketengine Binary:
#####################################################################

# special case begin
if (NOT QT_FEATURE_private_tests)
    return()
endif()
# special case end

qt_internal_add_manual_test(tst_socketengine
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
        Qt::Test
)

#### Keys ignored in scope 1:.:.:socketengine.pro:<TRUE>:
# TEMPLATE = "app"
