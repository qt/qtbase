qt_add_library(qrc_diag_lib STATIC)
target_sources(qrc_diag_lib PRIVATE app.qrc)
target_link_libraries(qrc_diag_lib PRIVATE Qt6::Core)
