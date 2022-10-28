# Generated from qapplication.pro.

add_subdirectory(desktopsettingsaware)
add_subdirectory(modal)
add_subdirectory(test)

add_dependencies(tst_qapplication
    desktopsettingsaware_helper
    modal_helper
)
