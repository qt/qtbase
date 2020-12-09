# Generated from kernel.pro.

add_subdirectory(events)
add_subdirectory(qmetatype)
add_subdirectory(qvariant)
add_subdirectory(qcoreapplication)
add_subdirectory(qtimer_vs_qmetaobject)
if(TARGET Qt::Widgets)
    add_subdirectory(qmetaobject)
    add_subdirectory(qobject)
endif()
if(WIN32)
    add_subdirectory(qwineventnotifier)
endif()
