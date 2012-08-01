# Test to see if include(), by default, fails when the specific file 
#   to include does not exist
include(missing_file.pri)
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
