# special case begin

install(
    FILES
        gradlew
        gradlew.bat
    DESTINATION
        src/3rdparty/gradle
    COMPONENT
        Devel)

install(
    DIRECTORY
        gradle
    DESTINATION
        src/3rdparty/gradle
    COMPONENT
        Devel)

# special case end
