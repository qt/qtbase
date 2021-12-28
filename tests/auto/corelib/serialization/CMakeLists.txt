# Generated from serialization.pro.

add_subdirectory(json)
add_subdirectory(qcborstreamreader)
add_subdirectory(qcborstreamwriter)
add_subdirectory(qcborvalue)
add_subdirectory(qcborvalue_json)
if(TARGET Qt::Gui)
    add_subdirectory(qdatastream)
    add_subdirectory(qdatastream_core_pixmap)
endif()
if(TARGET Qt::Network)
    add_subdirectory(qtextstream)
endif()
# QTBUG-87671 # special case
if(TARGET Qt::Network AND TARGET Qt::Xml AND NOT ANDROID AND NOT INTEGRITY)
    add_subdirectory(qxmlstream)
endif()
