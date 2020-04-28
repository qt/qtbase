# Generated from statemachine.pro.

add_subdirectory(factorial)
add_subdirectory(pingpong)
if(TARGET Qt::Widgets)
    add_subdirectory(eventtransitions)
    add_subdirectory(rogue)
    add_subdirectory(trafficlight)
    add_subdirectory(twowaybutton)
endif()
