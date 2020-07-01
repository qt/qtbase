# Generated from rhi.pro.

add_subdirectory(qshader)
# special case begin
# QTBUG-85364
if(NOT LINUX)
    add_subdirectory(qrhi)
endif()
# special case end
