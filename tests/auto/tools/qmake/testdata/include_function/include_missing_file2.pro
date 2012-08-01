# Specifying full set of parameters to include() to test that a warning 
#   is shown for this non-existing file.
include(missing_file.pri, "", false)
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
