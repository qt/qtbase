# special case begin

add_library(simplestyleplugin MODULE
    simplestyle.cpp simplestyle.h
    simplestyleplugin.cpp simplestyleplugin.h
)
target_link_libraries(simplestyleplugin PUBLIC Qt::Widgets)

# special case end
