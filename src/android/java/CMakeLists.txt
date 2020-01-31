# Android Java Resource files

# special case skip regeneration
# special case begin
set(resource_directories
    res
    src)

qt_install(DIRECTORY ${resource_directories}
    DESTINATION src/android/java
    COMPONENT Devel)
# special case end
