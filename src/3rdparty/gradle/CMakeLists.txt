# special case begin
# special case skip regeneration

qt_path_join(destination ${QT_INSTALL_DIR} "src/3rdparty/gradle")
qt_copy_or_install(
    FILES
        gradlew
        gradlew.bat
    DESTINATION
        "${destination}"
)

qt_copy_or_install(
    DIRECTORY
        gradle
    DESTINATION
        "${destination}"
)

# special case end
