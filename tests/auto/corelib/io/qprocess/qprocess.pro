TEMPLATE = subdirs

include(qprocess.pri)
SUBDIRS  = $$SUBPROGRAMS
# Add special cases
SUBDIRS += testProcessSpacesArgs/nospace.pro \
           testProcessSpacesArgs/onespace.pro \
           testProcessSpacesArgs/twospaces.pro \
           testSpaceInName

win32:!wince* {
    SUBDIRS += \
        testProcessEchoGui \
        testSetNamedPipeHandleState
}

test.depends += $$SUBDIRS
SUBDIRS += test
