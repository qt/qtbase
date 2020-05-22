# Generated from printsupport.pro.

if(WIN32)
    # add_subdirectory(windows) # special case TODO
endif()
if(QT_FEATURE_cups AND UNIX AND NOT APPLE)
    add_subdirectory(cups)
endif()
