# Android template files

# special case skip regeneration
# special case begin
set(template_files
    AndroidManifest.xml
    build.gradle)

set(template_directories
    res)

qt_install(FILES ${template_files}
    DESTINATION src/android/templates
    COMPONENT Devel)

qt_install(DIRECTORY ${template_directories}
    DESTINATION src/android/templates
    COMPONENT Devel)
# special case end
