qt_path_join(destination ${QT_INSTALL_DIR} "src/3rdparty/gradle")
qt_copy_or_install(
    PROGRAMS
        gradlew
    DESTINATION
        "${destination}"
)

qt_copy_or_install(
    FILES
        gradlew.bat
        gradle.properties
    DESTINATION
        "${destination}"
)

qt_copy_or_install(
    DIRECTORY
        gradle
    DESTINATION
        "${destination}"
)
