# Generated from rasterwindow.pro.

#####################################################################
## None Binary:
#####################################################################

set(sources main.cpp
    rasterwindow.cpp rasterwindow.h)
add_executable(rasterwindow
        ${sources}
)
qt_internal_automoc(rasterwindow ${sources})
target_include_directories(rasterwindow
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}"
    )
target_link_libraries(rasterwindow PRIVATE Qt::Core Qt::Test Qt::Gui)
