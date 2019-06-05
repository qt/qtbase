# Generated from android.pro.

# special case begin
# Only build jars during android build?
if (ANDROID)
    add_subdirectory(jar)
endif()
# special case end

# Templates need to be installed during host build
add_subdirectory(java)
add_subdirectory(templates)
