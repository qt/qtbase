# These subprojects look up the same system library dependency separately in each subdirectory
# scope to ensure that we simulate the same scenario as in a top-level qt5.git build, so that
# we try to reuse the same system library entity in multiple sboms.
add_subdirectory(subproj1)
add_subdirectory(subproj2)
