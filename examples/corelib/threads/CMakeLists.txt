# Generated from threads.pro.

add_subdirectory(semaphores)
add_subdirectory(waitconditions)
if(TARGET Qt::Widgets)
    add_subdirectory(mandelbrot)
    add_subdirectory(queuedcustomtype)
endif()
