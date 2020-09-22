# Generated from testProcessUniqueness.pro.

#####################################################################
## testProcessUniqueness Binary:
#####################################################################

qt_internal_add_executable(testProcessUniqueness
    INSTALL_DIRECTORY "${INSTALL_TESTSDIR}/tst_quuid/testProcessUniqueness"
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
)

# special case begin
set_target_properties(testProcessUniqueness PROPERTIES MACOSX_BUNDLE TRUE)
# special case end
