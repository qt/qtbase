function(qt_print_feature_summary)
    include(FeatureSummary)
    feature_summary(WHAT PACKAGES_FOUND
                         REQUIRED_PACKAGES_NOT_FOUND
                         RECOMMENDED_PACKAGES_NOT_FOUND
                         OPTIONAL_PACKAGES_NOT_FOUND
                         RUNTIME_PACKAGES_NOT_FOUND
                         FATAL_ON_MISSING_REQUIRED_PACKAGES)
endfunction()

function(qt_print_build_instructions)
    if((NOT PROJECT_NAME STREQUAL "QtBase" AND
        NOT PROJECT_NAME STREQUAL "Qt") OR
       QT_BUILD_STANDALONE_TESTS)

        return()
    endif()

    set(build_command "cmake --build . --parallel")
    set(install_command "cmake --install .")

    message("Qt is now configured for building. Just run '${build_command}'.")
    if(QT_WILL_INSTALL)
        message("Once everything is built, you must run '${install_command}'.")
        message("Qt will be installed into '${CMAKE_INSTALL_PREFIX}'")
    else()
        message("Once everything is built, Qt is installed.")
        message("You should NOT run '${install_command}'")
        message("Note that this build cannot be deployed to other machines or devices.")
    endif()
    message("To configure and build other modules, you can use the following convenience script:
        ${CMAKE_INSTALL_PREFIX}/${INSTALL_BINDIR}/qt-cmake")
    message("\nIf reconfiguration fails for some reason, try to remove 'CMakeCache.txt' \
from the build directory \n")
endfunction()
