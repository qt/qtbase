add_subdirectory(harfbuzz)
add_subdirectory(double-conversion)

#special case begin
if (ANDROID)
    add_subdirectory(gradle)
endif()
#special case end
