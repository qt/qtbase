# Generated from qstringview.pro.

#####################################################################
## tst_qstringview Test:
#####################################################################

qt_internal_add_test(tst_qstringview
    SOURCES
        tst_qstringview.cpp
)

if(QT_FEATURE_sanitize_undefined)
    qt_internal_extend_target(tst_qstringview
        DEFINES
            QT_SANITIZE_UNDEFINED # GCC (in)famously doesn't provide a predefined macro for this
    )
endif()

## Scopes:
#####################################################################
