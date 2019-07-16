# Generated from mimetypes.pro.

# special case begin
if(FEATURE_private_tests)
# special case end
    add_subdirectory(qmimetype)
    add_subdirectory(qmimedatabase)
# special case begin
endif()
# special case end

if(NOT QT_FEATURE_private_tests)
    ### remove_subdirectory("qmimetype")
endif()
